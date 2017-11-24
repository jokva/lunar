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
    struct endrec {};

    boost::variant< int, double, std::string, none, endrec > val;
    star repeat;
};

struct keyword {
    std::string name;
    std::vector< item > xs;
};

std::vector< keyword > parse( std::string::const_iterator fst,
                              std::string::const_iterator lst );

struct inlined {
    std::vector< char > inlined;
    std::vector< std::string > included;
};


inlined concatenate( const std::string& path );

std::string dot( const std::vector< keyword >& );

std::ostream& operator<<( std::ostream&, const item::star& );
std::ostream& operator<<( std::ostream&, const item::none& );
std::ostream& operator<<( std::ostream&, const item::endrec& );
std::ostream& operator<<( std::ostream&, const item& );

}

#endif // PARSER_HPP
