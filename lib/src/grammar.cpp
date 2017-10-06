#include <string>
#include <vector>

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#define BOOST_SPIRIT_DEBUG 1

#include <boost/fusion/adapted/adt/adapt_adt.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/spirit/include/support_adapt_adt_attributes.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_kwd.hpp>

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

template< typename... T >
using as_vector = qi::as< std::vector< T... > >;

using as_star = qi::as< item::star >;

static const auto empty_records = std::vector< record > {};

#define kword(sym) qi::raw[qi::lexeme[(sym) >> !qi::alnum]]

template< typename Itr >
qi::rule< Itr, std::string(), skipper< Itr > > quoted_string =
      qi::lexeme[ '\'' >> *(qi::char_ - '\'') >> '\'' ]
    | qi::lexeme[ '"'  >> *(qi::char_ - '"')  >> '"'  ]
    | qi::lexeme[ qi::char_("a-zA-Z") >> *qi::alnum ]
;

/* a typical deck contains a LOT more int-values than doubles, which means that
 * by checking for doubles first, we must essentially always backtrack, which
 * is somewhat expensive.
 *
 * TODO: figure out rule that allows valid ints to parse first that isn't
 * horribly complicated or expensive
 */

#define primary() (qi::lexeme[qi::int_ >> !qi::char_(".eEdD")] | qi::double_)
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
 */
template< typename Itr, typename... >
qi::rule< Itr, item(), skipper< Itr > > itemrule;

/*
 * TODO: optimise backtracking patterns
 */
template< typename Itr >
qi::rule< Itr, item(), skipper< Itr > > itemrule< Itr, int > =
      qi::lexeme[qi::int_ >> !qi::lit('*')]
    | qi::lexeme[as_star()[qi::int_] >> '*'  >> qi::int_]
;

template< typename Itr >
qi::rule< Itr, item(), skipper< Itr > > itemrule< Itr, double > =
      qi::lexeme[qi::double_ >> !qi::lit('*')]
    | qi::lexeme[as_star()[qi::int_] >> '*'  >> qi::double_]
;

template< typename Itr >
qi::rule< Itr, item(), skipper< Itr > > itemrule< Itr, int, double > =
      qi::lexeme[primary() >> !qi::lit('*')]
    | qi::lexeme[as_star()[qi::int_] >> '*'  >> primary()]
;

template< typename Itr >
qi::rule< Itr, item(), skipper< Itr > > itemrule< Itr, std::string > =
      qi::lexeme[quoted_string< Itr > >> !qi::lit('*')]
    | qi::lexeme[as_star()[qi::int_] >> '*' >> quoted_string< Itr >]
;

template< typename Itr, typename... T >
qi::rule< Itr, record(), skipper< Itr > > rec =
       *(itemrule< Itr, T... > | qi::lexeme[as_star()[qi::int_] >> '*'] | '*')
    >> term();

template< typename Itr >
struct grammar : qi::grammar< Itr, section(), skipper< Itr > > {
    template< typename T >
    using rule = qi::rule< Itr, T, skipper< Itr > >;

    grammar() : grammar::base_type( start ) {

        toggles =  "OIL", "WATER", "DISGAS", "VAPOIL";
        toggles += "METRIC", "FIELD", "LAB", "NOSIM";
        singlei += "DIMENS", "EQLDIMS";
        singlef += "SWATINIT";

        start %= qi::string("RUNSPEC") >> *(
            kword(singlei) >> qi::repeat(1)[ rec< Itr, int > ]
          | kword(singlef) >> qi::repeat(1)[ rec< Itr, double > ]
          | kword(toggles) >> qi::attr( empty_records )
        )
        ;

        start.name( "start" );
        itemrule< Itr >.name( "item" );
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
