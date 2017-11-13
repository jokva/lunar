#include <iostream>
#include <iterator>

#include <time.h>

#include <lunar/parser.hpp>

int main( int argc, char** argv ) {
    if( argc != 3 ) {
        std::cout << "Usage: " << argv[ 0 ] << " INPUT\n";
        return 1;
    }

    const auto iterations = std::atoi( argv[ 1 ] );

    timespec start, stop;
    clock_gettime( CLOCK_REALTIME, &start );

    int sideeffect = 0;
    for( int i = 0; i < iterations; ++i ) {
        auto il = lun::concatenate( argv[ 2 ] );
        sideeffect += il.inlined[ 0 ];
    }

    clock_gettime( CLOCK_REALTIME, &stop );

    double total = ( stop.tv_sec - start.tv_sec )
                 + ( stop.tv_nsec - start.tv_nsec )
                 / 1e9;
    double average = total / iterations;

    std::cout << "Result: " << sideeffect << "\n"
              << "Iterations: " << iterations << "\n"
              << "Total time: " << total << "\n"
              << "Average: " << average << "\n"
              ;
}

