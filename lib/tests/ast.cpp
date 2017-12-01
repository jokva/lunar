#include <catch/catch.hpp>

#include <lunar/ast.h>

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

        WHEN( "a cursor is constructed" ) {
            cursor cur( luncur_make( ast.get() ) );

            THEN( "it can be copied" ) {
                cursor cpy( luncur_copy( cur.get() ) );
                CHECK( cpy );
            }
        }
    }
}
