#ifndef SQLFW_INIT
#define SQLFW_INIT

#define MAX_QUERY_LENGTH 2048

extern void sqlfw_init();
extern int sqlfw_isInitialized();
extern int sqlfw_verify(const char *zSql, char **pzErrMsg);
//extern void sqlfw_establish_taint(const char *query, char *taint);
//extern int sqlfw_verify_taint(const char *zSql, char*,  char **pzErrMsg);
//extern void sqlfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint);

#endif
