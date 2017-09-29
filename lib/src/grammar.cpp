#include <string>
#include <vector>

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#define BOOST_SPIRIT_DEBUG 1

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <lunar/parser.hpp>

namespace spirit    = boost::spirit;
namespace phx       = boost::phoenix;
namespace qi        = boost::spirit::qi;
namespace ascii     = boost::spirit::ascii;
namespace bf        = boost::fusion;

BOOST_FUSION_ADAPT_STRUCT( section, name, xs )
BOOST_FUSION_ADAPT_STRUCT( keyword, name, xs )
BOOST_FUSION_ADAPT_STRUCT( item, repeat, ival, fval, sval )
BOOST_FUSION_ADAPT_STRUCT( item::star, val )

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

static const auto empty_records = std::vector< record > {};

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

/*
 * TODO: optimise backtracking patterns
 */
template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int > =
      qi::int_ >> !qi::lit('*')
    | star< Itr > >> qi::int_
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, double > =
      f77float >> !qi::lit('*')
    | star< Itr > >> qi::attr( 0 ) >> f77float
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, double > =
      primary() >> !qi::lit('*')
    | star< Itr > >> primary()
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, std::string > =
      str< Itr >[qi::_val = qi::_1]
    | star< Itr > >> str< Itr >[qi::_val = qi::_1]
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, std::string > =
      itemrule< Itr, int >
    | itemrule< Itr, std::string >
;

template< typename Itr, typename... T >
qi::rule< Itr, record(), skipper< Itr > > rec =
    *( itemrule< Itr, T... >
     | star< Itr >
     | '*'
     )
    >> term();

template< typename Itr >
struct grammar : qi::grammar< Itr, section(), skipper< Itr > > {
    template< typename T >
    using rule = qi::rule< Itr, T, skipper< Itr > >;

    grammar() : grammar::base_type( start ) {

        toggles =  "OIL", "WATER", "DISGAS", "VAPOIL";
        toggles += "METRIC", "FIELD", "LAB", "NOSIM";
        singlei += "DIMENS", "EQLDIMS";
        singlef += "MAPAXES";

        start %= qi::string("RUNSPEC") >> *(
            kword(singlei) >> qi::repeat(1)[ rec< Itr, int > ]
          | kword(singlef) >> qi::repeat(1)[ rec< Itr, double > ]
          | kword(toggles) >> qi::attr( empty_records )
          | qi::string("GRIDOPTS") >>
                qi::repeat(1)[ rec< Itr, int, std::string > ]
        )
        ;

        start.name( "start" );
        itemrule< Itr >.name( "item" );
        itemrule< Itr, int >.name( "item[int]" );
        itemrule< Itr, double >.name( "item[flt]" );
        itemrule< Itr, int, double >.name( "item[int|flt]" );
        itemrule< Itr, std::string >.name( "item[str]" );
        itemrule< Itr, int, std::string >.name( "item[int|str]" );
    }

    qi::symbols<> singlei;
    qi::symbols<> singlef;
    qi::symbols<> toggles;
    rule< section() > start;
};

}

std::ostream& operator<<( std::ostream& stream, const item::tag& x ) {
    switch( x ) {
        case item::tag::i:   return stream << "int";
        case item::tag::f:   return stream << "float";
        case item::tag::str: return stream << "str";
        default:             return stream << "none";
    }
}

std::ostream& operator<<( std::ostream& stream, const item::star& s ) {
    return stream << int(s) << "*";
}

std::ostream& operator<<( std::ostream& stream, const item& x ) {
    stream << "{" << x.type << "|";
    if( x.repeat > 1 ) stream << x.repeat;

    switch( x.type ) {
        case item::tag::i:   stream << x.ival; break;
        case item::tag::f:   stream << x.fval; break;
        case item::tag::str: stream << x.sval; break;
        default:             stream << "_";
    }

    return stream << "}";
}

section parse( std::string::const_iterator fst,
               std::string::const_iterator lst ) {

    using grm = grammar< std::string::const_iterator >;

    grm parser;
    section sec;

    auto ok = qi::phrase_parse( fst, lst, parser, skipper< decltype( fst ) >(), sec );
    if( !ok ) std::cerr << "PARSE FAILED" << std::endl;
    return sec;
}
