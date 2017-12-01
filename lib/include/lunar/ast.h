#ifndef LUN_AST_H
#define LUN_AST_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum lun_errors {
    LUN_OK = 0,
    LUN_OUT_OF_RANGE,
    LUN_NOTYPE,
};

typedef struct lunAST* lun_ast;

lun_ast lun_parse_string( const char* begin, const char* end );
void lun_ast_dispose( const lun_ast );

typedef struct luncursor* lun_cursor;

lun_cursor luncur_make( const lun_ast );
lun_cursor luncur_copy( const lun_cursor );
void luncur_dispose( const lun_cursor );

const char* luncur_kwname( const lun_cursor );
int luncur_records( const lun_cursor );

/*
 * Advance the cursor along a keyword, record or item.
 * Returns LUN_OK if the advance was successful, returns LUN_OUT_OF_RANGE if
 * there is no such sibling.
 *
 * When a record is exhausted, the iterator is NOT advanced to the next
 * keyword, but will return LUN_OUT_OF_RANGE
 *
 * If the type is invalid, it will return LUN_NOTYPE
 */
enum lun_node_type { LUN_KW, LUN_REC, LUN_ITEM };
int luncur_advance( lun_cursor, int type, int steps );
int luncur_next( lun_cursor, int type );
int luncur_prev( lun_cursor, int type );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //LUN_AST_H
