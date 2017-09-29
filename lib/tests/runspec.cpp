#define BOOST_TEST_MODULE runspec

#include <string>

#include <boost/test/unit_test.hpp>

#include <lunar/parser.hpp>

bool operator==( const std::string& key, const keyword& kw ) {
    return key == kw.name;
}

bool operator==( const keyword& kw, const std::string& key ) {
        return key == kw;
}

bool operator==( const item& x, int i ) {
    return x.type == item::tag::i && x.ival == i;
}

bool operator==( const item& x, double f ) {
    return x.type == item::tag::f && x.fval == f;
}

bool operator==( const item& x, const std::string& str ) {
    return x.type == item::tag::str && x.sval == str;
}

template< typename T >
bool operator==( const T& lhs, const item& rhs ) { return rhs == lhs; }
template< typename T >
bool operator!=( const T& lhs, const item& rhs ) { return !(rhs == lhs); }
template< typename T >
bool operator!=( const item& lhs, const T& rhs ) { return !(lhs == rhs); }

const std::vector< record >& at( const section& sec, const std::string& key ) {
    auto itr = std::find( sec.xs.begin(), sec.xs.end(), key );
    if( itr == sec.xs.end() ) throw std::out_of_range("No such key: " + key );

    return itr->xs;
}

bool has_keyword( const std::vector< keyword >& xs, const std::string& key ) {
    return std::find( xs.begin(), xs.end(), key ) != xs.end();
}

BOOST_AUTO_TEST_CASE( text_after_slash ) {
    const std::string input = R"(
RUNSPEC

DIMENS
    10 20 30 / text-after-slash

OIL
)";
}

BOOST_AUTO_TEST_CASE( no_space_before_slash ) {
    const std::string input = R"(
RUNSPEC

DIMENS
    10/
OIL
)";

    auto sec = parse( input.begin(), input.end() );
    BOOST_CHECK_EQUAL( "RUNSPEC", sec.name );
    BOOST_CHECK( has_keyword( sec.xs, "DIMENS" ) );
    BOOST_CHECK( has_keyword( sec.xs, "OIL" ) );
    auto& dimens = at( sec, "DIMENS" ).at( 0 );

    for( const auto& item : dimens )
        BOOST_CHECK_EQUAL( item.type, item::tag::i );
}

BOOST_AUTO_TEST_CASE( slash_different_line ) {
    const std::string input = R"(
RUNSPEC

DIMENS
    10
/
OIL
)";

    auto sec = parse( input.begin(), input.end() );
    BOOST_CHECK_EQUAL( "RUNSPEC", sec.name );
    BOOST_CHECK( has_keyword( sec.xs, "DIMENS" ) );
    BOOST_CHECK( has_keyword( sec.xs, "OIL" ) );
    auto& dimens = at( sec, "DIMENS" ).at( 0 );

    for( const auto& item : dimens )
        BOOST_CHECK_EQUAL( item.type, item::tag::i );
}

BOOST_AUTO_TEST_CASE( toggles ) {
    const std::string input = R"(
RUNSPEC

WATER
OIL
)";
    auto sec = parse( input.begin(), input.end() );
    BOOST_CHECK_EQUAL( "RUNSPEC", sec.name );
    BOOST_CHECK( has_keyword( sec.xs, "WATER" ) );
    BOOST_CHECK( has_keyword( sec.xs, "OIL" ) );
}

BOOST_AUTO_TEST_CASE( record_all_int ) {
    const std::string input = R"(
RUNSPEC

OIL -- toggle keyword to make the deck less trivial

DIMENS
    10 20 30 /

WATER
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& dimens = at( sec, "DIMENS" ).at( 0 );
    int exp[] = { 10, 20, 30 };

    BOOST_CHECK_EQUAL_COLLECTIONS( std::begin( dimens ), std::end( dimens ),
                                   std::begin( exp ), std::end( exp ) );
}

BOOST_AUTO_TEST_CASE( repeat_int ) {
    const std::string input = R"(
RUNSPEC

EQLDIMS
    3*5 /
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "EQLDIMS" ).at( 0 );

    BOOST_CHECK_EQUAL( 1U, kw.size() );

    const auto& x = kw.at( 0 );
    BOOST_CHECK_EQUAL( 3, x.repeat );
    BOOST_CHECK_EQUAL( item::tag::i, x.type );
    BOOST_CHECK_EQUAL( 5, x.ival );
}

BOOST_AUTO_TEST_CASE( repeat_int_mixed ) {
    const std::string input = R"(
RUNSPEC

DIMENS
    5 2*10 /
)";

    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "DIMENS" ).at( 0 );
    BOOST_CHECK_EQUAL( 2U, kw.size() );

    const auto& x1 = kw.at( 0 );
    BOOST_CHECK_EQUAL( 1, x1.repeat );
    BOOST_CHECK_EQUAL( item::tag::i, x1.type );
    BOOST_CHECK_EQUAL( 5, x1.ival );

    const auto& x2 = kw.at( 1 );
    BOOST_CHECK_EQUAL( 2, x2.repeat );
    BOOST_CHECK_EQUAL( item::tag::i, x2.type );
    BOOST_CHECK_EQUAL( 10, x2.ival );
}

BOOST_AUTO_TEST_CASE( repeat_float ) {
    const std::string input = R"(
RUNSPEC

MAPAXES
    3*100. 2*13.1 4*.3 /
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "MAPAXES" ).at( 0 );

    const auto& x1 = kw.at( 0 );
    BOOST_CHECK_EQUAL( 3, x1.repeat );
    BOOST_CHECK_EQUAL( item::tag::f, x1.type );
    BOOST_CHECK_CLOSE( 100.0, x1.fval, 1e-5 );

    const auto& x2 = kw.at( 1 );
    BOOST_CHECK_EQUAL( 2, x2.repeat );
    BOOST_CHECK_EQUAL( item::tag::f, x2.type );
    BOOST_CHECK_CLOSE( 13.1, x2.fval, 1e-5 );

    const auto& x3 = kw.at( 2 );
    BOOST_CHECK_EQUAL( 4, x3.repeat );
    BOOST_CHECK_EQUAL( item::tag::f, x3.type );
    BOOST_CHECK_CLOSE( .3, x3.fval, 1e-5 );
}

BOOST_AUTO_TEST_CASE( repeat_float_mixed ) {
    const std::string input = R"(
RUNSPEC

MAPAXES
    1.2 2*2.4 .8 /
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "MAPAXES" ).at( 0 );

    const auto& x1 = kw.at( 0 );
    BOOST_CHECK_EQUAL( 1, x1.repeat );
    BOOST_CHECK_EQUAL( item::tag::f, x1.type );
    BOOST_CHECK_CLOSE( 1.2, x1.fval, 1e-5 );

    const auto& x2 = kw.at( 1 );
    BOOST_CHECK_EQUAL( 2, x2.repeat );
    BOOST_CHECK_EQUAL( item::tag::f, x2.type );
    BOOST_CHECK_CLOSE( 2.4, x2.fval, 1e-5 );

    const auto& x3 = kw.at( 2 );
    BOOST_CHECK_EQUAL( 1, x3.repeat );
    BOOST_CHECK_EQUAL( item::tag::f, x3.type );
    BOOST_CHECK_CLOSE( .8, x3.fval, 1e-5 );
}

BOOST_AUTO_TEST_CASE( float_without_exponent ) {
    const std::string input = R"(
RUNSPEC

MAPAXES
    .5 0.5 0.500 50. /
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "MAPAXES" ).at( 0 );

    for( int i = 0; i < 3; ++i ) {
        const auto& x = kw.at( 0 );
        BOOST_CHECK_EQUAL( 1, x.repeat );
        BOOST_CHECK_EQUAL( item::tag::f, x.type );
        BOOST_CHECK_CLOSE( 0.5, x.fval, 1e-5 );
    }

    const auto& x = kw.at( 3 );
    BOOST_CHECK_EQUAL( 1, x.repeat );
    BOOST_CHECK_EQUAL( item::tag::f, x.type );
    BOOST_CHECK_CLOSE( 50, x.fval, 1e-5 );
}

BOOST_AUTO_TEST_CASE( negative_float_without_exponent ) {
    const std::string input = R"(
RUNSPEC

MAPAXES
    -.5 -0.5 -0.500 /
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "MAPAXES" ).at( 0 );

    for( int i = 0; i < 3; ++i ) {
        const auto& x = kw.at( 0 );
        BOOST_CHECK_EQUAL( 1, x.repeat );
        BOOST_CHECK_EQUAL( item::tag::f, x.type );
        BOOST_CHECK_CLOSE( -0.5, x.fval, 1e-5 );
    }
}

BOOST_AUTO_TEST_CASE( float_with_exponent ) {
    const std::string input = R"(
RUNSPEC

MAPAXES
    .5e2 0.5e2 0.500e2
    .5E2 0.5E2 0.500E2
    .5d2 0.5d2 0.500d2
    .5D2 0.5D2 0.500D2
/
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "MAPAXES" ).at( 0 );

    for( int i = 0; i < 6; ++i ) {
        const auto& x = kw.at( 0 );
        BOOST_CHECK_EQUAL( 1, x.repeat );
        BOOST_CHECK_EQUAL( item::tag::f, x.type );
        BOOST_CHECK_CLOSE( 0.5e2, x.fval, 1e-5 );
    }
}

BOOST_AUTO_TEST_CASE( negative_float_with_exponent ) {
    const std::string input = R"(
RUNSPEC

MAPAXES
    -.5e2 -0.5e2 -0.500e2
    -.5E2 -0.5E2 -0.500E2
    -.5d2 -0.5d2 -0.500d2
    -.5D2 -0.5D2 -0.500D2
/
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "MAPAXES" ).at( 0 );

    for( int i = 0; i < 6; ++i ) {
        const auto& x = kw.at( 0 );
        BOOST_CHECK_EQUAL( 1, x.repeat );
        BOOST_CHECK_EQUAL( item::tag::f, x.type );
        BOOST_CHECK_CLOSE( -0.5e2, x.fval, 1e-5 );
    }
}

BOOST_AUTO_TEST_CASE( float_with_negative_exponent ) {
    const std::string input = R"(
RUNSPEC

MAPAXES
    .5e-2 0.5e-2 0.500e-2
    .5E-2 0.5E-2 0.500E-2
    .5d-2 0.5d-2 0.500d-2
    .5D-2 0.5D-2 0.500D-2
/
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "MAPAXES" ).at( 0 );

    for( int i = 0; i < 6; ++i ) {
        const auto& x = kw.at( 0 );
        BOOST_CHECK_EQUAL( 1, x.repeat );
        BOOST_CHECK_EQUAL( item::tag::f, x.type );
        BOOST_CHECK_CLOSE( 0.5e-2, x.fval, 1e-5 );
    }
}

BOOST_AUTO_TEST_CASE( boolean_string_yes ) {
    const std::string inputs[] = {
R"(
RUNSPEC
GRIDOPTS
    1*'YES' 0 /
)",

R"(
RUNSPEC
GRIDOPTS
    'Y' 0 /
)",

R"(
RUNSPEC
GRIDOPTS
    YES 0 0 /
)",

R"(
RUNSPEC
GRIDOPTS
    Y 0 /
)",

R"(
RUNSPEC
GRIDOPTS
    'yes' 0 /
)",

R"(
RUNSPEC
GRIDOPTS
    'y' 0 /
)",

R"(
RUNSPEC
GRIDOPTS
    yes 0 /
)",

R"(
RUNSPEC
GRIDOPTS
    y 0 /
)"

};

    const std::string results[] = { "YES", "Y", "YES", "Y",
                                    "yes", "y", "yes", "y" };

    auto isize = std::distance( std::begin( inputs ), std::end( inputs ) );
    auto rsize = std::distance( std::begin( results ), std::end( results ) );
    BOOST_CHECK_EQUAL( isize, rsize );

    int i = 0;
    for( const auto& input : inputs ) {
        const auto& result = results[ i++ ];

        auto sec = parse( input.begin(), input.end() );
        const auto& kw = at( sec, "GRIDOPTS" ).at( 0 );

        const auto& x1 = kw.at( 0 );
        BOOST_CHECK_EQUAL( item::tag::str, x1.type );
        BOOST_CHECK_EQUAL( x1.sval, result );

        const auto& x2 = kw.at( 1 );
        BOOST_CHECK_EQUAL( item::tag::i, x2.type );
        BOOST_CHECK_EQUAL( x2.ival, 0 );
    }
}
