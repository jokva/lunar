#include <string>
#include <vector>

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#define BOOST_SPIRIT_DEBUG 1

#include <boost/fusion/adapted/adt/adapt_adt.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
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
BOOST_FUSION_ADAPT_ADT( item,
        (obj.getint(),    obj.set(val))
        (obj.getdouble(), obj.set(val))
        (obj.getstring(), obj.set(val))
        (obj.getstar(),   obj.set(val))
        (obj.gettag(),    obj.set(val))
)

namespace {

template< typename Itr >
struct skipper : public qi::grammar< Itr > {
    skipper() : skipper::base_type( skip ) {
        skip = ascii::space
             | (qi::lit("--")) >> *(qi::char_ - qi::eol) >> qi::eol
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


//template< typename Itr, typename T >
//qi::rule< Itr, std::vector< T >( int ), skipper< Itr > > bounded =
//    item< Itr, T >[qi::_pass = phx::size( qi::_1 ) < qi::_r1, qi::_val = qi::_1]
//;

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

        syms =  "OIL", "WATER", "DISGAS", "VAPOIL";
        syms += "METRIC", "FIELD", "LAB", "NOSIM";
        syms += "DIMENS";

        start %= qi::string("RUNSPEC")
            >> *( kword(syms) >> qi::repeat(1)[ *itemrule< Itr > >> qi::eol ] )
        ;

    //    qi::on_error< qi::fail >( simple, std::cerr << phx::val("")
    //            << "error: expected ["
    //            << qi::_r1 << ", " << qi::_r2
    //            << "] at:"
    //            << std::endl
    //            << phx::construct< std::string >( qi::_3, qi::_2 )
    //            << std::endl
    //    );

    //    start %= qi::string("RUNSPEC")
    //            >> *(
    //                  kword(fix13) >> qi::repeat(1)[ item< Itr, int > ]
    //                | kword(toggles) >> qi::attr( empty_records )
    //                | qi::string("EQLDIMS") >> qi::repeat(1)[bounded< Itr, int >( 5 )]
    //                | qi::string("GRIDOPTS") >> qi::repeat(1)[
    //                    as_vector< int >()[yesno >> *qi::int_ ]]
    //                )
    //            ;
        start.name( "start" );
        itemrule< Itr >.name( "item" );
    }

    qi::symbols<> syms;
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
