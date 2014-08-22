#ifndef APPFW_INIT
#define APPFW_INIT

#include <stdio.h>

#define MAX_COMMAND_LENGTH 65535

enum { APPFW_BLESSED, APPFW_TAINTED, APPFW_SECURITY_VIOLATION, APPFW_SECURITY_VIOLATION2, APPFW_BLESSED_KEYWORD, APPFW_UNKNOWN, APPFW_CRITICAL_TOKEN };

void appfw_init();               // load/initialize signature patterns off signature file (specified via env. variable)  
void appfw_init_from_file(const char *p_file);
int appfw_isInitialized();
int appfw_getNumSignatures();    // returns number of signature patterns
char **appfw_getSignatures();    // returns array containing signature patterns
void appfw_error(const char*);   // generic error display routine

void appfw_dump_signatures(FILE *);

void appfw_taint_range(char *taint, char taintValue, int from, int len); // set taint markings
void appfw_taint_range_by_pos(char *taint, char taintValue, int beg, int end); // set taint markings
void appfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint);
extern void appfw_establish_blessed(const char *input, char *taint, int case_sensitive); 
int appfw_establish_taint_fast2(const char *input, char *taint, int case_sensitive); 
void appfw_empty_taint(const char *command, char *taint);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif


