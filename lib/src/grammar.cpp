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
BOOST_FUSION_ADAPT_STRUCT( star, val )
//BOOST_FUSION_ADAPT_ADT( item,
//        (obj.ival,      obj.set(val))
//        (obj.fval,      obj.set(val))
//        (obj.sval,      obj.set(val))
//        (obj.repeat,    obj.set(val))
//)

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
 * TODO: optimise backtracking patterns
 */
template< typename Itr >
qi::rule< Itr, item(), skipper< Itr > > itemrule = (
      qi::lexeme[primary() >> !qi::lit('*')]
    | qi::lexeme[qi::as< star >()[qi::int_] >> '*'  >> primary()]
    | qi::lexeme[qi::as< star >()[qi::int_] >> '*']
    | '*'
    | quoted_string< Itr >
    )
;

template< typename Itr >
qi::rule< Itr, record(), skipper< Itr > > rec =
    *itemrule< Itr > >> term();

template< typename Itr >
struct grammar : qi::grammar< Itr, section(), skipper< Itr > > {
    template< typename T >
    using rule = qi::rule< Itr, T, skipper< Itr > >;

    grammar() : grammar::base_type( start ) {

        toggles =  "OIL", "WATER", "DISGAS", "VAPOIL";
        toggles += "METRIC", "FIELD", "LAB", "NOSIM";
        syms += "DIMENS", "EQLDIMS";

        start %= qi::string("RUNSPEC") >> *(
            //kword(syms) >> qi::repeat(1)[ (*itemrule< Itr >) > term()]
            kword(syms) >> qi::repeat(1)[ rec< Itr > ]
          | kword(toggles) >> qi::attr( empty_records )
        )
        ;

        start.name( "start" );
        itemrule< Itr >.name( "item" );
    }

    qi::symbols<> syms;
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

std::ostream& operator<<( std::ostream& stream, const star& s ) {
    return stream << int(s) << "*";
}

std::ostream& operator<<( std::ostream& stream, const item& x ) {
    stream << "{" << x.type << "|";
    if( x.repeat > 1 )
        stream << x.repeat << "*";

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
