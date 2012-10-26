#define _GNU_SOURCE
#include <stdio.h>
/*
#include <stdint.h>
#include <string.h>
*/
#include <stdlib.h>
#include <dlfcn.h>

#include "oscfw.h"
int system(const char *p_command)
{
  char taint[MAX_COMMAND_LENGTH];
  static int (*my_system)(const char *) = NULL;
  if (!my_system)
    my_system = dlsym(RTLD_NEXT, "system");

  oscfw_init(); // will do this automagically later 

  if (oscfw_verify(p_command, taint))
  {
    int ret = my_system(p_command);
	return ret;
  }
  else
  {
#ifdef SHOW_TAINT_MARKINGS
    appfw_display_taint("OS Command Injection detected", p_command, taint);
#endif

    return -1; // error code for system
  }
}

FILE* popen(const char *p_command, const char* p_type)
{
  char taint[MAX_COMMAND_LENGTH];
  static int (*my_popen)(const char *, const char*) = NULL;
  if (!my_popen)
    my_popen = dlsym(RTLD_NEXT, "popen");

  oscfw_init(); // will do this automagically later 

  if (oscfw_verify(p_command, taint))
  {
    int ret = my_popen(p_command,p_type);
	return ret;
  }
  else
  {
#ifdef SHOW_TAINT_MARKINGS
    appfw_display_taint("OS Command Injection detected", p_command, taint);
#endif

    return 0; // error code for popen
  }
}
