#ifndef PARSER_HPP
#define PARSER_HPP

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <boost/variant.hpp>

namespace lun {

struct item {
    struct star {
        star() = default;
        star( int x ) : val( x ) {}
        operator int() const { return this->val; }
        int val = 1;
    };

    struct none {};

    boost::variant< int, double, std::string, none > val;
    star repeat;
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

std::vector< section > parse( std::string::const_iterator fst,
                              std::string::const_iterator lst );

struct inlined {
    std::vector< char > inlined;
    std::vector< std::string > included;
};


inlined concatenate( const std::string& path );

std::string dot( const std::vector< section >& );

std::ostream& operator<<( std::ostream&, const item::star& );
std::ostream& operator<<( std::ostream&, const item::none& );
std::ostream& operator<<( std::ostream&, const item& );

}

#endif // PARSER_HPP
