#ifndef APPFW_INIT
#define APPFW_INIT

extern void appfw_init();
extern int appfw_getNumSignatures();
extern char **appfw_getSignatures();
extern void appfw_error(const char*);

#endif
