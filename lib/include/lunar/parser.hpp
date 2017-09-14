#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

#include <boost/variant.hpp>

using item = boost::variant<
    std::vector< int >,
    std::vector< double >
>;

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


#endif // PARSER_HPP
