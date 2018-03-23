#include <string>

#include <catch/catch.hpp>

#include <lunar/ast.hpp>

using namespace std::string_literals;

static const auto End = lun::ast::type::End;

namespace Catch {

template<>
struct StringMaker< lun::ast::type > {
    static std::string convert( const lun::ast::type& t ) {
    using type = lun::ast::type;
    switch( t ) {
        case type::Int:   return "int";
        case type::Float: return "float";
        case type::Str:   return "str";
        case type::None:  return "none";
        case type::End:   return "end";
        default:          return "???";
    }
}

};

}

SCENARIO( "cursors can be moved", "[cursor]" ) {
    const std::string input = R"(
RUNSPEC

EQLDIMS
    3*5 /

DIMENS
    5 2*10 /
)";

    GIVEN( "a valid, incomplete deck" ) {
        lun::ast ast;
        ast.define_keyword( "RUNSPEC"s, lun::ast::arity::none );
        ast.define_keyword( "EQLDIMS"s, lun::ast::arity::unary );
        ast.define_keyword( "DIMENS"s,  lun::ast::arity::unary );
        ast.read( input );

        auto cur = ast.cursor();

        WHEN( "a cursor is constructed" ) {
            THEN( "the keyword matches the first in the deck" ) {
                CHECK( cur.name()  == "RUNSPEC"s );
                CHECK( cur.records() == 0 );
                CHECK( cur.repeats() == -1 );
                //CHECK( cur.type() == End );
            }
        }

        auto cpy = cur;
        WHEN( "a cursor is copied" ) {
            THEN( "the keyword names match" ) {
                CHECK( cpy.name() == "RUNSPEC"s );
                CHECK( cur.name() == "RUNSPEC"s );
            }

            THEN( "the numbers of records match" ) {
                CHECK( cpy.records() == cur.records() );
            }
        }

        WHEN( "a copy is advanced" ) {
            auto ok = cpy.next( lun::ast::nodetype::kw );
            REQUIRE( ok );

            THEN( "the keyword is different" ) {
                CHECK( cpy.name()  == "EQLDIMS"s );
                CHECK( cpy.records() == 1 );
                CHECK( cpy.repeats() == 3 );
                CHECK( cpy.type() == lun::ast::type::Int );
            }

            THEN( "the original is unchanged" ) {
                CHECK( cur.name()  == "RUNSPEC"s );
                CHECK( cur.records() == 0 );
                CHECK( cur.repeats() == -1 );
                //CHECK( cur.type() == End );
            }
        }

        WHEN( "a single-item record is traversed per-item" ) {
            auto ok = cpy.next( lun::ast::nodetype::kw );
            REQUIRE( ok );

            THEN( "advancing the record cursor fails" ) {
                auto adv = cpy.next( lun::ast::nodetype::rec );
                CHECK( !adv );
            }

            THEN( "advancing the item cursor fails" ) {
                auto adv = cpy.next( lun::ast::nodetype::item );
                CHECK( !adv );
            }

            THEN( "advancing the keyword cursor succeeds" ) {
                auto adv = cpy.next( lun::ast::nodetype::kw );
                CHECK( adv );
            }
        }

        WHEN( "a multi-item record is traversed per-item" ) {
            auto ok = cpy.advance( lun::ast::nodetype::kw, 2 );
            REQUIRE( ok );
            REQUIRE( cpy.name() == "DIMENS"s );

            THEN( "advancing the record cursor fails" ) {
                auto adv = cpy.next( lun::ast::nodetype::rec );
                CHECK( !adv );
            }

            THEN( "the repeat and type are correct" ) {
                CHECK( cpy.repeats() == 0 );
                CHECK( cpy.type() == lun::ast::type::Int );
            }

            THEN( "advancing the item cursor succeeds" ) {
                auto adv = cpy.next( lun::ast::nodetype::item );
                CHECK( adv );
                CHECK( cpy.repeats() == 2 );
                CHECK( cpy.type() == lun::ast::type::Int );
            }
        }
    }
}
