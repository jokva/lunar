#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <lunar/parser.hpp>

std::string dot( const section& sec ) {
    std::stringstream stream;

    stream << sec.name << " -> ";
    stream << "{";
    for( const auto& kw : sec.xs )
        stream << kw.name << " ";
    stream << "}" << std::endl;
}

int main( int argc, char** argv ) {
    const std::string filename{ argv[ 1 ] };
    std::ifstream fs( filename );
    std::string input{ std::istreambuf_iterator< char >( fs ),
                       std::istreambuf_iterator< char >() };

    auto begin = input.cbegin();
    auto end   = input.cend();
    auto sec = parse( begin, end );

    std::cout << dot( sec ) << std::endl;
}
