#ifndef LDAP_INIT
#define LDAP_INIT

#include "appfw.h"

extern void appfw_ldap_init();
extern int appfw_ldap_isInitialized();
extern int appfw_ldap_verify(const char *p_cmd);

#endif
