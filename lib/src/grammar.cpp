#include <string>
#include <vector>

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
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
             | (qi::lit("--") | '/') >> *(qi::char_ - qi::eol) >> qi::eol
             ;
    };

    qi::rule< Itr > skip;
};

template< typename... T >
using as_vector = qi::as< std::vector< T... > >;

static const auto empty_records = std::vector< record > {};

#define kword(sym) qi::raw[qi::lexeme[(sym) >> !qi::alnum]]

namespace bf = boost::fusion;

template< typename Itr >
qi::rule< Itr, std::vector< double >(), skipper< Itr > > ditem =
    +( +qi::lexeme[qi::double_ >> !qi::lit('*')]
      | qi::omit[qi::lexeme[qi::int_ >> '*' >> qi::double_][
            phx::insert( qi::_val, phx::end(qi::_val),
                                   phx::at_c< 0 >(qi::_1),
                                   phx::at_c< 1 >(qi::_1) )
                ]] >> qi::attr( qi::_val )
    )
;

template< typename Itr >
qi::rule< Itr, std::vector< int >(), skipper< Itr > > iitem =
    +( +qi::lexeme[qi::int_ >> !qi::lit('*')]
      | qi::omit[qi::lexeme[qi::int_ >> '*' >> qi::int_][
            phx::insert( qi::_val, phx::end(qi::_val),
                                   phx::at_c< 0 >(qi::_1),
                                   phx::at_c< 1 >(qi::_1) )
                ]] >> qi::attr( qi::_val )
    )
;


template< typename Itr >
struct grammar : qi::grammar< Itr, section(), skipper< Itr > > {
    template< typename T >
    using rule = qi::rule< Itr, T, skipper< Itr > >;

    grammar() : grammar::base_type( start ) {

        toggles  = "OIL", "WATER", "DISGAS", "VAPOIL";
        toggles += "METRIC", "FIELD", "LAB", "NOSIM";
        fix13 = "DIMENS", "EQLDIMS";

        simple %= qi::eps > qi::repeat(qi::_r1)[
                    as_vector< int >()[qi::repeat(qi::_r2)[ qi::int_ ]]
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
                      kword(fix13) >> qi::repeat(1)[ iitem< Itr > ]
                    | kword(toggles) >> qi::attr( empty_records )
                    | qi::string("SWATINIT") >> qi::repeat(1)[ ditem< Itr > ]
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
