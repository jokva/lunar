#include <iostream>
#include <iterator>

#include <lunar/parser.hpp>

int main( int argc, char** argv ) {
    if( argc != 2 ) {
        std::cout << "Usage: " << argv[ 0 ] << " INPUT\n";
        return 1;
    }

    auto il = lun::concatenate( argv[ 1 ] );
    std::copy( il.inlined.begin(), il.inlined.end(),
               std::ostream_iterator< char >( std::cout, "" ) );
}
