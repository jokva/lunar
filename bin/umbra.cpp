#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <lunar/ast.hpp>

namespace {

std::ostream& dot( std::ostream& stream, lun::ast::cur cursor ) {
    using namespace lun;
    using nt = lun::ast::nodetype;

    stream << "strict graph {" << std::endl;

    do {
        stream << "root -- ";
        stream << cursor.name() << "[shape=box]\n";

        if( cursor.records() == 0 )
            continue;

        int rec = 0;
        int it = 0;
        do {
            if( it == 0 )
                stream << "\t"
                        << cursor.name() << "_" << rec << "[label=" << rec << "]"
                        << cursor.name() << " -- "
                        << cursor.name() << "_" << rec
                        << "\n";

            stream << "\t\t"
                   << cursor.name() << "_" << rec << "_" << it
                                  << "[shape=record, label="
                                  << "\""  << cursor << "\"" << "]"
                                  << "\n"
                   << cursor.name() << "_" << rec << " -- "
                   << cursor.name() << "_" << rec << "_" << it
                   << "\n" ;

            ++it;
        } while( cursor.next( nt::item ) or
                (cursor.next( nt::rec ) and ((it = 0) or ++rec)) );

    } while( cursor.next( nt::kw ) );

    return stream << "}";
}

}

int main( int , char** argv ) {
    const std::string filename{ argv[ 1 ] };
    std::ifstream fs( filename );
    std::string input{ std::istreambuf_iterator< char >( fs ),
                       std::istreambuf_iterator< char >() };

    auto ast = lun::parse( input );
    dot( std::cout, ast.cursor() ) << std::endl;
}
