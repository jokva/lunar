#ifndef LUNAR_AST_HPP
#define LUNAR_AST_HPP

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

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

        std::vector< std::string > keywords() const;
        std::vector< std::string > unique_keywords() const;

        cur cursor() const;

    private:
        friend ast lun::parse( const std::string& );
        class impl;
        std::unique_ptr< impl > nodes;
};

class ast::cur {
    public:
        explicit cur( ast::impl& );

        bool advance( ast::nodetype, int n = 1 );
        bool next( ast::nodetype );
        bool prev( ast::nodetype );

        const std::string& name() const;
        int records() const;

        int repeats() const;
        ast::type type() const;

    private:
        const ast::impl& p;
        int kw = 0;
        int item = 0;
        friend std::ostream& operator<<( std::ostream&, const cur& );
};

ast parse( const std::string& );

}

#endif // LUNAR_AST_HPP
