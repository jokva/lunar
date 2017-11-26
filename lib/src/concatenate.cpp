#include <algorithm>
#include <forward_list>
#include <iterator>
#include <vector>

#include <boost/iostreams/device/mapped_file.hpp>

#include <filesystem/path.h>

#include <lunar/concatenate.hpp>
#include <lunar/parser.hpp>

#ifdef HAVE_BUILTIN_EXPECT
    #define likely(cond)   __builtin_expect(static_cast<bool>((cond)), 1)
    #define unlikely(cond) __builtin_expect(static_cast<bool>((cond)), 0)
#else
    #define likely(cond)   (cond)
    #define unlikely(cond) (cond)
#endif

namespace lun {

namespace {

template< typename Itr >
Itr search( Itr begin, Itr end ) {
    /*
     * The search function is essentially a static boyer-moore, adapted to work
     * on *two* patterns.
     *
     * inspect every len(PATHS) character and check if it's possible it is a
     * part of either PATHS or INCLUDE. If it's a match (rarely - norne is
     * approx 450 000 lines and 26M large and contain between 50 and 60
     * includes), rewind until the start of the word and simply check for a
     * match with no magic.
     *
     * Returns an iterator the *first* match in the sequence of either a PATHS
     * or an INCLUDE keyword that is not in a comment.
     */
    // TODO: noexcept?
    constexpr static const char
        P = 0, A = 1, T = 2, H = 3, S = 4,
        I = 0, N = 1, C = 2, L = 3, U = 4, D = 5, E = 6;

    constexpr static const char rewinds[21] = {
        A, 0, C, D, E, 0, 0, H,
        I, 0, 0, L, 0, N, 0, P,
        0, 0, S, T, U
    };

    const auto rskip = []( auto c ) { return rewinds[ c - 'A' ]; };
    const auto advance = []( auto& itr ) { return itr += 6; };

    const auto rend = std::make_reverse_iterator( begin );

    const auto candidate = [rend, begin, end]( auto fst ) {
        constexpr static const char paths[] = { 'A','T','H','S' };
        constexpr static const char incl[]  = { 'N','C','L','U','D','E' };

        if( *fst != 'P' && *fst != 'I' )
            return false;

        /* if less than 10 chars are left it's impossible to form a complete
         * INCLUDE or PATHS keyword, but it has to be checked not to read after
         * the end-of-file
         */
        if( std::distance( fst, end ) < 10 )
            return false;

        if( *fst == 'I'
          && !std::equal( std::begin( incl ), std::end( incl ), fst + 1 ) )
            return false;

        if( *fst == 'P'
          && !std::equal( std::begin( paths ), std::end( paths ), fst + 1 ) )
            return false;

        /*
         * definitely a match - now search backwards to determine if it's the
         * first non-blank thing on thise line. Otherwise, it's either
         * malformed or inside a comment, in which case just skip past it and
         * continue
         */

        /* start-of-file, i.e. no need to check if this a comment or dead */
        if( fst == begin ) return true;

        /* skip blank chars backwards */
        const auto rfst = std::make_reverse_iterator( fst );
        constexpr auto blank = [](unsigned char c) { return std::isblank(c); };
        return *std::find_if_not( rfst, rend, blank ) == '\n';
    };

    // immediately advanced by len+1, which could miss PATHS as the first input
    // so correct for it
    auto fst = begin - 2;

    /*
     * The end-of-file check is *very* unlikely (once per file, they tend to be
     * several megabytes) to ever be true, and some speed gains are measurable
     * when marking it unlikely. the problem is that advancing past the end
     * pointer is very much undefined, so sequencing is important. branch
     * predictor should get this most of the time.
     *
     * The checking if this character is a substring match, however, is *also*
     * very likely to be "false", i.e. not a match. This draws on the
     * observation that *most* of the input file is integers or floats, not one
     * of the 13 uppercase characters in PATHS and INCLUDE. In Norne, 0.6% of
     * all characters in the input deck would match.
     *
     * The switch was measured to be slightly faster than a 256-byte lookup
     * table and a std::bitset.
     */
    for( ;; ) {
        fst = advance( fst );
        if( unlikely(fst >= end) ) return end;

        switch( *fst ) {
            case 'P':
            case 'A':
            case 'T':
            case 'H':
            case 'S':

            case 'I':
            case 'N':
            case 'C':
            case 'L':
            case 'U':
            case 'D':
            case 'E':
                break;

            default:
                continue;
        }

        /* matches a partial, search backwards */
        const auto cur = fst - rskip( *fst );

        if( !candidate( cur ) ) continue;

        return cur;
    }
}

}

inlined concatenate( const std::string& path ) {
    static const int M = 1000000;
    std::vector< char > output;
    output.reserve( 50 * M );

    using Itr = const char*;
    using mmapd = boost::iostreams::mapped_file_source;
    using strpair = std::pair< std::string, std::string >;
    struct fv { Itr begin, end; };

    const auto dir = filesystem::path( path ).parent_path();

    pathresolver aliases;
    std::forward_list< mmapd > filehandles = { mmapd{ path } };
    std::vector< std::string > input_files = { path };
    std::vector< fv > filequeue = { { filehandles.front().begin(),
                                      filehandles.front().end() } };

    const auto unixify = []( const auto& prefix, const auto& x ) {
        /*
         * paths, both expanded from PATHS and from INCLUDE can contain
         * backslashes as dir separators. reinterpret it to only use / as a
         * separator, since windows handles that just fine
         *
         * Probably a tad inefficient, but is only called once per include, of
         * which there are only a handful
         */
        filesystem::path include( x );
        if( !include.is_absolute() )
            include = prefix / include;
        include.set( include.str(), filesystem::path::windows_path );
        return include.str();
    };

    while( !filequeue.empty() ) {
        const auto& current = filequeue.back();
        filequeue.pop_back();

        auto cursor = search( current.begin, current.end );
        output.insert( output.end(), current.begin, cursor );

        /* file exhausted - nothing more to do */
        if( cursor == current.end ) {
            filehandles.pop_front();
            continue;
        }

        if( *cursor == 'I' ) {
            auto included = INCLUDE( cursor, current.end );

            filequeue.push_back( { cursor, current.end } );
            included = unixify( dir, aliases.resolve( included ) );
            input_files.push_back( included );
            filehandles.emplace_front( included );
            const auto& fh = filehandles.front();
            filequeue.push_back( { fh.begin(), fh.end() } );
        } else {
            auto tmp_paths = PATHS( cursor, current.end );
            aliases.insert( tmp_paths.begin(), tmp_paths.end() );
            filequeue.push_back( { cursor, current.end } );
        }
    }

    return { std::move( output ), std::move( input_files ) };
}

}
