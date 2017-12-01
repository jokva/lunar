#include <memory>

#include <lunar/ast.h>
#include <lunar/parser.hpp>

using namespace lun;

struct lunAST {
    std::vector< keyword > keywords;
};

lun_ast lun_parse_string( const char* begin, const char* end ) {
    auto ast = std::make_unique< lunAST >();
    ast->keywords = lun::parse( begin, end );
    return ast.release();
}

void lun_ast_dispose( const lun_ast ast ) {
    delete ast;
}

struct luncursor {
    lun_ast ast;
};

lun_cursor luncur_make( const lun_ast ast ) {
    auto cur = std::make_unique< luncursor >();
    cur->ast = ast;
    return cur.release();
}

lun_cursor luncur_copy( const lun_cursor cur ) {
    return std::make_unique< luncursor >( *cur ).release();
}

void luncur_dispose( const lun_cursor cur ) {
    delete cur;
}
