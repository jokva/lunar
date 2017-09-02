#include <string>
#include <vector>

#define BOOST_SPIRIT_DEBUG 1

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <lunar/parser.hpp>

namespace spirit    = boost::spirit;
namespace phx       = boost::phoenix;
namespace qi        = boost::spirit::qi;
namespace ascii     = boost::spirit::ascii;

BOOST_FUSION_ADAPT_STRUCT(
    section,
    (std::string, name),
    (std::vector< keyword >, xs)
)

BOOST_FUSION_ADAPT_STRUCT(
    keyword,
    (std::string, name),
    (std::vector< record >, xs)
)

template< typename Itr >
struct skipper : public qi::grammar< Itr > {
    skipper() : skipper::base_type( skip ) {
        skip = ascii::space
             | (qi::lit("--") >> *(qi::char_ - qi::eol) >> qi::eol)
             ;
    };

    qi::rule< Itr > skip;
};

constexpr static const auto blank = "\n\t ";

template< typename... T >
using as_vector = qi::as< std::vector< T... > >;

static const auto empty_records = std::vector< record > {};

template< typename Itr >
struct grammar : qi::grammar< Itr, section(), skipper< Itr > > {
    using skip = skipper< Itr >;

    grammar() : grammar::base_type( start ) {

        dimens  = nodefault( "DIMENS", 3 );
        eqldims = nodefault( "EQLDIMS", 3 );
        oil     = toggle( "OIL" );
        water   = toggle( "WATER" );
        gas     = toggle( "GAS" );
        disgas  = toggle( "DISGAS" );
        vapoil  = toggle( "VAPOIL" );
        metric  = toggle( "METRIC" );
        field   = toggle( "FIELD" );
        nosim   = toggle( "NOSIM" );

        start %= qi::string("RUNSPEC")
                >> *( dimens
                    | eqldims
                    | oil
                    | water
                    | gas
                    | disgas
                    | vapoil
                    | metric
                    | field
                    | nosim
                    )
            ;
    }

    using rule = qi::rule< Itr, keyword(), skip >;

    rule dimens, eqldims;
    rule oil, water, gas, disgas, vapoil;
    rule metric, field, nosim;
    qi::rule< Itr, section(),       skip > start;

    private:

    static rule nodefault( std::string kw ,
                           int items,
                           int records = 1 ) {
        return qi::string( kw ) >>
                qi::repeat(records)[
                    qi::repeat(items)[qi::int_] > qi::lit('/')
                ]
           ;
    }

    static rule toggle( std::string kw ) {
        return qi::string( kw ) >> qi::attr( empty_records );
    }
};

section parse( std::string::const_iterator fst,
               std::string::const_iterator lst ) {

    using grm = grammar< std::string::const_iterator >;

    grm parser;
    section sec;

    qi::phrase_parse( fst, lst, parser, skipper< decltype( fst ) >(), sec );
    return sec;
}
