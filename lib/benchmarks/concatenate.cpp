#include <iostream>
#include <numeric>
#include <iterator>

#include <ctime>

#include <lunar/parser.hpp>

int main( int argc, char** argv ) {
    if( argc != 3 ) {
        std::cout << "Usage: " << argv[ 0 ] << " INPUT\n";
        return 1;
    }

    const auto iterations = std::atoi( argv[ 1 ] );

    timespec start, stop;
    std::vector< double > timings;
    timings.reserve( iterations );

    for( int i = 0; i < iterations; ++i ) {
        clock_gettime( CLOCK_REALTIME, &start );
        auto il = lun::concatenate( argv[ 2 ] );
        clock_gettime( CLOCK_REALTIME, &stop );

        double duration = ( stop.tv_sec - start.tv_sec )
            + ( stop.tv_nsec - start.tv_nsec )
            / 1e9;

        timings.push_back( duration );
    }

    std::sort( timings.begin(), timings.end() );

    double total = std::accumulate( timings.begin(), timings.end(), 0.0 );
    double average = total / iterations;
    double mode = -1;
    int count = -1;

    for( int i = 0; i < iterations; ++i ) {
        const auto current = timings[i];
        const auto start   = i;

        /* within 0.1ms == equal */
        while( (timings[i] - current) < 1e-4 && i < iterations )
            ++i;

        const auto countMode = i - start;

        if( countMode > count ) {
            mode = current;
            count = countMode;
        }
    }

    std::cout << "Iterations: " << iterations << "\n"
              << "Total time: " << total << "\n"
              << "Average: " << average << "\n"
              << "Mode: " << mode << "\n"
              ;
}
