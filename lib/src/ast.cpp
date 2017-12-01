#include <algorithm>
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
    size_t pos = 0;
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

const char* luncur_kwname( const lun_cursor cur ) {
    return cur->ast->keywords.at( cur->pos ).name.c_str();
}

namespace {
struct endvisit : boost::static_visitor< bool > {
    template< typename T >
    bool operator()( T )                  const { return false; }
    bool operator()( const std::string& ) const { return false; }
    bool operator()( lun::item::endrec )  const { return true; }
};
}

int luncur_records( const lun_cursor cur ) {
    const auto& items = cur->ast->keywords.at( cur->pos ).xs;
    if( items.empty() ) return 0;

    auto is_end = []( const auto& x ) {
        return boost::apply_visitor( endvisit(), x.val );
    };

    std::count_if( items.begin(), items.end(), is_end );
}
