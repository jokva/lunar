#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <string>
#include <vector>

struct item {
    enum class tag { i, f, str, none };
    struct star {
        star() = default;
        star( int x ) : val( x ) {}
        operator int() const { return this->val; }
        int val = 1;
    };

    tag type = tag::none;
    star repeat = 1;

    item() = default;

    item( int star, int x )    : type( tag::i ), repeat( star ), ival( x ) {}
    item( int star, double x ) : type( tag::f ), repeat( star ), fval( x ) {}
    item( int star, std::string x ) :
        type( tag::str ),
        repeat( star ),
        sval( std::move( x ) ) {}

    item( int, star x ) : type( tag::none ), repeat( x.val ) {}
    item( int star, tag ) : type( tag::none ), repeat( star ) {}
    static item defaulted( int star = 1 ) { return item( star, tag::none ); }

    template< typename T >
    item( T x ) : item( 1, std::forward< T >( x ) ) {}

    int ival;
    double fval;
    std::string sval;
};

using record = std::vector< item >;

struct keyword {
    std::string name;
    std::vector< record > xs;
};

struct section {
    std::string name;
    std::vector< keyword > xs;
};

section parse( std::string::const_iterator fst,
               std::string::const_iterator lst );

std::string dot( const section& );

std::ostream& operator<<( std::ostream&, const item::star& );
std::ostream& operator<<( std::ostream&, const item::tag& );
std::ostream& operator<<( std::ostream&, const item& );

#endif // PARSER_HPP
