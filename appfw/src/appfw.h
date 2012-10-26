#ifndef APPFW_INIT
#define APPFW_INIT

#define MAX_COMMAND_LENGTH 2048

enum { APPFW_BLESSED = 0, APPFW_TAINTED = 1, APPFW_SECURITY_VIOLATION =2 };

extern void appfw_init();               // load/initialize signature patterns off signature file (specified via env. variable)  
extern int appfw_isInitialized();
extern int appfw_getNumSignatures();    // returns number of signature patterns
extern char **appfw_getSignatures();    // returns array containing signature patterns
extern void appfw_error(const char*);   // generic error display routine

extern void appfw_taint_range(char *taint, char taintValue, int from, int len); // mark as tainted
extern void appfw_establish_taint(const char *input, char *taint);              // return tainted portion of input string
extern void appfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint);

#endif
