#ifndef LUNAR_CONCATENATE
#define LUNAR_CONCATENATE

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace lun {

class pathresolver {

public:
    using kv = std::pair< std::string, std::string >;

    std::string resolve( const std::string& key ) {
        /*
         * Searching in reverse order and always appending we get set-like
         * behaviour (since later entries would be found first).
         *
         * Paths aliases are few, and likely to be created not long before
         * they're used, so searching from the back should give fewer
         * comparisons (and regardless of method - a trivial amount)
         */

        std::string path;

        auto sigil_end = []( char c ) {
            return c == '/' || c == '\\' || c == '$';
        };

        auto begin = key.begin();
        const auto end = key.end();
        for( ;; ) {
            const auto fst = std::find( begin, end, '$' );

            path.insert( path.end(), begin, fst );
            if( fst == end ) return path;

            const auto lst = std::find_if( fst + 1, end, sigil_end );

            /* store without the leading $ */
            std::string alias( fst + 1, lst );

            auto matches = [&alias]( const auto& x ) {
                return x.first == alias;
            };

            const auto itr = std::find_if( this->aliases.rbegin(),
                                           this->aliases.rend(),
                                           matches );

            if( itr == this->aliases.rend() )
                throw std::runtime_error( "Unable to substitute alias " + key );

            path.append( itr->second );

            begin = lst;
            if( begin == end ) return path;
        }
    }

    template< typename Itr >
    void insert( Itr fst, Itr lst ) {
        auto end = this->aliases.end();
        this->aliases.insert( end, fst, lst );
    };

private:
    std::vector< kv > aliases;
};

}

#endif // LUNAR_CONCATENATE
