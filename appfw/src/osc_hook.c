#define _GNU_SOURCE
#include <stdio.h>
/*
#include <stdint.h>
*/
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <assert.h>

#define FALSE 0
#define TRUE 0

extern char **environ;

int within_osc_monitor=FALSE;

int (*my_system)(const char *) = NULL;
#include "oscfw.h"
int system(const char *p_command)
{
  if(getenv("APPFW_VERBOSE"))
	fprintf(stderr, "In system\n");

  char taint[MAX_COMMAND_LENGTH];
  if (!my_system)
    my_system = dlsym(RTLD_NEXT, "system");

  oscfw_init(); // will do this automagically later 


  if (within_osc_monitor || oscfw_verify(p_command, taint))
  {
	if(getenv("APPFW_VERBOSE"))
    		appfw_display_taint("OS Command Injection safe", p_command, taint);
	within_osc_monitor=TRUE;
    	int ret = my_system(p_command);
	within_osc_monitor=FALSE;
	return ret;
  }
  else
  {
	if(getenv("APPFW_VERBOSE"))
    		appfw_display_taint("OS Command Injection detected", p_command, taint);

    	return -1; // error code for system
  }
}

FILE* (*my_popen)(const char *, const char*) = NULL;
FILE* popen(const char *p_command, const char* p_type)
{
  char taint[MAX_COMMAND_LENGTH];
  if (!my_popen)
    my_popen = dlsym(RTLD_NEXT, "popen");

  oscfw_init(); // will do this automagically later 

  if (within_osc_monitor || oscfw_verify(p_command, taint))
  {
	within_osc_monitor=TRUE;
    	FILE* ret = my_popen(p_command,p_type);
	within_osc_monitor=FALSE;
	return ret;
  }
  else
  {
	if(getenv("APPFW_VERBOSE"))
    		appfw_display_taint("OS Command Injection detected", p_command, taint);
    	return 0; // error code for popen
  }
}

int (*my_rcmd)(char **ahost, int inport, const char *locuser,
                const char *remuser, const char *cmd, int *fd2p) = NULL;
int rcmd(char **ahost, int inport, const char *locuser,
                const char *remuser, const char *cmd, int *fd2p)
{
  char taint[MAX_COMMAND_LENGTH];
  if (!my_rcmd)
    my_rcmd = dlsym(RTLD_NEXT, "rcmd");

  oscfw_init(); // will do this automagically later 

  if (within_osc_monitor || oscfw_verify(cmd, taint))
  {
	within_osc_monitor=TRUE;
    	int ret = my_rcmd(ahost,inport,locuser,remuser,cmd,fd2p);
	within_osc_monitor=FALSE;
	return ret;
  }
  else
  {
	if(getenv("APPFW_VERBOSE"))
    		appfw_display_taint("OS Command Injection detected", cmd, taint);
    	return -1; // error code for rcmd
  }
}


// used for execl for example
// goal: verify all flag options come from the same signature
int oscfw_verify_args(char* const argv[])
{
	int is_verbose=getenv("APPFW_VERBOSE")!=0;
  	char taint[MAX_COMMAND_LENGTH];
  	char cmd[MAX_COMMAND_LENGTH];
	int i=0;

#ifdef DEBUG
	while(argv[i]!=NULL)
	{
		fprintf(stderr, "arg: %s\n", argv[i]);

		i++;
	}

	i = 0;
#endif
	while(argv[i]!=NULL)
	{
		if(argv[i][0]=='-')
		{
			// e.g., we need to verify that: -lta originates from a single fragment
			// 
			// create a fake command consisting of just the flag
			// mark all characters as security violations
			// run it through appfw_establish_taint2
			// and look at return value
			//
			//	cmd: -lta
			//    taint: vvvv
			//
			// after appfw_establish_taint2, taint markings should be:
			//    taint: bbbb
	
			int length = strlen(argv[i]);

			strcpy(cmd, argv[i]);
			appfw_taint_range(taint, APPFW_SECURITY_VIOLATION, 0, length);
			int success = appfw_establish_taint_fast2(cmd, taint, FALSE, FALSE);

			if (!success)
				appfw_display_taint("OS Command Injection detected (options): ", argv[i], taint);

			return success;
		}
		else 
		{
			if((strcmp(argv[0],"/bin/sh")==0 || strstr(argv[0],"bash")!=0) 
				&& i>0 
				&& strcmp(argv[i-1],"-c")==0)
			{
				int OK=oscfw_verify(argv[i], taint);
				if(is_verbose && OK)
				{
					fprintf(stderr,"Detected '%s' as non-argument to command '%s'\n", 
						argv[i], argv[0]);
				}
				else if(is_verbose)
				{
					fprintf(stderr,"Detected '%s' as argument to command '%s'\n", 
						argv[i], argv[0]);
				}
				if(!OK)
					return 0;
			
			}
			else if(is_verbose)
			{
				fprintf(stderr,"Detected '%s' as non-argument to command '%s'\n", 
					argv[i], argv[0]);
			}
		}
		i++;
	}
	return 1;
}

int (*my_execve)(const char*,char*const[], char*const[])=NULL;
int handle_execl(const char *file, char *const argv[], char *const envp[])
{
  	char taint[MAX_COMMAND_LENGTH];
	int cmd_verify, args_verify;

  	if (!my_execve)
    		my_execve = dlsym(RTLD_NEXT, "execve");
	assert(my_execve);

  	oscfw_init(); // will do this automagically later

  	if (within_osc_monitor || ((cmd_verify = oscfw_verify(file, taint)) && (args_verify = oscfw_verify_args(argv))) || getenv("DEBUG_APPFW"))
  	{
		if(getenv("APPFW_VERBOSE"))
			fprintf(stderr, "Exec detected as OK\n");
		within_osc_monitor=TRUE;
        	int ret = my_execve(file,argv,envp);
		within_osc_monitor=FALSE;
        	return ret;
  	}
  	else
  	{
		if (!cmd_verify)
	    		fprintf(stderr, "Failed argument check for handle_execl: command verification failed\n");
		if (!args_verify)
	    		fprintf(stderr, "Failed argument check for handle_execl: argument verification failed\n");
    		appfw_display_taint("OS Command Injection detected", file, taint);
    		return -1; // error code for rcmd
  	}
}

int (*my_fexecve)(int,char*const [], char*const [])=NULL;
int handle_fexec(int fd, char *const argv[], char *const envp[])
{
  	char taint[MAX_COMMAND_LENGTH];
  	if (!my_fexecve)
    		my_fexecve = dlsym(RTLD_NEXT, "fexecve");
  	assert(my_fexecve);

  	oscfw_init(); // will do this automagically later

  	if (within_osc_monitor || oscfw_verify_args(argv))
  	{
		within_osc_monitor=TRUE;
        	int ret = my_fexecve(fd,argv,envp);
		within_osc_monitor=FALSE;
        	return ret;
  	}
  	else
  	{
    		fprintf(stderr, "Failed argument check for handle_fexec\n");
    		return -1; // error code for rcmd
  	}
}

int (*my_execvpe)(const char*,char*const [], char*const [])=NULL;
int handle_execp(const char *file, char *const argv[], char *const envp[])
{
  	char taint[MAX_COMMAND_LENGTH];
  	if (!my_execvpe)
    		my_execvpe = dlsym(RTLD_NEXT, "execvpe");
	assert(my_execvpe);
	
  	oscfw_init(); // will do this automagically later

  	if (within_osc_monitor || (oscfw_verify(file, taint) && oscfw_verify_args(argv)) || getenv("DEBUG_APPFW"))
  	{
		within_osc_monitor=TRUE;
        	int ret = my_execvpe(file,argv,envp);
		within_osc_monitor=FALSE;
        	return ret;
  	}
  	else
  	{
    		fprintf(stderr, "Failed argument check for execp\n");
    		appfw_display_taint("OS Command Injection detected", file, taint);
    		return -1; // error code for rcmd
  	}
}

void process_args(char* arg, va_list *vlist, char*** ret)
{
	*ret=malloc(0);
	int index=0;

//fprintf(stderr, "In proc_args, arg=%s\n", arg);
	do
	{
		*ret=realloc(*ret,(index+1)*sizeof(void*));
		(*ret)[index++]=arg;
//fprintf(stderr, "In proc_args, *ret[0]=%s, arg=%s\n", (*ret)[0], arg);
		/* test for exit if arg is 0. */
		if(arg==NULL)	
			return;
		arg=(char*)va_arg(*vlist,void*);

	} while(1);

	return;
}

int (*my_execl)(const char *path, const char *arg, ...);
int execl(const char *path, const char *arg, ...)
{
	char **all_args=NULL;
	char **env=NULL;
	va_list vlist;
	va_start(vlist, arg);
	process_args((char*)arg,&vlist,&all_args);
	env=environ;
//		fprintf(stderr, "args[0]=%s\n", all_args[0]);

	return handle_execl(path,all_args,env);
}

int (*my_execlp)(const char *file, const char *arg, ...);
int execlp(const char *file, const char *arg, ...)
{

	char **all_args;
	char **env;
	va_list vlist;
	va_start(vlist, arg);
	process_args((char*)arg,&vlist,&all_args);
	env=environ;
// 		fprintf(stderr, "args[0]=%s\n", all_args[0]);

	return handle_execp(file,all_args,env);
}

int (*my_execle)(const char *path, const char *arg, ...);
int execle(const char *path, const char *arg, ...)
{

	char **all_args;
	char **env;
	va_list vlist;
	va_start(vlist, arg);
	process_args((char*)arg,&vlist,&all_args);
	env=va_arg(vlist,void*);

//	if(getenv("APPFW_VERBOSE"))
// 		fprintf(stderr, "args[0]=%s, arg=%s\n", all_args[0], arg);
	return handle_execl(path,all_args,env);
}

int (*my_execv)(const char *path, char *const argv[]);
int execv(const char *path, char *const argv[])
{
	return handle_execl(path,argv,environ);
}

int (*my_execvp)(const char *file, char *const argv[]);
int execvp(const char *file, char *const argv[])
{
	return handle_execp(file,argv,environ);
}

int (*my_execvpe)(const char *file, char *const argv[], char *const envp[]);
int execvpe(const char *file, char *const argv[], char *const envp[])
{
	return handle_execp(file,argv,envp);
}

int (*my_execve)(const char *filename, char *const argv[], char *const envp[]);
int execve(const char *filename, char *const argv[], char *const envp[])
{
	return handle_execl(filename,argv,envp);
}

//int (*my_fexecve(int fd, char *const argv[], char *const envp[]));
int fexecve(int fd, char *const argv[], char *const envp[])
{
	return handle_fexec(fd,argv,envp);
}





