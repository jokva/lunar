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

/*
 * compile-time map some data type to its parser, in order to allow rules to be
 * parametrised over expected return type. Instead of having two rules,
 * int_item and double_item, a template rule item< int|double > can be used
 * instead.
 *
 * Instantiating the value parser in a rule is somewhat tricky:
 *      typename num< T >::p()
 */
template< typename T > struct num;
template<> struct num< int >    { using p = decltype( qi::int_ ); };
template<> struct num< double > { using p = decltype( qi::double_ ); };

/*
 * The basic rule for parsing a non-defaultable item entry, i.e. a list of a
 * single data type, possibly with repetition, into a vector.
 */
template< typename Itr, typename T >
qi::rule< Itr, std::vector< T >(), skipper< Itr > > item =
    +( +qi::lexeme[typename num< T >::p() >> !qi::lit('*')]
      | qi::omit[qi::lexeme[qi::int_ >> '*' >> typename num< T >::p()][
            phx::insert( qi::_val, phx::end(qi::_val),
                                   phx::at_c< 0 >(qi::_1),
                                   phx::at_c< 1 >(qi::_1) )
                ]] >> qi::attr( qi::_val )
    )
;


/*
 * spell out all accepted up/lowcase variations of yes/no, to avoid introducing
 * a new rule just for no-casing
 */
struct yesnoc : public qi::symbols< char, int > {
    yesnoc() {
        add ("'yes'",   1)
            ( "yes",    1)
            ("'y'",     1)
            ( "y",      1)
            ("'YES'",   1)
            ( "YES",    1)
            ("'Y'",     1)
            ( "Y",      1)
            ("'no'",    0)
            ( "no",     0)
            ("'n'",     0)
            ( "n",      0)
            ("'NO'",    0)
            ( "NO",     0)
            ("'N'",     0)
            ( "N",      0)
            ;
    }
} yesno;

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
                      kword(fix13) >> qi::repeat(1)[ item< Itr, int > ]
                    | kword(toggles) >> qi::attr( empty_records )
                    | qi::string("SWATINIT") >> qi::repeat(1)[ item< Itr, double > ]
                    | qi::string("GRIDOPTS")
                        >> qi::repeat(1)[
                        as_vector< int >()[yesno >> *qi::int_ ]]
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
