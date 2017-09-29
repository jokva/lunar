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

BOOST_AUTO_TEST_CASE( fixed_record_size ) {
    const std::string input = R"(
RUNSPEC

OIL -- toggle keyword to make the deck less trivial

DIMENS
    10 20 30 /

)";
    auto sec = parse( input.begin(), input.end() );
    const auto& dimens = at( sec, "DIMENS" ).at( 0 );
    std::cout << " ==== " << std::endl;
    for( const auto& x : dimens ) std::cout << x  << std::endl;
    std::cout << " ==== " << std::endl;
    int exp[] = { 10, 20, 30 };

    BOOST_CHECK_EQUAL_COLLECTIONS( std::begin( dimens ), std::end( dimens ),
                                   std::begin( exp ), std::end( exp ) );
}

// BOOST_AUTO_TEST_CASE( repeated_value ) {
//     const std::string input = R"(
// RUNSPEC
//
// EQLDIMS
//     3*5 /
// )";
//     auto sec = parse( input.begin(), input.end() );
//     const auto& kw = at( sec, "EQLDIMS" ).at( 0 );
//     std::vector< double > tgt( 3 , 5 );
//     auto& vals = boost::get< const std::vector< int > >( kw.at( 0 ) );
//
//     BOOST_CHECK_EQUAL_COLLECTIONS( vals.begin(), vals.end(),
//                                    tgt.begin(), tgt.end() );
// }
//
// BOOST_AUTO_TEST_CASE( too_many_items_fails ) {
//     const std::string input = R"(
// RUNSPEC
//
// EQLDIMS
//     10*5 /
// )";
//     auto sec = parse( input.begin(), input.end() );
//     BOOST_CHECK_THROW( at( sec, "EQLDIMS" ).at( 0 ), std::out_of_range );
// }
//
// BOOST_AUTO_TEST_CASE( repeat_int ) {
//     const std::string input = R"(
// RUNSPEC
//
// DIMENS
//     3*10 /
// )";
//     auto sec = parse( input.begin(), input.end() );
//     const auto& dimens = at( sec, "DIMENS" ).at( 0 );
//     std::vector< int > tgt( 3, 10 );
//     auto& vals = boost::get< std::vector< int > >( dimens.at( 0 ) );
//     BOOST_CHECK_EQUAL_COLLECTIONS( vals.begin(), vals.end(),
//                                    tgt.begin(), tgt.end() );
// }
//
// BOOST_AUTO_TEST_CASE( repeat_int_mixed ) {
//     const std::string input = R"(
// RUNSPEC
//
// DIMENS
//     5 2*10 /
// )";
//     auto sec = parse( input.begin(), input.end() );
//     const auto& dimens = at( sec, "DIMENS" ).at( 0 );
//     std::vector< int > tgt = { 5, 10, 10 };
//     auto& vals = boost::get< std::vector< int > >( dimens.at( 0 ) );
//     BOOST_CHECK_EQUAL_COLLECTIONS( vals.begin(), vals.end(),
//                                    tgt.begin(), tgt.end() );
// }
//
// BOOST_AUTO_TEST_CASE( boolean_string_yes ) {
//     const std::string inputs[] = {
// R"(
// RUNSPEC
// GRIDOPTS
//     'YES' 0 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     'Y' 0 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     YES 0 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     Y 0 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     'yes' 0 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     'y' 0 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     yes 0 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     y 0 /
// )"
//
// };
//     for( const auto& input : inputs ) {
//         auto sec = parse( input.begin(), input.end() );
//         const auto& dimens = at( sec, "GRIDOPTS" ).at( 0 );
//         std::vector< int > tgt = { 1, 0 };
//         auto& vals = boost::get< std::vector< int > >( dimens.at( 0 ) );
//         BOOST_CHECK_EQUAL_COLLECTIONS( vals.begin(), vals.end(),
//                                        tgt.begin(), tgt.end() );
//     }
// }
//
// BOOST_AUTO_TEST_CASE( boolean_string_no ) {
//     const std::string inputs[] = {
// R"(
// RUNSPEC
// GRIDOPTS
//     'NO' 1 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     'N' 1 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     NO 1 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     N 1 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     'n' 1 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     'n' 1 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     no 1 /
// )",
//
// R"(
// RUNSPEC
// GRIDOPTS
//     n 1 /
// )"
//
// };
//
//     for( const auto& input : inputs ) {
//         auto sec = parse( input.begin(), input.end() );
//         const auto& dimens = at( sec, "GRIDOPTS" ).at( 0 );
//         std::vector< int > tgt = { 0, 1 };
//         auto& vals = boost::get< std::vector< int > >( dimens.at( 0 ) );
//         BOOST_CHECK_EQUAL_COLLECTIONS( vals.begin(), vals.end(),
//                                        tgt.begin(), tgt.end() );
//     }
// }
