#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <lunar/parser.hpp>

#include <catch/catch.hpp>

namespace Catch {

template<>
struct StringMaker< lun::keyword > {
    static std::string convert( const lun::keyword& kw ) {
        return kw.name;
    }
};

}

namespace {

class HasKeyword : public Catch::MatcherBase< std::vector< lun::keyword > > {
    public:
        HasKeyword( std::string str ) : name( std::move( str ) ) {}

        bool match( const std::vector< lun::keyword >& kws ) const override {
            for( const auto& kw : kws )
                if( kw.name == this->name ) return true;

            return false;
        }

        virtual std::string describe() const override {
            return "contains " + this->name;
        }

    private:
        std::string name;
};

struct typedescr {
    std::string operator()( int ) const               { return "int"; }
    std::string operator()( double ) const            { return "float"; }
    std::string operator()( std::string ) const       { return "string"; }
    std::string operator()( lun::item::none ) const   { return "none"; }
    std::string operator()( lun::item::endrec ) const { return "end"; }
};

template< typename T >
struct istype : boost::static_visitor< bool > {
    template< typename U >
    bool operator()( U ) const { return std::is_same< T, U >::value; }
};

template< typename T >
class IsType : public Catch::MatcherBase< lun::item > {
    public:

        bool match( const lun::item& x ) const override {
            return boost::apply_visitor( istype< T >(), x.val );
        }

        virtual std::string describe() const override {
            return "is of type " + typedescr()( T() );
        }
};

auto IsInt()    -> IsType< int >         { return IsType< int >(); }
auto IsFloat()  -> IsType< double >      { return IsType< double >(); }
auto IsString() -> IsType< std::string > { return IsType< std::string >(); }
auto IsEnd ()   -> IsType< lun::item::endrec > {
    return IsType< lun::item::endrec >();
}

class Repeats : public Catch::MatcherBase< lun::item > {
    public:
        Repeats( int n ) : repeats( n ) {}

        bool match( const lun::item& x ) const override {
            return x.repeat == this->repeats;
        }

        virtual std::string describe() const override {
            return "repeats " + std::to_string( this->repeats ) + " times";
        }

    private:
        int repeats;
};

const lun::keyword& get( const std::string& key,
                         const std::vector< lun::keyword >& kws ) {
    auto itr = std::find_if( kws.begin(), kws.end(),
                    [&]( const lun::keyword& x ) { return key == x.name; }
             );

    if( itr == kws.end() ) throw std::out_of_range( "Key not found: " + key );

    return *itr;
}

/*
 * implement operator== for item so that tests can be written as item == 10
 * (int), item == "STRING" or item == Approx(1.5)
 */

template< typename T >
bool operator==( const lun::item& lhs, T rhs ) {
    return boost::get< T >( lhs.val ) == rhs;
}

bool operator==( const lun::item& lhs, const Approx& rhs ) {
    return boost::get< double >( lhs.val ) == rhs;
}

bool operator==( const lun::item& lhs, const char* rhs ) {
    return lhs == std::string( rhs );
}

}

TEST_CASE( "slash terminates record", "[slash]") {
    const std::string input = R"(
RUNSPEC

DIMENS
    10 20 30 / text-after-slash

GAS

EQLDIMS
    2/

REGDIMS
    10
/

OIL
)";

    auto sec = lun::parse( input );
    REQUIRE( sec.front().name == "RUNSPEC" );

    SECTION( "toggles have no items" ) {
        CHECK_THAT( sec, HasKeyword( "OIL" ) );
        const auto& oil = get( "OIL", sec );
        CHECK( oil.xs.empty() );
    }

    SECTION( "integers are recognised" ) {
        REQUIRE_THAT( sec, HasKeyword( "DIMENS" ) );
        const auto& kw = get( "DIMENS", sec );

        CHECK_THAT( kw.xs.back(), IsEnd() );

        std::for_each( kw.xs.begin(), kw.xs.end() - 1,
            []( auto& item ) { CHECK_THAT( item, IsInt() ); } );
    }

    SECTION( "/ following int does not change the value" ) {
        REQUIRE_THAT( sec, HasKeyword( "EQLDIMS" ) );
        const auto& eqldims = get( "EQLDIMS", sec ).xs;
        REQUIRE( eqldims.size() == 2 );

        const auto& x = eqldims.front();
        CHECK_THAT( x, IsInt() );
        CHECK( x == 2 );
    }

    SECTION( "/ on new line does not change the value" ) {
        REQUIRE_THAT( sec, HasKeyword( "REGDIMS" ) );
        const auto& kw = get( "REGDIMS", sec ).xs;
        REQUIRE( kw.size() == 2 );

        auto& x = kw.front();
        CHECK_THAT( x, IsInt() );
        CHECK( x == 10 );
    }
}

TEST_CASE( "ints with repetition", "[repeat]") {
    const std::string input = R"(
RUNSPEC

EQLDIMS
    3*5 /

DIMENS
    5 2*10 /

)";

    auto sec = lun::parse( input );
    REQUIRE( sec.front().name == "RUNSPEC" );

    SECTION( "a single int" ) {
        REQUIRE_THAT( sec, HasKeyword( "EQLDIMS" ) );

        const auto& kw = get( "EQLDIMS", sec ).xs;
        REQUIRE( kw.size() == 2 );

        const auto& x = kw.front();
        CHECK_THAT( x, Repeats( 3 ) );
        CHECK_THAT( x, IsInt() );
        CHECK( x == 5 );
    }

    SECTION( "int, repeated int" ) {
        REQUIRE_THAT( sec, HasKeyword( "DIMENS" ) );

        const auto& kw = get( "DIMENS", sec ).xs;
        REQUIRE( kw.size() == 3 );

        SECTION( "the single integer" ) {
            const auto& x = kw.at( 0 );
            REQUIRE_THAT( x, IsInt() );
            CHECK( x == 5 );
        }

        SECTION( "the repeated integer" ) {
            const auto& x = kw.at( 1 );
            REQUIRE_THAT( x, IsInt() );
            CHECK_THAT( x, Repeats( 2 ) );
            CHECK( x == 10 );
        }
    }
}

TEST_CASE( "floating point numbers", "[float]") {
    const std::string input = R"(
GRID

MAPAXES
    3*100. 2*13.1 4*.3 /

MAPAXES
    1.2 2*2.4 .8 8.0 8. /

MAPAXES
    .5 0.5 0.500 /

MAPAXES
    -.5 -0.5 -0.500 /

MAPAXES
    .5e2 0.5e2 0.500e2
    .5E2 0.5E2 0.500E2
    .5d2 0.5d2 0.500d2
    .5D2 0.5D2 0.500D2
/

MAPAXES
    -.5e2 -0.5e2 -0.500e2
    -.5E2 -0.5E2 -0.500E2
    -.5d2 -0.5d2 -0.500d2
    -.5D2 -0.5D2 -0.500D2
/

MAPAXES
    .5e-2 0.5e-2 0.500e-2
    .5E-2 0.5E-2 0.500E-2
    .5d-2 0.5d-2 0.500d-2
    .5D-2 0.5D-2 0.500D-2
/

)";
    auto sec = lun::parse( input );
    REQUIRE( sec.front().name == "GRID" );
    REQUIRE( sec.size() == 8 );

    SECTION( "repeating floats" ) {
        const auto& kw = sec.at( 1 ).xs;

        REQUIRE( kw.size() == 4 );

        SECTION( "3*100." ) {
            const auto& item = kw[ 0 ];
            CHECK_THAT( item, Repeats( 3 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( 100 ) );
        }

        SECTION( "2*13.1" ) {
            const auto& item = kw[ 1 ];
            CHECK_THAT( item, Repeats( 2 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( 13.1 ) );
        }

        SECTION( "4*.3" ) {
            const auto& item = kw[ 2 ];
            CHECK_THAT( item, Repeats( 4 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( .3 ) );
        }
    }

    SECTION( "mix repeated and non-repeated" ) {
        const auto& kw = sec.at( 2 ).xs;

        REQUIRE( kw.size() == 6 );

        SECTION( "1.2" ) {
            const auto& item = kw[ 0 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( 1.2 ) );
        }

        SECTION( "2*2.4" ) {
            const auto& item = kw[ 1 ];
            CHECK_THAT( item, Repeats( 2 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( 2.4 ) );
        }

        SECTION( ".8" ) {
            const auto& item = kw[ 2 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( 0.8 ) );
        }

        SECTION( "8.0" ) {
            const auto& item = kw[ 3 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( 8 ) );
        }

        SECTION( "8." ) {
            const auto& item = kw[ 4 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item == Approx( 8 ) );
        }
    }

    SECTION( "can be written without exponent" ) {
        const auto& xs = sec.at( 3 ).xs;
        std::for_each( xs.begin(), xs.end() - 1, []( auto& x ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x == Approx( 0.5 ) );
            } );
    }

    SECTION( "can be negative" ) {
        const auto& xs = sec.at( 4 ).xs;
        std::for_each( xs.begin(), xs.end() - 1, []( auto& x ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x == Approx( -0.5 ) );
        } );
    }

    SECTION( "can be written in exponential notation" ) {
        const auto& xs = sec.at( 5 ).xs;
        std::for_each( xs.begin(), xs.end() - 1, []( auto& x ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x == Approx( 50 ) );
        } );
    }

    SECTION( "can be negative with exponential notation" ) {
        const auto& xs = sec.at( 6 ).xs;
        std::for_each( xs.begin(), xs.end() - 1, []( auto& x ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x == Approx( -50 ) );
        } );
    }

    SECTION( "can have negative exponent" ) {
        const auto& xs = sec.at( 7 ).xs;
        std::for_each( xs.begin(), xs.end() - 1, []( auto& x ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x == Approx( 0.005 ) );
        } );
    }
}

TEST_CASE( "simple strings", "[string]") {
    const std::string input = R"(
GRIDOPTS
    'YES' /

GRIDOPTS
    YES /

GRIDOPTS
    2*'YES' /

GRIDOPTS
    2*YES /

GRIDOPTS
    YES/
)";

    auto sec = lun::parse( input );
    REQUIRE( sec.size() == 5 );

    for( const auto& kw : sec )
        CHECK( kw.name == "GRIDOPTS" );

    for( const auto& kw : sec )
        REQUIRE( kw.xs.size() == 2 );

    SECTION( "can be quoted" ) {
        const auto& x = sec.at( 0 ).xs.front();
        REQUIRE_THAT( x, IsString() );
        CHECK( x == "YES" );
    }

    SECTION( "can omit quotes" ) {
        const auto& x = sec.at( 1 ).xs.front();
        REQUIRE_THAT( x, IsString() );
        CHECK( x == "YES" );
    }

    SECTION( "can be repeated with quotes" ) {
        const auto& x = sec.at( 2 ).xs.front();
        REQUIRE_THAT( x, IsString() );
        CHECK_THAT( x, Repeats( 2 ) );
        CHECK( x == "YES" );
    }

    SECTION( "can be repeated without quotes" ) {
        const auto& x = sec.at( 3 ).xs.front();
        REQUIRE_THAT( x, IsString() );
        CHECK_THAT( x, Repeats( 2 ) );
        CHECK( x == "YES" );
    }

    SECTION( "stops at slash without whitespace" ) {
        const auto& x = sec.at( 4 ).xs.front();
        REQUIRE_THAT( x, IsString() );
        CHECK( x == "YES" );
    }
}
