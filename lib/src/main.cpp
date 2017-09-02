#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

#include <lunar/parser.hpp>

namespace std {

template< typename T >
std::ostream& operator<<(std::ostream& stream, const std::vector< T >& v ) {
    for( auto&& x : v ) stream << x << " ";
    return stream;
}

}

std::ostream& operator<<( std::ostream& stream, const item& i ) {
    stream << "item ( ";
    for( auto x : i ) stream << x << " ";
    return stream << ")";
}

std::ostream& operator<<( std::ostream& stream, const record& r ) {
    stream << "record { ";
    for( auto& x : r ) stream << x << " ";
    return stream << "}";
}

std::ostream& operator<<( std::ostream& stream, const keyword& kw ) {
    return stream << kw.name << "\t[ " << kw.xs << "]";
}

std::ostream& operator<<(std::ostream& stream, const section& r ) {
    stream << r.name << ":\n";
    for( const auto& x : r.xs )
        stream << x << "\n";
    return stream;
}

int main( int argc, char** argv ) {
    const std::string filename{ argv[ 1 ] };
    std::ifstream fs( filename );
    std::string input{ std::istreambuf_iterator< char >( fs ),
                       std::istreambuf_iterator< char >() };

    auto begin = input.cbegin();
    auto end   = input.cend();
    auto sec = parse( begin, end );

    std::cout << sec << std::endl;
}
