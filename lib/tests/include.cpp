#include <string>

#include <lunar/parser.hpp>
#include <lunar/concatenate.hpp>

#include <catch/catch.hpp>

namespace {

template< typename T >
std::string str( const T& x ) {
    return std::string( std::begin( x ), std::end( x ) );
}

}

SCENARIO( "path aliases are resolved", "[paths]" ) {
    GIVEN( "a resolver with some aliases" ) {
        lun::pathresolver aliases;
        lun::pathresolver::kv init[] = {
            { "DIR",    "dir1" },
            { "DOTDIR", "./dir2" },
            { "MULTI",  "dir3/dir4" },
            { "BACK",   "dir5\\dir6" },
        };
        aliases.insert( std::begin( init ), std::end( init ) );

        WHEN( "inputs only contain the alias" ) {
            THEN( "it is substituted" ) {
                CHECK( aliases.resolve( "$DIR" ) == "dir1" );
                CHECK( aliases.resolve( "$DOTDIR" ) == "./dir2" );
            }

        }

        WHEN( "inputs have directory separators" ) {
            THEN( "substution terminates at dirname" ) {
                CHECK( aliases.resolve( "$DIR/name" ) == "dir1/name" );
                CHECK( aliases.resolve( "$DIR\\name" ) == "dir1\\name" );
                CHECK( aliases.resolve( "name/$DIR" ) == "name/dir1" );
                CHECK( aliases.resolve( "name\\$DIR" ) == "name\\dir1" );
            }
        }

        WHEN( "inputs have multiple substituions" ) {
            THEN( "all are resolved" ) {
                CHECK( aliases.resolve( "$DIR/name/$DOTDIR" ) == "dir1/name/./dir2" );
                CHECK( aliases.resolve( "$DIR\\name\\$DOTDIR" ) == "dir1\\name\\./dir2" );
            }

            THEN( "$ works as a separator" ) {
                CHECK( aliases.resolve( "$DIR$DOTDIR" ) == "dir1./dir2" );
                CHECK( aliases.resolve( "$DOTDIR$DIR" ) == "./dir2dir1" );
            }
        }

        WHEN( "the substitute has multiple levels" ) {
            THEN( "all levels are carried over" ) {
                CHECK( aliases.resolve( "$MULTI" ) == "dir3/dir4" );
                CHECK( aliases.resolve( "$DIR/$MULTI" ) == "dir1/dir3/dir4" );
            }
        }

        WHEN( "the substitute has backslash in path" ) {
            THEN( "the backslash carries over" ) {
                CHECK( aliases.resolve( "$BACK" ) == "dir5\\dir6" );
            }
        }

        WHEN( "the substitute isn't registered" ) {
            CHECK_THROWS( aliases.resolve( "$FOO" ) );
        }
    }
}

TEST_CASE( "valid include", "[include]" ) {
    using Catch::Matchers::Equals;

    SECTION( "single include" ) {
        auto cat = lun::concatenate( "decks/valid.data" );
        CHECK_THAT( str( cat.inlined ), Equals( "included-in-valid\n" ) );
    }

    SECTION( "included with PATHS alias" ) {
        auto cat = lun::concatenate( "decks/paths-in-root.data" );
        CHECK_THAT( str( cat.inlined ), Equals( "included-in-valid\n" ) );
    }

    SECTION( "PATHS defined in included file", "[include][paths]" ) {
        auto cat = lun::concatenate( "decks/paths-in-recursive.data" );
        CHECK_THAT( str( cat.inlined ), Equals( "included-in-valid\n" ) );
    }

    SECTION( "PATHS with backslashes" ) {
        auto cat = lun::concatenate( "decks/paths-with-backslash.data" );
        CHECK_THAT( str( cat.inlined ), Equals( "included-in-valid\n" ) );
    }
}

TEST_CASE( "include with wrong case", "[include]" ) {
    CHECK_THROWS( lun::concatenate( "decks/wrong-case-filename.data" ) );
    CHECK_THROWS( lun::concatenate( "decks/wrong-case-dirname.data" ) );
    CHECK_THROWS( lun::concatenate( "decks/wrong-case-dirname-filename.data" ) );
}

TEST_CASE( "include non-existent file", "[include]" ) {
    CHECK_THROWS( lun::concatenate( "void.data" ) );
}
