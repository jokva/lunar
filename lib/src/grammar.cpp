#include <string>
#include <vector>

#define BOOST_SPIRIT_DEBUG 1

#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_kwd.hpp>


#include <lunar/parser.hpp>

namespace spirit    = boost::spirit;
namespace phx       = boost::phoenix;
namespace qi        = boost::spirit::qi;
namespace ascii     = boost::spirit::ascii;

BOOST_FUSION_ADAPT_STRUCT( section, name, xs )
BOOST_FUSION_ADAPT_STRUCT( keyword, name, xs )

namespace {

template< typename Itr >
struct skipper : public qi::grammar< Itr > {
    skipper() : skipper::base_type( skip ) {
        skip = ascii::space
             | (qi::lit("--") >> *(qi::char_ - qi::eol) >> qi::eol)
             ;
    };

    qi::rule< Itr > skip;
};

template< typename... T >
using as_vector = qi::as< std::vector< T... > >;

static const auto empty_records = std::vector< record > {};

#define kword(sym) qi::raw[qi::lexeme[(sym) >> !qi::alnum]]

template< typename Itr >
struct grammar : qi::grammar< Itr, section(), skipper< Itr > > {
    template< typename T >
    using rule = qi::rule< Itr, T, skipper< Itr > >;

    grammar() : grammar::base_type( start ) {

        toggles  = "OIL", "WATER", "DISGAS", "VAPOIL";
        toggles += "METRIC", "FIELD", "LAB", "NOSIM";
        fix13 = "DIMENS", "EQLDIMS";

        simple %= qi::eps > qi::repeat(qi::_r1)[
                    qi::repeat(qi::_r2)[ qi::int_ ] >> qi::lit('/')
                ];

        qi::on_error< qi::fail >( simple, std::cerr << phx::val("")
                << "error: expected ["
                << qi::_r1 << ", " << qi::_r2
                << "] at:"
                << std::endl
                << phx::construct< std::string >( qi::_3, qi::_2 )
                << std::endl
        );


        start %= qi::string("RUNSPEC")
                >> *(
                      kword(fix13) >> simple(1, 3)
                    | kword(toggles) >> qi::attr( empty_records )
                    )
            ;

    }

    qi::symbols<> toggles;
    qi::symbols<> fix13;
    rule< std::vector< record >(int, int) > simple;
    rule< section() > start;
};

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
