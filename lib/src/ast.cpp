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
    struct positions {
        int kw   = 0;
        int item = 0;
    };

    lun_ast ast;
    positions pos;
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
    return cur->ast->keywords.at( cur->pos.kw ).name.c_str();
}

namespace {
struct endvisit : boost::static_visitor< bool > {
    template< typename T >
    bool operator()( T )                  const { return false; }
    bool operator()( const std::string& ) const { return false; }
    bool operator()( lun::item::endrec )  const { return true; }
};

bool is_end( const lun::item& x ) {
    return boost::apply_visitor( endvisit(), x.val );
}

}

int luncur_records( const lun_cursor cur ) {
    const auto& items = cur->ast->keywords.at( cur->pos.kw ).xs;
    if( items.empty() ) return 0;

    std::count_if( items.begin(), items.end(), is_end );
}

namespace {

int advancekw( lun_cursor cur, int steps ) {
    const auto& kws = cur->ast->keywords;
    const auto next = cur->pos.kw + steps;

    if( next < 0 || next >= kws.size() )
        return LUN_OUT_OF_RANGE;

    cur->pos = { next, 0 };

    return LUN_OK;
}

int advancerec( lun_cursor cur, int steps ) {
    const auto& items = cur->ast->keywords.at( cur->pos.kw ).xs;
    if( items.empty() ) return LUN_OUT_OF_RANGE;

    const auto current = items.begin() + cur->pos.item;

    const auto begin = steps > 0
                     ? current
                     : std::begin( items );

    const auto end = steps > 0
                   ? std::end( items )
                   : current;

    auto count = [steps] ( const auto& x ) mutable {
        if( is_end( x ) ) --steps;
        return steps == 0;
    };

    const auto itr = std::find_if( begin, end, count );

    if( itr == end )
        return LUN_OUT_OF_RANGE;

    /* moved forward, so check if this is the final end marker */
    if( steps > 0 && std::distance( itr, end ) == 1 )
        return LUN_OUT_OF_RANGE;

    cur->pos.item = std::distance( std::begin( items ), itr );
    return LUN_OK;
}

int advanceitem( lun_cursor cur, int steps ) {
    const auto& items = cur->ast->keywords.at( cur->pos.kw ).xs;
    const auto next = cur->pos.item + steps;

    if( next < 0 || next >= items.size() )
        return LUN_OUT_OF_RANGE;

    const auto min = std::min( cur->pos.item, next );
    const auto max = std::max( cur->pos.item, next );

    const auto begin = items.begin() + min;
    const auto end = items.begin() + max + 1;
    const auto itr = std::find_if( begin, end, is_end );

    if( itr != end )
        return LUN_OUT_OF_RANGE;

    cur->pos.item = next;
    return LUN_OK;
}

}

int luncur_advance( lun_cursor cur, int type, int steps ) {
    if( steps == 0 ) return LUN_OK;

    switch( type ) {
        case LUN_KW:   return advancekw( cur, steps );
        case LUN_REC:  return advancerec( cur, steps );
        case LUN_ITEM: return advanceitem( cur, steps );
        default:       return LUN_NOTYPE;
    }
}

int luncur_next( lun_cursor cur, int type ) {
    return luncur_advance( cur, type, 1 );
}

int luncur_prev( lun_cursor cur, int type ) {
    return luncur_advance( cur, type, -1 );
}
