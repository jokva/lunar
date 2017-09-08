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

template< typename... T >
using as_vector = qi::as< std::vector< T... > >;

static const auto empty_records = std::vector< record > {};

#define kword(sym) qi::raw[qi::lexeme[(sym) >> !qi::alnum]]

template< typename Itr, typename Skip, int items, int records = 1 >
struct fixed_record : qi::grammar< Itr, keyword(), Skip > {
    fixed_record() : fixed_record::base_type( start ) {

            start %= kword(sym)
                   > qi::repeat(records)
                        [qi::repeat(items)[qi::int_] >> qi::lit('/')]
                   ;

            qi::on_error< qi::fail >( start, std::cerr
                << phx::val("")
                << "error: expected [" << records << ", " << items << "]"
                << std::endl
            );
        }

    qi::symbols<> sym;
    qi::rule< Itr, keyword(), Skip > start;
};

template< typename Itr >
struct grammar : qi::grammar< Itr, section(), skipper< Itr > > {
    using skip = skipper< Itr >;
    using rule = qi::rule< Itr, keyword(), skip >;

    grammar() : grammar::base_type( start ) {

        toggles  = "OIL", "WATER", "DISGAS", "VAPOIL";
        toggles += "METRIC", "FIELD", "LAB", "NOSIM";

        f13.sym = "DIMENS", "EQLDIMS";

        start %= qi::string("RUNSPEC")
                >> *(
                      f13
                    | kword(toggles) >> qi::attr( empty_records )
                    )
            ;

    }

    qi::symbols<> toggles;
    fixed_record< Itr, skip, 3 > f13;
    qi::rule< Itr, section(), skip > start;
};

section parse( std::string::const_iterator fst,
               std::string::const_iterator lst ) {

    using grm = grammar< std::string::const_iterator >;

    grm parser;
    section sec;

    auto ok = qi::phrase_parse( fst, lst, parser, skipper< decltype( fst ) >(), sec );
    if( !ok ) std::cerr << "PARSE FAILED" << std::endl;
    return sec;
}
