#ifndef SQLFW_INIT
#define SQLFW_INIT

#include "sql_structure.h"

#define MAX_QUERY_LENGTH 65536

#define S3_SQL_SAFE 0
#define S3_SQL_ATTACK_DETECTED 0x1
#define S3_SQL_PARSE_ERROR 0x2 
#define S3_SQL_ERROR 0x4 

extern void sqlfw_init();
extern void sqlfw_init_from_file(const char *p_file);
extern void sqlfw_save_query_structure_cache(const char *p_file);
extern int sqlfw_isInitialized();
extern int sqlfw_verify(const char *zSql, char **pzErrMsg);
extern int sqlfw_verify_fast(const char *zSql);
extern int sqlfw_verify_s(const char *zSql, char *p_annot);
extern int sqlfw_get_structure(const char *zSql, char *p_taint, char *p_structure);

extern int sqlfw_is_safe(int);
extern int sqlfw_is_error(int);
extern int sqlfw_is_parse_error(int);
extern int sqlfw_is_attack(int);

#endif
