#include <algorithm>
#include <cctype>
#include <exception>
#include <string>
#include <vector>

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#define BOOST_SPIRIT_DEBUG 1

#include <boost/fusion/include/adapt_struct_named.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <filesystem/path.h>

#include <lunar/concatenate.hpp>

#include <lunar/parser.hpp>

namespace spirit    = boost::spirit;
namespace phx       = boost::phoenix;
namespace qi        = boost::spirit::qi;
namespace ascii     = boost::spirit::ascii;
namespace bf        = boost::fusion;

BOOST_FUSION_ADAPT_STRUCT( lun::keyword, name, xs )
// NB! reversed member order in the adapted struct, because in the grammar,
// repeats comes first
BOOST_FUSION_ADAPT_STRUCT( lun::item, repeat, val )
BOOST_FUSION_ADAPT_STRUCT( lun::item::star, val )

namespace lun {

namespace {

template< typename Itr >
struct skipper : public qi::grammar< Itr > {
    skipper() : skipper::base_type( skip ) {
        skip = ascii::space
             | "--" >> *(qi::char_ - qi::eol)
        ;
    };

    qi::rule< Itr > skip;
};

template< typename Itr >
qi::rule< Itr, item::star() > star = qi::int_ >> '*';

/*
 * keyword lookup through a qi::symbol table is pretty convenient, but that
 * only selects rule, it doesn't preserve the string. This nifty macro makes
 * will ensure the found keyword is synthesized properly. It also ensures that
 * a keyword is only a match if it matches the *complete* input, i.e. stops the
 * 'SYM' keyword would otherwise match the string 'SYMBOLS'
 */
#define kword(sym) qi::raw[qi::lexeme[(sym) >> !qi::alnum]]

template< typename Itr >
qi::rule< Itr, std::string() > str =
      '\'' >> *(qi::char_ - '\'') >> '\''
    | '"'  >> *(qi::char_ - '"')  >> '"'
    | qi::alpha >> *qi::alnum
;

template< typename T >
struct fortran_double : qi::real_policies< T > {
    // Eclipse supports Fortran syntax for specifying exponents of floating
    // point numbers ('D' and 'E', e.g., 1.234d5)
    template< typename It >
    static bool parse_exp( It& first, const It& last ) {
        if( first == last ||
            (*first != 'e' && *first != 'E' &&
            *first != 'd' && *first != 'D' ) )
            return false;
        ++first;
        return true;
    }
};

qi::real_parser< double, fortran_double< double > > f77float;

/* a typical deck contains a LOT more int-values than doubles, which means that
 * by checking for doubles first, we must essentially always backtrack, which
 * is somewhat expensive.
 *
 * TODO: figure out rule that allows valid ints to parse first that isn't
 * horribly complicated or expensive
 */

#define primary() (qi::int_ >> !qi::char_(".eEdD") | f77float)
#define term()    (qi::lit('/') >> qi::skip(qi::char_ - qi::eol)[qi::eps])

/*
 * The basic rule for parsing 1 record (aka list of items), including repeats
 *
 * The rule itself is variadically parametrised on a set of types, which allows
 * rules to share the same basic name, but restrict what types they accept.
 *
 * the rule <int> will only accept integers and defaults, and fail on doubles
 * and strings, and similarly for <double> and <string>. <int, double> will
 * fail encountering strings, but work for int and doubles. This is because a
 * lot of keywords accept only the one data type, and can report that error
 * early, where some keywords accept mixed int/double/string and will usually
 * be deferred to handle input errors at a later stage.
 *
 * It's motivated primarily by:
 * - most rules don't accept strings (and a lot don't accept doubles), so
 *   performance should improve slightly by having to consider less
 *   combinations (-> backtracking)
 * - we can report some type errors earlier
 * - the rule inputs become somewhat clearer
 */
template< typename Itr, typename... >
qi::rule< Itr, item() > itemrule;

static const item endrec = { item::endrec{} };

/*
 * TODO: optimise backtracking patterns
 */
template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int > =
      qi::attr( item::star( 0 ) ) >> qi::int_ >> !qi::lit('*')
    | star< Itr > >> qi::int_
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, double > =
      qi::attr( item::star( 0 ) ) >> f77float >> !qi::lit('*')
    | star< Itr > >> f77float
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, double > =
      qi::attr( item::star( 0 ) ) >> primary() >> !qi::lit('*')
    | star< Itr > >> primary()
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, std::string > =
     qi::attr( item::star( 0 ) ) >> str< Itr > >> !qi::lit('*')
    | star< Itr > >> str< Itr >
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, std::string > =
      itemrule< Itr, int >
    | itemrule< Itr, std::string >
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, double, std::string > =
      itemrule< Itr, int >
    | itemrule< Itr, double >
    | itemrule< Itr, std::string >
;

template< typename Itr, typename... T >
qi::rule< Itr, std::vector< item >(), skipper< Itr > > record_rule =
    *( itemrule< Itr, T... >
     | star< Itr > >> qi::attr( item::none{} )
     | '*' >> qi::attr( item::star( 0 ) ) >> qi::attr( item::none{} )
     )
    >> term() >> qi::attr( endrec );

std::vector< item > flatten( const std::vector< std::vector< item > >& xs ) {
    auto size = std::accumulate( xs.begin(), xs.end(), 0,
            []( auto acc, auto& x ) { return x.size() + acc; } );

    std::vector< item > ys;
    ys.reserve( size );
    for( const auto& x : xs )
        ys.insert( ys.end(), x.begin(), x.end() );

    return ys;
}

template< typename Itr, int N, typename... T >
qi::rule< Itr, std::vector< item >(), skipper< Itr > > rec =
    qi::repeat(N)[record_rule< Itr, T... >]
        [ qi::_val = phx::bind( flatten, qi::_1 ) ]
    ;

template< typename Itr >
struct grammar : qi::grammar< Itr, std::vector< keyword >(), skipper< Itr >,
        qi::locals< qi::rule< Itr, std::vector< item >(), skipper< Itr > >* >
    > {
    grammar() : grammar::base_type( start ) {

        toggle %= qi::eps;

        /* RUNSPEC */
        keyword.add
            ( "RUNSPEC",    &toggle )

            ( "OIL",        &toggle )
            ( "WATER",      &toggle )
            ( "GAS",        &toggle )
            ( "DISGAS",     &toggle )
            ( "VAPOIL",     &toggle )
            ( "METRIC",     &toggle )
            ( "FIELD",      &toggle )
            ( "LAB",        &toggle )
            ( "NOSIM",      &toggle )
            ( "UNIFIN",     &toggle )
            ( "UNIFOUT",    &toggle )

            ( "DIMENS",     &rec< Itr, 1, int > )
            ( "EQLDIMS",    &rec< Itr, 1, int > )
            ( "REGDIMS",    &rec< Itr, 1, int > )
            ( "WELLDIMS",   &rec< Itr, 1, int > )
            ( "VFPIDIMS",   &rec< Itr, 1, int > )
            ( "VFPPDIMS",   &rec< Itr, 1, int > )
            ( "FAULTDIM",   &rec< Itr, 1, int > )
            ( "PIMTDIMS",   &rec< Itr, 1, int > )
            ( "NSTACK",     &rec< Itr, 1, int > )
            ( "OPTIONS",    &rec< Itr, 1, int > )

            ( "EQLOPTS",    &rec< Itr, 1, std::string > )
            ( "SATOPTS",    &rec< Itr, 1, std::string > )

            ( "ENDSCALE",   &rec< Itr, 1, int, std::string > )
            ( "GRIDOPTS",   &rec< Itr, 1, int, std::string > )
            ( "START",      &rec< Itr, 1, int, std::string > )
            ( "TABDIMS",    &rec< Itr, 1, int, std::string > )

            ( "TRACERS",    &rec< Itr, 1, int, double, std::string > )

        /* GRID */
            ( "GRID",       &toggle )
            ( "NEWTRAN",    &toggle )
            ( "GRIDFILE",   &rec< Itr, 1, int > )
            ( "MAPAXES",    &rec< Itr, 1, double > )
        ;

        start %= *( kword(keyword[ qi::_a = qi::_1 ]) >> qi::lazy( *qi::_a ))
            >> qi::eoi
        ;

        itemrule< Itr >.name( "item" );
        itemrule< Itr, int >.name( "item[int]" );
        itemrule< Itr, double >.name( "item[flt]" );
        itemrule< Itr, int, double >.name( "item[int|flt]" );
        itemrule< Itr, std::string >.name( "item[str]" );
        itemrule< Itr, int, std::string >.name( "item[int|str]" );
        itemrule< Itr, int, double, std::string >.name( "item[*]" );
    }

    using rule = qi::rule< Itr, std::vector< item >(), skipper< Itr > >;

    rule toggle;
    qi::symbols< char, rule* > keyword;
    qi::rule< Itr, std::vector< lun::keyword >(), skipper< Itr >, qi::locals< rule* > > start;
};

}

auto INCLUDE( const char*& fst, const char* lst ) -> std::string {
    using Itr = const char*;

    std::string included;
    auto ok = qi::phrase_parse( fst, lst,
            "INCLUDE" >> str< Itr > >> term(),
            skipper< Itr >(), included );

    if( !ok ) throw std::runtime_error( "Invalid INCLUDE" );

    return included;
}

auto PATHS( const char*& fst, const char* lst ) ->
    std::vector< std::pair< std::string, std::string > > {

    using Itr = const char*;

    std::vector< std::pair< std::string, std::string > > paths;
    auto ok = qi::phrase_parse( fst, lst,
            "PATHS" >> *(str< Itr > >> str< Itr > >> term()) >> term(),
            skipper< Itr >(), paths );

    if( !ok ) throw std::runtime_error( "Invalid PATHS" );

    return paths;
}

std::ostream& operator<<( std::ostream& stream, const item::star& s ) {
    return stream << int(s) << "*";
}

std::ostream& operator<<( std::ostream& stream, const item::none& ) {
    return stream << "_";
}

std::ostream& operator<<( std::ostream& stream, const item::endrec& ) {
    return stream << "/";
}

std::ostream& operator<<( std::ostream& stream, const item& x ) {
    struct type : boost::static_visitor< const char* > {
        const char* operator()( int ) const                { return "int"; }
        const char* operator()( double ) const             { return "float"; }
        const char* operator()( const std::string& ) const { return "str"; }
        const char* operator()( const item::none ) const   { return "_"; }
        const char* operator()( const item::endrec ) const { return "end"; }
    };

    stream << "{" << boost::apply_visitor( type(), x.val ) << "|";
    if( x.repeat > 1 ) stream << x.repeat;

    return stream << x.val << "}";
}

std::vector< keyword > parse( std::string::const_iterator fst,
                              std::string::const_iterator lst ) {

    using grm = grammar< std::string::const_iterator >;

    grm parser;
    std::vector< lun::keyword > sec;

    auto ok = qi::phrase_parse( fst, lst, parser, skipper< decltype( fst ) >(), sec );
    if( !ok ) std::cerr << "PARSE FAILED" << std::endl;
    return sec;
}

}
