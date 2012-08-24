#ifndef APPFW_INIT
#define APPFW_INIT

extern void appfw_init();               // load/initialize signature patterns off signature file (specified via env. variable)  
extern int appfw_isInitialized();
extern int appfw_getNumSignatures();    // returns number of signature patterns
extern char **appfw_getSignatures();    // returns array containing signature patterns
extern void appfw_error(const char*);   // generic error display routine

#endif
