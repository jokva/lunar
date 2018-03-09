#include <iostream>
#include <memory>
#include <vector>

#include <lunar/ast.hpp>
#include <lunar/parser.hpp>

namespace lun {

struct ast::impl {
    impl() = default;
    explicit impl( std::vector< keyword > v ) : tree( std::move( v ) ) {}

    std::vector< keyword >* operator->() noexcept {
        return &this->tree;
    }
    const std::vector< keyword >* operator->() const noexcept {
        return &this->tree;
    }

    std::vector< keyword > tree;
};

ast::ast() = default;
ast::ast( ast&& ) = default;
ast::ast( const ast& o ) : nodes( std::make_unique< impl >( *o.nodes.get() ) ) {}

ast& ast::operator=( ast&& o ) = default;

ast& ast::operator=( const ast& o ) {
    this->nodes = std::make_unique< impl >( *o.nodes.get() );
    return *this;
}

ast::~ast() = default;

ast::cur::cur( ast::impl& x ) : p( x ) {}

ast::cur ast::cursor() const {
    return cur( *this->nodes.get() );
}

namespace {

struct endvisit : boost::static_visitor< bool > {
    template< typename T >
    bool operator()( T )                  const { return false; }
    bool operator()( const std::string& ) const { return false; }
    bool operator()( item::endrec )  const { return true;  }
};

bool is_end( const item& x ) {
    return boost::apply_visitor( endvisit(), x.val );
}

}

bool ast::cur::advance( ast::nodetype nt, int steps ) {

    auto keyword = [=]( ast::cur& cursor ) {
        const auto& kws = cursor.p.tree;
        const auto next = cursor.kw + steps;

        if( next < 0 || next >= kws.size() ) return false;

        cursor.kw = next;
        cursor.item = 0;

        return true;
    };

    auto count = [steps] ( const auto& x ) mutable {
        if( is_end( x ) ) --steps;
        return steps == 0;
    };

    auto record = [=]( auto& cursor ) {
        const auto& items = cursor.p->at( cursor.kw ).xs;
        if( items.empty() ) return false;

        const auto current = items.begin() + cursor.item;

        const auto begin = steps > 0 ? current           : std::begin( items );
        const auto end   = steps > 0 ? std::end( items ) : current;

        const auto itr = std::find_if( begin, end, count );

        if( itr == end ) return false;

        /* moved forward, so check if this is the final end marker */
        if( steps > 0 && std::distance( itr, end ) == 1 )
            return false;

        cursor.item = std::distance( std::begin( items ), itr );
        return true;
    };

    auto item = [=]( auto& cursor ) {
        const auto& items = cursor.p->at( cursor.kw ).xs;
        const auto next = cursor.item + steps;

        if( next < 0 || next >= items.size() )
            return false;

        const auto min = std::min( cursor.item, next );
        const auto max = std::max( cursor.item, next );

        const auto begin = items.begin() + min;
        const auto end = items.begin() + max + 1;
        const auto itr = std::find_if( begin, end, is_end );

        if( itr != end )
            return false;

        cursor.item = next;
        return true;
    };

    switch( nt ) {
        case nodetype::kw:   return keyword( *this );
        case nodetype::rec:  return record( *this );
        case nodetype::item: return item( *this );
        default:
            throw std::runtime_error( "Unknown node type tag" );
    }
}

bool ast::cur::next( ast::nodetype nt ) {
    return this->advance( nt, 1 );
}

bool ast::cur::prev( ast::nodetype nt ) {
    return this->advance( nt, -1 );
}

const std::string& ast::cur::name() const {
    return this->p->at( this->kw ).name;
}

int ast::cur::records() const {
    const auto& items = this->p->at( this->kw ).xs;
    if( items.empty() ) return 0;

    return std::count_if( items.begin(), items.end(), is_end );
}

int ast::cur::repeats() const {
    const auto& items = this->p->at( this->kw ).xs;
    if( items.empty() ) return -1;
    return items.at( this->item ).repeat;
}

ast::type ast::cur::type() const {
    const auto& items = this->p->at( this->kw ).xs;
    if( items.empty() ) return ast::type::End;

    switch( items.at( this->item ).val.which() ) {
        case 0: return ast::type::Int;
        case 1: return ast::type::Float;
        case 2: return ast::type::Str;
        case 3: return ast::type::None;
        default:
             throw std::runtime_error( "wrong type tag!" );
    }
}

ast parse( const std::string& str ) {
    auto kws = parse( str.begin(), str.end() );
    auto impl = std::make_unique< ast::impl >();
    impl->tree = std::move( kws );
    ast tree;
    tree.nodes = std::move( impl );
    return tree;
}

std::ostream& operator<<( std::ostream& stream, const ast::cur& cursor ) {
    return stream << cursor.p->at( cursor.kw ).xs.at( cursor.item );
}

}
