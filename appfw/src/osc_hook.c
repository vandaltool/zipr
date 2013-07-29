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
  char taint[MAX_COMMAND_LENGTH];
  if (!my_system)
    my_system = dlsym(RTLD_NEXT, "system");

  oscfw_init(); // will do this automagically later 

  if(getenv("APPFW_VERBOSE"))
	fprintf(stderr, "In system\n");

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


int oscfw_verify_args(char* const argv[])
{
	int is_verbose=getenv("APPFW_VERBOSE")!=0;
  	char taint[MAX_COMMAND_LENGTH];
	int i=0;
	while(argv[i]!=NULL)
	{
		if(argv[i][0]=='-')
		{
			int length = strlen(argv[i]);
			matched_record** matched_signatures = appfw_allocate_matched_signatures(length);

			appfw_establish_taint(argv[i], taint, matched_signatures,TRUE);
			if(is_verbose)
        			appfw_display_taint("Debugging OS Command", argv[i], taint);

			int j;
			for(j=0;j<strlen(argv[i]);j++)
			{
                		if(taint[j]!=APPFW_BLESSED)
				{
					fprintf(stderr, "Failed argument check\n");
					appfw_display_taint("OS Command Injection detected", argv[i], taint);
					appfw_deallocate_matched_signatures(matched_signatures, length);
					return 0;
				}
			}

			appfw_deallocate_matched_signatures(matched_signatures, length);
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
  	if (!my_execve)
    		my_execve = dlsym(RTLD_NEXT, "execve");
	assert(my_execve);

  	oscfw_init(); // will do this automagically later

  	if (within_osc_monitor || (oscfw_verify(file, taint) && oscfw_verify_args(argv)) || getenv("DEBUG_APPFW"))
  	{
		if(getenv("VERBOSE"))
			fprintf(stderr, "Exec detected as OK\n");
		within_osc_monitor=TRUE;
        	int ret = my_execve(file,argv,envp);
		within_osc_monitor=FALSE;
        	return ret;
  	}
  	else
  	{

    		fprintf(stderr, "Failed argument check for handle_execl\n");
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

va_list process_args(char* arg, va_list vlist, char*** ret)
{
	*ret=malloc(0);
	int index;

	do
	{
		*ret=realloc(*ret,(index+1)*sizeof(void*));
		(*ret)[index++]=arg;
		/* test for exit if arg is 0. */
		if(arg==NULL)	
			return vlist;
		arg=(char*)va_arg(vlist,void*);

	} while(1);

	return vlist;
}

int (*my_execl)(const char *path, const char *arg, ...);
int execl(const char *path, const char *arg, ...)
{
	char **all_args=NULL;
	char **env=NULL;
	va_list vlist;
	va_start(vlist, arg);
	vlist=process_args((char*)arg,vlist,&all_args);
	env=environ;

	return handle_execl(path,all_args,env);
}

int (*my_execlp)(const char *file, const char *arg, ...);
int execlp(const char *file, const char *arg, ...)
{

	char **all_args;
	char **env;
	va_list vlist;
	va_start(vlist, arg);
	vlist=process_args((char*)arg,vlist,&all_args);
	env=environ;

	return handle_execp(file,all_args,env);
}

int (*my_execle)(const char *path, const char *arg, ...);
int execle(const char *path, const char *arg, ...)
{

	char **all_args;
	char **env;
	va_list vlist;
	va_start(vlist, arg);
	vlist=process_args((char*)arg,vlist,&all_args);
	env=va_arg(vlist,void*);

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





