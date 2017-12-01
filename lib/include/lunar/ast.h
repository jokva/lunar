#ifndef LUN_AST_H
#define LUN_AST_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct lunAST* lun_ast;

lun_ast lun_parse_string( const char* begin, const char* end );
void lun_ast_dispose( const lun_ast );

typedef struct luncursor* lun_cursor;

lun_cursor luncur_make( const lun_ast );
lun_cursor luncur_copy( const lun_cursor );
void luncur_dispose( const lun_cursor );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //LUN_AST_H
