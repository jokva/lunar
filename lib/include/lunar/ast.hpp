#ifndef LUNAR_AST_HPP
#define LUNAR_AST_HPP

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace lun {

class ast {
    public:
        ast();
        ast( ast&& );
        ast( const ast& );
        ast& operator=( ast&& );
        ast& operator=( const ast& );
        ~ast();

        class cur;
        enum class type { Int, Float, Str, None, End };
        enum class nodetype { kw, rec, item };
        enum class arity { none, unary, nary };

        std::vector< std::string > keywords() const;
        std::vector< std::string > unique_keywords() const;

        cur cursor() const;

        ast& define_keyword( std::string, arity );

        ast& addkeyword( const std::string& );
        ast& addkeyword( std::string, arity );
        ast& addrecord();
        ast& endrecord();
        ast& additem( int );
        ast& additem( double );
        ast& additem( std::string );
        ast& additem();

        ast& read( const std::string& );

    private:
        class impl;
        std::unique_ptr< impl > nodes;

        std::unordered_map< std::string, arity > reg;
        std::vector< int > keywordpos;
};

class ast::cur {
    public:
        explicit cur( const ast& );

        bool advance( ast::nodetype, int n = 1 );
        bool next( ast::nodetype );
        bool prev( ast::nodetype );

        const std::string& name() const;
        int records() const;

        int repeats() const;
        ast::type type() const;

    private:
        const ast& tree;
        int kw = 0;
        int pos = 0;
        friend std::ostream& operator<<( std::ostream&, const cur& );
};

}

#endif // LUNAR_AST_HPP
