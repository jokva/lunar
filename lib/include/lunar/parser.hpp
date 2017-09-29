#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <string>
#include <vector>

struct star {
    star() = default;
    star( int x ) : val( x ) {}
    operator int() { return this->val; }
    int val = 1;
};

struct item {
    enum class tag { i, f, str, none };

    tag type = tag::none;
    int repeat = -1;

    item() = default;

    item( int star, int x )    : type( tag::i ), repeat( star ), ival( x ) {}
    item( int star, double x ) : type( tag::f ), repeat( star ), fval( x ) {}
    item( int star, std::string x ) :
        type( tag::str ),
        repeat( star ),
        sval( std::move( x ) ) {}

    item( int, star x ) : type( tag::none ), repeat( x ) {}
    item( int star, tag ) : type( tag::none ), repeat( star ) {}
    static item defaulted( int star = 1 ) { return item( star, tag::none ); }

    int getint() const { return this->ival; }
    double getdouble() const { return this->fval; }
    std::string getstring() const { return this->sval; }
    star getstar() const { return this->repeat; }
    tag gettag() const { return this->type; }

    item& set( int x ) {
        this->ival = x;
        this->type = tag::i;
        return *this;
    }

    item& set( double x ) {
        this->fval = x;
        this->type = tag::f;
        return *this;
    }

    item& set( std::string x ) {
        this->sval = std::move( x );
        this->type = tag::str;
        return *this;
    }

    item& set( star x ) {
        this->repeat = x.val;
        return *this;
    }

    item& set( tag ) { this->type = tag::none; return *this; }

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

std::ostream& operator<<( std::ostream&, const item::tag& );
std::ostream& operator<<( std::ostream&, const item& );

#endif // PARSER_HPP
