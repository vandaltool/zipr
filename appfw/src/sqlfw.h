#ifndef SQLFW_INIT
#define SQLFW_INIT

#define MAX_QUERY_LENGTH 65536

extern void sqlfw_init();
extern int sqlfw_isInitialized();
extern int sqlfw_verify(const char *zSql, char **pzErrMsg);
extern int sqlfw_verify_fast(const char *zSql);
extern int sqlfw_verify_s(const char *zSql, char *p_structure);
extern int sqlfw_get_structure(const char *zSql, char *p_taint);

#endif
