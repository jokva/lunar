#include <iostream>
#include <memory>
#include <vector>

#include <lunar/ast.hpp>
#include <lunar/parser.hpp>

namespace lun {

struct ast::impl {
    impl() = default;
    explicit impl( std::vector< item > v ) : tree( std::move( v ) ) {}

    std::vector< item >* operator->() noexcept {
        return &this->tree;
    }
    const std::vector< item >* operator->() const noexcept {
        return &this->tree;
    }

    std::vector< item > tree;
};

ast::ast() : nodes( std::make_unique< impl >() ) {}
ast::ast( ast&& ) = default;
ast::ast( const ast& o ) : nodes( std::make_unique< impl >( *o.nodes.get() ) ) {}

ast& ast::operator=( ast&& o ) = default;

ast& ast::operator=( const ast& o ) {
    this->nodes = std::make_unique< impl >( *o.nodes.get() );
    return *this;
}

ast::~ast() = default;

ast::cur::cur( const ast& x ) : tree( x ) {}

ast::cur ast::cursor() const {
    return cur( *this );
}

ast& ast::define_keyword( std::string key, ast::arity numrecords ) {
    auto itr = this->reg.emplace( key, numrecords ).first;

    if( itr->second != numrecords ) {
        throw std::invalid_argument(
            "keyword already defined with different arity"
        );
    }

    return *this;
}

ast& ast::addkeyword( const std::string& key ) {
    auto arity = this->reg.at( key );
    return this->addkeyword( key, arity );
}

ast& ast::addkeyword( std::string key, ast::arity numrecords ) {
    this->define_keyword( std::move( key ), numrecords );
    this->nodes->tree.push_back( { key } );
    this->keywordpos.push_back( this->nodes->tree.size() - 1 );
    return *this;
}

ast& ast::addrecord() {
    return *this;
}

ast& ast::endrecord() {
    this->nodes->tree.push_back( { lun::item::endrec() } );
    return *this;
}

ast& ast::additem( int x ) {
    this->nodes->tree.push_back( { x } );
    return *this;
}

ast& ast::additem( double x ) {
    this->nodes->tree.push_back( { x } );
    return *this;
}

ast& ast::additem( std::string x ) {
    this->nodes->tree.push_back( { std::move( x ) } );
    return *this;
}

ast& ast::additem() {
    this->nodes->tree.push_back( { lun::item::none() } );
    return *this;
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

ast& ast::read( const std::string& str ) {
    const auto tokens = tokenize( str.begin(), str.end() ).tokens;

    const auto fst = tokens.begin();
    const auto lst = tokens.end();
    auto itr = fst;

    while( true ) {
        if( itr == lst ) break;

        const auto& key = boost::get< std::string >( itr->val );
        if( this->reg.count( key ) != 1 )
            throw std::runtime_error( "Unknown keyword: " + key );

        this->keywordpos.push_back( std::distance( fst, itr ) );

        switch( this->reg.at( key ) ) {
            case ast::arity::none: break;
            case ast::arity::unary:
                itr = std::find_if( itr, lst, is_end );
                break;
            case ast::arity::nary:
                break;
                while( true ) {
                    itr = std::find_if( itr, lst, is_end );
                    if( itr == lst ) {
                        --itr;
                        break;
                    }

                    if( is_end( *(itr+1) ) ) {
                        ++itr;
                        break;
                    }
                }
                break;
        }

        ++itr;
    }

    this->nodes = std::make_unique< impl >( tokens );
    return *this;
}


bool ast::cur::advance( ast::nodetype nt, int steps ) {

    auto keyword = [=]( ast::cur& cursor ) {
        const auto& tree = cursor.tree;

        if( steps + cursor.kw >= tree.keywordpos.size()
         or steps + cursor.kw < 0 )
            return false;

        cursor.kw += steps;
        cursor.pos = tree.keywordpos[cursor.kw];

        if( cursor.kw + 1 != tree.keywordpos.size()
        and cursor.pos + 1 < tree.keywordpos[cursor.kw + 1] )
            cursor.pos += 1;

        return true;
    };

    auto count = [steps] ( const auto& x ) mutable {
        if( is_end( x ) ) --steps;
        return steps == 0;
    };

    auto record = [=]( auto& cursor ) {
        const auto& tree = cursor.tree;
        const auto& tokens = tree.nodes->tree;

        const auto current = tokens.begin() + cursor.pos;
        const auto kwbegin = cursor.tree.keywordpos[ cursor.kw ];
        const auto kwend = cursor.kw == cursor.tree.keywordpos.size() - 1
                         ? tokens.size() - 1
                         : cursor.tree.keywordpos[ cursor.kw + 1 ]
                         ;

        const auto begin = steps > 0 ? current : std::begin( tokens ) + kwbegin;
        const auto end   = steps > 0 ? std::begin( tokens ) + kwend : current;

        const auto itr = std::find_if( begin, end, count );

        if( itr == end ) return false;

        /* moved forward, so check if this is the final end marker */
        if( steps > 0 && std::distance( itr, end ) == 1 )
            return false;

        cursor.pos = std::distance( std::begin( tokens ), itr );
        return true;
    };

    auto item = [=]( auto& cursor ) {
        const auto& tree = cursor.tree;
        const auto& keywordpos = tree.keywordpos;
        // TODO: Make all invariants/checks complete
        if( keywordpos.at(cursor.kw + 1) - keywordpos.at(cursor.kw) == 1 )
            return false;

        const auto& tokens = cursor.tree.nodes->tree;
        const auto next = cursor.pos + steps;

        if( next < 0 || next >= tokens.size() )
            return false;

        const auto min = std::min( cursor.pos, next );
        const auto max = std::max( cursor.pos, next );

        const auto begin = tokens.begin() + min;
        const auto end = tokens.begin() + max + 1;
        const auto itr = std::find_if( begin, end, is_end );

        if( itr < end )
            return false;

        cursor.pos = next;
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

static std::string emptystr = "";
struct getstr : boost::static_visitor< const std::string& > {
    template< typename T >
    const std::string& operator()( T )                    const { return emptystr; }
    const std::string& operator()( const std::string& x ) const { return x; }
};


const std::string& ast::cur::name() const {
    const auto& kw = this->tree.nodes->tree.at(
            this->tree.keywordpos[ this->kw ]
    );

    const auto& key = boost::get< std::string >( kw.val );
    return boost::apply_visitor( getstr(), kw.val );
}

int ast::cur::records() const {
    const auto& tokens = this->tree.nodes->tree;
    auto fst = std::begin( tokens ) + this->tree.keywordpos.at( this->kw );
    // TODO: .at()/range check if this is last keyword
    auto lst = this->kw + 1 == this->tree.keywordpos.size()
             ? std::end( tokens )
             : std::begin( tokens ) + this->tree.keywordpos.at( this->kw + 1 );

    return std::count_if( fst, lst, is_end );
}

int ast::cur::repeats() const {
    return this->tree.nodes->tree.at( this->pos ).repeat;
}

ast::type ast::cur::type() const {
    const auto& tokens = this->tree.nodes->tree;

    if( this->pos + 1 == tokens.size() )
        return ast::type::End;

    switch( tokens.at( this->pos ).val.which() ) {
        case 0: return ast::type::Int;
        case 1: return ast::type::Float;
        case 2: return ast::type::Str;
        case 3: return ast::type::None;

        default:
             throw std::runtime_error( "wrong type tag!" );
    }
}

//parser& parser::add_toggle( std::string key ) {
//    // TODO: verify( key )
//    this->toggles.push_back( std::move( key ) );
//    return *this;
//}
//
//parser& parser::add_single( std::string key ) {
//    // TODO: verify( key )
//    this->singles.push_back( std::move( key ) );
//    return *this;
//}
//
//parser& parser::add_multi( std::string key ) {
//    // TODO: verify( key )
//    this->multis.push_back( std::move( key ) );
//    return *this;
//}
//
//std::ostream& operator<<( std::ostream& stream, const ast::cur& cursor ) {
//    return stream << cursor.p->at( cursor.kw );
//}

}
