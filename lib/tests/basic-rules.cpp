#include <algorithm>
#include <stdexcept>
#include <string>

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

class IsType : public Catch::MatcherBase< lun::item > {
    public:
        IsType( lun::item::tag x ) : type( x ) {}

        bool match( const lun::item& x ) const override {
            return this->type == x.type;
        }

        virtual std::string describe() const override {
            std::stringstream ss;
            ss << "is of type " << this->type;
            return ss.str();
        }

    private:
        lun::item::tag type;
};

IsType IsInt() { return IsType( lun::item::tag::i ); }
IsType IsFloat() { return IsType( lun::item::tag::f ); }
IsType IsString() { return IsType( lun::item::tag::str ); }

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

    auto sec = lun::parse( input.begin(), input.end() ).front();
    REQUIRE( sec.name == "RUNSPEC" );

    SECTION( "toggles have no items" ) {
        CHECK_THAT( sec.xs, HasKeyword( "OIL" ) );
        const auto& oil = get( "OIL", sec.xs );
        CHECK( oil.xs.empty() );
    }

    SECTION( "integers are recognised" ) {
        REQUIRE_THAT( sec.xs, HasKeyword( "DIMENS" ) );
        const auto& kw = get( "DIMENS", sec.xs );

        for( const auto& item : kw.xs ) {
            REQUIRE( !item.empty() );
            CHECK_THAT( item[ 0 ], IsInt() );
        }
    }

    SECTION( "/ following int does not change the value" ) {
        REQUIRE_THAT( sec.xs, HasKeyword( "EQLDIMS" ) );
        const auto& eqldims = get( "EQLDIMS", sec.xs );
        REQUIRE( !eqldims.xs.empty() );
        REQUIRE( eqldims.xs[ 0 ].size() == 1 );

        const auto& rec = eqldims.xs[ 0 ];
        REQUIRE( !rec.empty() );
        REQUIRE_THAT( rec[ 0 ], IsInt() );
        CHECK( rec[ 0 ].ival == 2 );
    }

    SECTION( "/ on new line does not change the value" ) {
        REQUIRE_THAT( sec.xs, HasKeyword( "REGDIMS" ) );
        const auto& kw = get( "REGDIMS", sec.xs );
        REQUIRE( !kw.xs.empty() );
        REQUIRE( kw.xs[ 0 ].size() == 1 );

        auto& rec = kw.xs[ 0 ];
        REQUIRE( !rec.empty() );
        REQUIRE_THAT( rec[ 0 ], IsInt() );
        CHECK( rec[ 0 ].ival == 10 );
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

    auto sec = lun::parse( input.begin(), input.end() ).front();
    REQUIRE( sec.name == "RUNSPEC" );

    SECTION( "a single int" ) {
        REQUIRE_THAT( sec.xs, HasKeyword( "EQLDIMS" ) );

        const auto& kw = get( "EQLDIMS", sec.xs );
        REQUIRE( kw.xs.size() == 1 );
        REQUIRE( kw.xs[ 0 ].size() == 1 );

        const auto& x = kw.xs[ 0 ][ 0 ];
        CHECK_THAT( x, Repeats( 3 ) );
        REQUIRE_THAT( x, IsInt() );
        CHECK( x.ival == 5 );
    }

    SECTION( "int, repeated int" ) {
        REQUIRE_THAT( sec.xs, HasKeyword( "DIMENS" ) );

        const auto& kw = get( "DIMENS", sec.xs );
        REQUIRE( kw.xs.size() == 1 );
        REQUIRE( kw.xs[ 0 ].size() == 2 );

        SECTION( "the single integer" ) {
            const auto& x = kw.xs[ 0 ][ 0 ];
            REQUIRE_THAT( x, IsInt() );
            CHECK( x.ival == 5 );
        }

        SECTION( "the repeated integer" ) {
            const auto& x = kw.xs[ 0 ][ 1 ];
            REQUIRE_THAT( x, IsInt() );
            CHECK_THAT( x, Repeats( 2 ) );
            CHECK( x.ival == 10 );
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
    auto sec = lun::parse( input.begin(), input.end() ).front();
    REQUIRE( sec.name == "GRID" );
    REQUIRE( sec.xs.size() == 7 );

    for( const auto& kw : sec.xs )
        CHECK( kw.name == "MAPAXES" );

    for( const auto& kw : sec.xs )
        REQUIRE( kw.xs.size() == 1 );

    SECTION( "repeating floats" ) {
        const auto& kw = sec.xs[ 0 ];
        const auto& rec = kw.xs.front();
        REQUIRE( rec.size() == 3 );

        SECTION( "3*100." ) {
            const auto& item = rec[ 0 ];
            CHECK_THAT( item, Repeats( 3 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( 100 ) );
        }

        SECTION( "2*13.1" ) {
            const auto& item = rec[ 1 ];
            CHECK_THAT( item, Repeats( 2 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( 13.1 ) );
        }

        SECTION( "4*.3" ) {
            const auto& item = rec[ 2 ];
            CHECK_THAT( item, Repeats( 4 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( .3 ) );
        }
    }

    SECTION( "mix repeated and non-repeated" ) {
        const auto& kw = sec.xs[ 1 ];
        const auto& rec = kw.xs.front();
        REQUIRE( rec.size() == 5 );

        SECTION( "1.2" ) {
            const auto& item = rec[ 0 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( 1.2 ) );
        }

        SECTION( "2*2.4" ) {
            const auto& item = rec[ 1 ];
            CHECK_THAT( item, Repeats( 2 ) );
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( 2.4 ) );
        }

        SECTION( ".8" ) {
            const auto& item = rec[ 2 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( 0.8 ) );
        }

        SECTION( "8.0" ) {
            const auto& item = rec[ 3 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( 8 ) );
        }

        SECTION( "8." ) {
            const auto& item = rec[ 4 ];
            CHECK_THAT( item, IsFloat() );
            CHECK( item.fval == Approx( 8 ) );
        }
    }

    SECTION( "can be written without exponent" ) {
        for( const auto& x : sec.xs[ 2 ].xs.front() ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x.fval == Approx( 0.5 ) );
        }
    }

    SECTION( "can be negative" ) {
        for( const auto& x : sec.xs[ 3 ].xs.front() ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x.fval == Approx( -0.5 ) );
        }
    }

    SECTION( "can be written in exponential notation" ) {
        for( const auto& x : sec.xs[ 4 ].xs.front() ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x.fval == Approx( 50 ) );
        }
    }

    SECTION( "can be negative with exponential notation" ) {
        for( const auto& x : sec.xs[ 5 ].xs.front() ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x.fval == Approx( -50 ) );
        }
    }

    SECTION( "can have negative exponent" ) {
        for( const auto& x : sec.xs[ 6 ].xs.front() ) {
            REQUIRE_THAT( x, IsFloat() );
            CHECK( x.fval == Approx( 0.005 ) );
        }
    }
}

TEST_CASE( "simple strings", "[string]") {
    const std::string input = R"(
RUNSPEC

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

    auto sec = lun::parse( input.begin(), input.end() ).front();
    REQUIRE( sec.name == "RUNSPEC" );
    REQUIRE( sec.xs.size() == 5 );

    for( const auto& kw : sec.xs )
        CHECK( kw.name == "GRIDOPTS" );

    for( const auto& kw : sec.xs )
        REQUIRE( kw.xs.front().size() == 1 );

    SECTION( "can be quoted" ) {
        const auto& x = sec.xs[ 0 ].xs.front().front();
        REQUIRE_THAT( x, IsString() );
        CHECK( x.sval == "YES" );
    }

    SECTION( "can omit quotes" ) {
        const auto& x = sec.xs[ 1 ].xs.front().front();
        REQUIRE_THAT( x, IsString() );
        CHECK( x.sval == "YES" );
    }

    SECTION( "can be repeated with quotes" ) {
        const auto& x = sec.xs[ 2 ].xs.front().front();
        REQUIRE_THAT( x, IsString() );
        CHECK_THAT( x, Repeats( 2 ) );
        CHECK( x.sval == "YES" );
    }

    SECTION( "can be repeated without quotes" ) {
        const auto& x = sec.xs[ 3 ].xs.front().front();
        REQUIRE_THAT( x, IsString() );
        CHECK_THAT( x, Repeats( 2 ) );
        CHECK( x.sval == "YES" );
    }

    SECTION( "stops at slash without whitespace" ) {
        const auto& x = sec.xs[ 4 ].xs.front().front();
        REQUIRE_THAT( x, IsString() );
        CHECK( x.sval == "YES" );
    }
}
