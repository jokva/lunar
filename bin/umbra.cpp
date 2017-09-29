#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <lunar/parser.hpp>

std::string dot( const section& sec ) {
    std::stringstream stream;

    stream << "strict graph {" << std::endl;

    stream << sec.name << " -- { ";
    for( const auto& kw : sec.xs )
        stream << kw.name << " ";
    stream << "}" << std::endl;

    for( const auto& kw : sec.xs ) {

        stream << kw.name << " -- { ";
        for( size_t i = 0; i < kw.xs.size(); ++i ) {
            stream << kw.name << "_" << i << " ";
        }
        stream << " }" << std::endl;

        for( size_t i = 0; i < kw.xs.size(); ++i ) {
            const auto& rec = kw.xs[ i ];

            stream << kw.name << "_" << i << " -- { ";
            for( size_t j = 0; j < rec.size(); ++j ) {
                const auto& item = rec.at( j );
                stream << kw.name << "_" << i << "_" << j
                       << "[shape=record, label="
                       << "\"" << item << "\"" << "]";
            }
            stream << " }" << std::endl;


        }
    }

    stream << "}";

    return stream.str();
}

int main( int , char** argv ) {
    const std::string filename{ argv[ 1 ] };
    std::ifstream fs( filename );
    std::string input{ std::istreambuf_iterator< char >( fs ),
                       std::istreambuf_iterator< char >() };

    auto begin = input.cbegin();
    auto end   = input.cend();
    auto sec = parse( begin, end );

    std::cout << dot( sec ) << std::endl;
}
