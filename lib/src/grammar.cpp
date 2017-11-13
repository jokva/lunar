#include <algorithm>
#include <cctype>
#include <exception>
#include <forward_list>
#include <iterator>
#include <string>
#include <vector>

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#define BOOST_SPIRIT_DEBUG 1

#include <boost/fusion/include/std_pair.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

#include <filesystem/path.h>

#include <lunar/concatenate.hpp>

#include <lunar/parser.hpp>

namespace spirit    = boost::spirit;
namespace phx       = boost::phoenix;
namespace qi        = boost::spirit::qi;
namespace ascii     = boost::spirit::ascii;
namespace bf        = boost::fusion;

BOOST_FUSION_ADAPT_STRUCT( lun::section, name, xs )
BOOST_FUSION_ADAPT_STRUCT( lun::keyword, name, xs )
BOOST_FUSION_ADAPT_STRUCT( lun::item, repeat, ival, fval, sval )
BOOST_FUSION_ADAPT_STRUCT( lun::item::star, val )

namespace lun {

namespace {

template< typename Itr >
struct skipper : public qi::grammar< Itr > {
    skipper() : skipper::base_type( skip ) {
        skip = ascii::space
             | "--" >> *(qi::char_ - qi::eol)
        ;
    };

    qi::rule< Itr > skip;
};

template< typename Itr >
qi::rule< Itr, item::star() > star = qi::int_ >> '*';

static const auto empty_records = std::vector< record > {};

/*
 * keyword lookup through a qi::symbol table is pretty convenient, but that
 * only selects rule, it doesn't preserve the string. This nifty macro makes
 * will ensure the found keyword is synthesized properly. It also ensures that
 * a keyword is only a match if it matches the *complete* input, i.e. stops the
 * 'SYM' keyword would otherwise match the string 'SYMBOLS'
 */
#define kword(sym) qi::raw[qi::lexeme[(sym) >> !qi::alnum]]

template< typename Itr >
qi::rule< Itr, std::string() > str =
      '\'' >> *(qi::char_ - '\'') >> '\''
    | '"'  >> *(qi::char_ - '"')  >> '"'
    | qi::alpha >> *qi::alnum
;

template< typename T >
struct fortran_double : qi::real_policies< T > {
    // Eclipse supports Fortran syntax for specifying exponents of floating
    // point numbers ('D' and 'E', e.g., 1.234d5)
    template< typename It >
    static bool parse_exp( It& first, const It& last ) {
        if( first == last ||
            (*first != 'e' && *first != 'E' &&
            *first != 'd' && *first != 'D' ) )
            return false;
        ++first;
        return true;
    }
};

qi::real_parser< double, fortran_double< double > > f77float;

/* a typical deck contains a LOT more int-values than doubles, which means that
 * by checking for doubles first, we must essentially always backtrack, which
 * is somewhat expensive.
 *
 * TODO: figure out rule that allows valid ints to parse first that isn't
 * horribly complicated or expensive
 */

#define primary() (qi::int_ >> !qi::char_(".eEdD") | f77float)
#define term()    (qi::lit('/') >> qi::skip(qi::char_ - qi::eol)[qi::eps])

/*
 * The basic rule for parsing 1 record (aka list of items), including repeats
 *
 * The rule itself is variadically parametrised on a set of types, which allows
 * rules to share the same basic name, but restrict what types they accept.
 *
 * the rule <int> will only accept integers and defaults, and fail on doubles
 * and strings, and similarly for <double> and <string>. <int, double> will
 * fail encountering strings, but work for int and doubles. This is because a
 * lot of keywords accept only the one data type, and can report that error
 * early, where some keywords accept mixed int/double/string and will usually
 * be deferred to handle input errors at a later stage.
 *
 * It's motivated primarily by:
 * - most rules don't accept strings (and a lot don't accept doubles), so
 *   performance should improve slightly by having to consider less
 *   combinations (-> backtracking)
 * - we can report some type errors earlier
 * - the rule inputs become somewhat clearer
 */
template< typename Itr, typename... >
qi::rule< Itr, item() > itemrule;

/*
 * TODO: optimise backtracking patterns
 */
template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int > =
      qi::int_ >> !qi::lit('*')
    | star< Itr > >> qi::int_
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, double > =
      f77float >> !qi::lit('*')
    | star< Itr > >> qi::attr( 0 ) >> f77float
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, double > =
      primary() >> !qi::lit('*')
    | star< Itr > >> primary()
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, std::string > =
      str< Itr >[qi::_val = qi::_1]
    | (star< Itr > >> str< Itr >)
        [qi::_val = phx::construct< item >(qi::_1, qi::_2)]
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, std::string > =
      itemrule< Itr, int >
    | itemrule< Itr, std::string >
;

template< typename Itr >
qi::rule< Itr, item() > itemrule< Itr, int, double, std::string > =
      itemrule< Itr, int >
    | itemrule< Itr, double >
    | itemrule< Itr, std::string >
;

template< typename Itr, typename... T >
qi::rule< Itr, record(), skipper< Itr > > rec =
    *( itemrule< Itr, T... >
     | star< Itr >
     | '*'
     )
    >> term();

template< typename Itr >
struct runspec : qi::grammar< Itr, section(), skipper< Itr > > {
    template< typename T >
    using rule = qi::rule< Itr, T, skipper< Itr > >;

    runspec() : runspec::base_type( start ) {

        toggles  += "OIL", "WATER", "GAS", "DISGAS", "VAPOIL";
        toggles  += "METRIC", "FIELD", "LAB", "NOSIM", "UNIFIN", "UNIFOUT";

        singlei  += "DIMENS", "EQLDIMS", "REGDIMS", "WELLDIMS";
        singlei  += "VFPIDIMS", "VFPPDIMS", "FAULTDIM", "PIMTDIMS";
        singlei  += "NSTACK", "OPTIONS";

        singles  += "EQLOPTS", "SATOPTS";

        singleis += "ENDSCALE", "GRIDOPTS", "START", "TABDIMS";

        single_all += "TRACERS";

        start %= qi::string("RUNSPEC") >> *(
            kword(singlei)      >> rec< Itr, int >
          | kword(singlef)      >> rec< Itr, double >
          | kword(singles)      >> rec< Itr, std::string >
          | kword(singleis)     >> rec< Itr, int, std::string >
          | kword(single_all)   >> rec< Itr, int, double, std::string >
          | kword(toggles)  >> qi::attr( empty_records )
          )
        ;

        itemrule< Itr >.name( "item" );
        itemrule< Itr, int >.name( "item[int]" );
        itemrule< Itr, double >.name( "item[flt]" );
        itemrule< Itr, int, double >.name( "item[int|flt]" );
        itemrule< Itr, std::string >.name( "item[str]" );
        itemrule< Itr, int, std::string >.name( "item[int|str]" );
        itemrule< Itr, int, double, std::string >.name( "item[*]" );
    }

    qi::symbols<> singlei;
    qi::symbols<> singlef;
    qi::symbols<> singles;
    qi::symbols<> singleis;
    qi::symbols<> single_all;
    qi::symbols<> toggles;
    rule< section() > start;
};

template< typename Itr >
struct grid : qi::grammar< Itr, section(), skipper< Itr > > {
    grid() : grid::base_type( start ) {

        toggles += "NEWTRAN";

        singlei += "GRIDFILE";

        singlef += "MAPAXES";

        start %= qi::string("GRID") >> *(
            kword(singlei)      >> rec< Itr, int >
          | kword(singlef)      >> rec< Itr, double >
          | kword(toggles)      >> qi::attr( empty_records )
          )
        ;
    }

    qi::symbols<> toggles;
    qi::symbols<> singlei;
    qi::symbols<> singlef;
    qi::rule< Itr, section(), skipper< Itr > > start;
};

template< typename Itr >
struct deckp : qi::grammar< Itr, std::vector< section >(), skipper< Itr > > {
    deckp() : deckp::base_type( start ) {
        start %= (RUNSPEC | -RUNSPEC)
              >> (GRID    | -GRID)
              >> qi::eoi
        ;
    }

    runspec< Itr > RUNSPEC;
    grid< Itr > GRID;
    qi::rule< Itr, std::vector< section >(), skipper< Itr > > start;
};

#ifdef HAVE_BUILTIN_EXPECT
    #define likely(cond)   __builtin_expect(static_cast<bool>((cond)), 1)
    #define unlikely(cond) __builtin_expect(static_cast<bool>((cond)), 0)
#else
    #define likely(cond)   (cond)
    #define unlikely(cond) (cond)
#endif

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
            std::string included;
            auto ok = qi::phrase_parse( cursor, current.end,
                    "INCLUDE" >> str< Itr > >> term(),
                    skipper< Itr >(), included );

            if( !ok ) throw std::runtime_error( "Invalid INCLUDE" );

            filequeue.push_back( { cursor, current.end } );
            included = unixify( dir, aliases.resolve( included ) );
            input_files.push_back( included );
            filehandles.emplace_front( included );
            const auto& fh = filehandles.front();
            filequeue.push_back( { fh.begin(), fh.end() } );
        } else {
            std::vector< strpair > tmp_paths;
            auto ok = qi::phrase_parse( cursor, current.end,
                    "PATHS" >> *(str< Itr > >> str< Itr > >> term()) >> term(),
                    skipper< Itr >(), tmp_paths );

            if( !ok ) throw std::runtime_error( "Invalid PATHS" );

            aliases.insert( tmp_paths.begin(), tmp_paths.end() );
            filequeue.push_back( { cursor, current.end } );
        }
    }

    return { std::move( output ), std::move( input_files ) };
}


std::ostream& operator<<( std::ostream& stream, const item::tag& x ) {
    switch( x ) {
        case item::tag::i:   return stream << "int";
        case item::tag::f:   return stream << "float";
        case item::tag::str: return stream << "str";
        default:             return stream << "none";
    }
}

std::ostream& operator<<( std::ostream& stream, const item::star& s ) {
    return stream << int(s) << "*";
}

std::ostream& operator<<( std::ostream& stream, const item& x ) {
    stream << "{" << x.type << "|";
    if( x.repeat > 1 ) stream << x.repeat;

    switch( x.type ) {
        case item::tag::i:   stream << x.ival; break;
        case item::tag::f:   stream << x.fval; break;
        case item::tag::str: stream << x.sval; break;
        default:             stream << "_";
    }

    return stream << "}";
}

std::vector< section > parse( std::string::const_iterator fst,
                              std::string::const_iterator lst ) {

    using grm = deckp< std::string::const_iterator >;

    grm parser;
    std::vector< section > sec;

    auto ok = qi::phrase_parse( fst, lst, parser, skipper< decltype( fst ) >(), sec );
    if( !ok ) std::cerr << "PARSE FAILED" << std::endl;
    return sec;
}

}
