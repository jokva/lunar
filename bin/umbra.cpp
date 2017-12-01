#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <lunar/parser.hpp>

struct is_end : boost::static_visitor< bool > {
    template< typename T >
    bool operator()( T ) const { return false; }
    bool operator()( lun::item::endrec ) const { return true; }
};

std::string lun::dot( const std::vector< keyword >& kws ) {
    std::stringstream stream;

    stream << "strict graph {" << std::endl;

    for( const auto& kw : kws ) {

        stream << "root -- ";
        stream << kw.name << "[shape=box]" << "\n";

        if( kw.xs.empty() ) continue;

        int rec = 0;
        int it = 0;

        stream << "\t"
               << kw.name << "_" << rec << "[label=" << rec << "]"
               << kw.name << " -- "
               << kw.name << "_" << rec
               << "\n";

        for( const auto& item : kw.xs ) {
            if( boost::apply_visitor( is_end(), item.val ) ) {
                rec += 1;
                it = 0;

                stream << "\t"
                       << kw.name << "_" << rec << "[label=" << rec << "]"
                       << kw.name << " -- "
                       << kw.name << "_" << rec
                       << "\n";
                continue;
            }

            stream << "\t\t"
                   << kw.name << "_" << rec << "_" << it
                              << "[shape=record, label="
                              << "\"" << item << "\"" << "]"
                              << "\n"
                   << kw.name << "_" << rec << " -- "
                   << kw.name << "_" << rec << "_" << it
                              << "\n"
            ;

            it += 1;
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

    auto sec = lun::parse( input );

    std::cout << lun::dot( sec ) << std::endl;
}
