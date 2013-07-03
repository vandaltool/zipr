#define __USE_GNU
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <assert.h>

extern void **my_pgExec;
extern int PQexec(),
	intercept_sqlQuery, mysql_query(),
	intercept_sqlRealQuery, mysql_real_query(),
	intercept_sqlite3Query, sqlite3_exec(),
	my_system, 
	my_popen, 
	my_rcmd, rcmd(),
	my_execl, 
	my_execlp, 
	my_execle, 
	my_execv,  execv(), 
	my_execvp, execvp(),
	my_execvpe, execvpe(),
	my_execve,  execve(), 
	my_fexecve, fexecve(),
	my_SQLExecDirect, SQLExecDirect(),
	my_SQLPrepare, SQLPrepare();

int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg, ...);




void* (*real_dlsym)()=NULL;

struct mapper
{
	char* callback_name;
	void** callback_storage;
	void* new_sym; 
};
struct mapper dlsym_mapper[] = 
{
	{"PQexec", (void**)&my_pgExec, &PQexec},
	{"mysql_query", (void**)&intercept_sqlQuery, &mysql_query},
	{"mysql_real_query", (void**)&intercept_sqlRealQuery, &mysql_real_query},
	{"sqlite3_exec", (void**)&intercept_sqlite3Query, &sqlite3_exec},
	{"system", (void**)&my_system, &system},
	{"popen", (void**)&my_popen, &popen},
	{"rcmd", (void**)&my_rcmd, &rcmd},
	{"execl", (void**)&my_execl, &execl},
	{"execlp", (void**)&my_execlp, &execlp},
	{"execle", (void**)&my_execle, &execle},
	{"execv", (void**)&my_execv, &execv},
	{"execvp", (void**)&my_execvp, &execvp},
	{"execvpe", (void**)&my_execvpe, &execvpe},
	{"execve", (void**)&my_execve, &execve},
	{"fexecve", (void**)&my_fexecve, &fexecve},
	{"SQLExecDirect", (void**)&my_SQLExecDirect, &SQLExecDirect},
	{"SQLPrepare", (void**)&my_SQLPrepare, &SQLPrepare},
	{NULL,NULL,NULL}
};




void* dl_sym_helper(void* handle, const char* symbol, char* callback_name, void **callback_var, void* new_sym)
{
	assert(callback_var);
	if(strcmp(symbol, callback_name)==0)
	{
		if(getenv("APPFW_VERBOSE")!=0)
		{
			printf("Found match for %s\n", callback_name);
		}
                void* result = (void*)(*real_dlsym)(handle, symbol); /* now, this will call dlsym() library function */
		*callback_var=result;
                return new_sym;
	}
	if(getenv("APPFW_VERBOSE")!=0)
		printf("No match foundfor '%s'!='%s'\n", symbol,callback_name);
	return NULL;
}

extern void sqlfw_init();
extern void oscfw_init();
extern void xqfw_init();
extern void appfw_ldap_init();

void *dlsym(void *handle, const char *symbol)
{
	if(getenv("APPFW_VERBOSE")!=0)
	{
        	printf("Ha Ha...dlsym() Hooked with handle=%p, symbol=%s\n", (void*)handle,symbol);
	}
	if(real_dlsym==NULL)
	{
		void* handler = dlopen("libdl.so", RTLD_LAZY);
		assert(handler);
        	real_dlsym  = (void*)__libc_dlsym(handler, "dlsym"); /* now, this will call dlsym() library function */
		assert(real_dlsym);


		/* init it all */
		sqlfw_init();
		oscfw_init();
		xqfw_init();
		appfw_ldap_init();
	}

        if(handle!=RTLD_NEXT && handle!=RTLD_DEFAULT)
        {
		int i=0;
		while(dlsym_mapper[i].callback_name!=NULL)
		{
			void* res=dl_sym_helper(handle,symbol,
				dlsym_mapper[i].callback_name,
				dlsym_mapper[i].callback_storage,
				dlsym_mapper[i].new_sym
				);
			if(res) 
			{
				if(getenv("APPFW_VERBOSE")!=0)
					printf("Found match in hooker: old=%p new=%p", *dlsym_mapper[i].callback_storage, res);
				return res;
			}
			i++;
		}
		
	}
		
        void* result = (void*)(*real_dlsym)(handle, symbol); /* now, this will call dlsym() library function */
        return result;
}


