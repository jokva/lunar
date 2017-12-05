#include <string>

#include <catch/catch.hpp>

#include <lunar/ast.h>

using namespace std::string_literals;

namespace {

struct ast_deleter {
    void operator()( const lun_ast ast ) { lun_ast_dispose( ast ); }
};

struct cursor_deleter {
    void operator()( const lun_cursor cur ) { luncur_dispose( cur ); }
};

using AST = std::unique_ptr< lunAST, ast_deleter >;
using cursor = std::unique_ptr< luncursor, cursor_deleter >;

}

SCENARIO( "cursors can be created", "[cursor]" ) {
    const std::string input = R"(
RUNSPEC

EQLDIMS
    3*5 /

DIMENS
    5 2*10 /
)";

    GIVEN( "a valid, incomplete deck" ) {
        const auto begin = input.c_str();
        const auto end = input.c_str() + input.size();
        AST ast( lun_parse_string( begin, end ) );

        cursor curptr( luncur_make( ast.get() ) );
        auto* cur = curptr.get();

        WHEN( "a cursor is constructed" ) {
            THEN( "the keyword matches the first in the deck" ) {
                CHECK( luncur_id( cur )  == "RUNSPEC"s );
                CHECK( luncur_records( cur ) == 0 );
            }
        }

        WHEN( "a cursor is copied" ) {
            cursor cpyptr( luncur_copy( cur ) );
            REQUIRE( cpyptr );
            auto* cpy = cpyptr.get();

            THEN( "the keyword names match" ) {
                CHECK( luncur_id( cpy ) == "RUNSPEC"s );
                CHECK( luncur_id( cur ) == "RUNSPEC"s );
            }

            THEN( "the numbers of records match" ) {
                CHECK( luncur_records( cpy ) == luncur_records( cur ) );
            }
        }

        WHEN( "a copy is advanced" ) {
            cursor cpyptr( luncur_copy( cur ) );
            REQUIRE( cpyptr );
            auto* cpy = cpyptr.get();

            auto ok = luncur_next( cpy, LUN_KW );
            REQUIRE( ok == LUN_OK );

            THEN( "the keyword is different" ) {
                CHECK( luncur_id( cpy )  == "EQLDIMS"s );
                CHECK( luncur_records( cpy ) == 1 );
            }

            THEN( "the original is unchanged" ) {
                CHECK( luncur_id( cur )  == "RUNSPEC"s );
                CHECK( luncur_records( cur ) == 0 );
            }
        }

        WHEN( "a single-item record is traversed per-item" ) {
            cursor cpyptr( luncur_copy( cur ) );
            REQUIRE( cpyptr );
            auto* cpy = cpyptr.get();

            auto ok = luncur_next( cpy, LUN_KW );
            REQUIRE( ok == LUN_OK );

            THEN( "advancing the record cursor fails" ) {
                auto adv = luncur_next( cpy, LUN_REC );
                CHECK( adv == LUN_OUT_OF_RANGE );
            }

            THEN( "advancing the item cursor fails" ) {
                auto adv = luncur_next( cpy, LUN_ITEM );
                CHECK( adv == LUN_OUT_OF_RANGE );
            }

            THEN( "advancing the keyword cursor succeeds" ) {
                auto adv = luncur_next( cpy, LUN_KW );
                CHECK( adv == LUN_OK );
            }
        }
    }
}
