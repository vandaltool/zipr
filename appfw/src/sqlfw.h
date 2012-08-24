#ifndef SQLFW_INIT
#define SQLFW_INIT

#define MAX_QUERY_LENGTH 2048

extern void sqlfw_init();
extern int sqlfw_verify(const char *zSql, char **pzErrMsg);

#endif
