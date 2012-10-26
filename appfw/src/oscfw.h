#ifndef OSCFW_INIT
#define OSCFW_INIT

#include "appfw.h"

extern void oscfw_init();
extern int oscfw_isInitialized();
extern int oscfw_verify(const char *p_cmd, const char *p_taint);

#endif
