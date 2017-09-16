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

const std::vector< record >& at( const section& sec, const std::string& key ) {
    auto itr = std::find( sec.xs.begin(), sec.xs.end(), key );
    if( itr == sec.xs.end() ) throw std::out_of_range("No such key: " + key );

    return itr->xs;
}

bool has_keyword( const std::vector< keyword >& xs, const std::string& key ) {
    return std::find( xs.begin(), xs.end(), key ) != xs.end();
}

struct size : public boost::static_visitor< size_t > {
    template< typename T >
    size_t operator()( const T& v ) const { return v.size(); }
};

BOOST_AUTO_TEST_CASE(text_after_slash) {
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
    BOOST_CHECK_EQUAL( 3, boost::apply_visitor( size(), dimens.at( 0 ) ) );

}

BOOST_AUTO_TEST_CASE(toggles) {
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

BOOST_AUTO_TEST_CASE(fixed_record_size) {
    const std::string input = R"(
RUNSPEC

OIL -- toggle keyword to make the deck less trivial

DIMENS
10 20 30 /

)";
    auto sec = parse( input.begin(), input.end() );
    const auto& dimens = at( sec, "DIMENS" ).at( 0 );
    int exp[] = { 10, 20, 30 };
    auto& vals = boost::get< const std::vector< int > >( dimens.at( 0 ) );

    BOOST_CHECK_EQUAL( 1, dimens.size() );
    BOOST_CHECK_EQUAL_COLLECTIONS( vals.begin(), vals.end(),
                                   std::begin( exp ), std::end( exp ) );
}

BOOST_AUTO_TEST_CASE( swatinit ) {
    const std::string input = R"(
RUNSPEC

SWATINIT
-- 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 0.25 /
10*0.25 /
)";
    auto sec = parse( input.begin(), input.end() );
    const auto& kw = at( sec, "SWATINIT" ).at( 0 );
    std::vector< double > tgt( 10, 0.25 );
    auto& vals = boost::get< const std::vector< double > >( kw.at( 0 ) );

    BOOST_CHECK_EQUAL( 10, vals.size() );
    BOOST_CHECK_EQUAL_COLLECTIONS( vals.begin(), vals.end(),
                                   tgt.begin(), tgt.end() );
}
