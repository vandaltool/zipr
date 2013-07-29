#define __USE_GNU
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <assert.h>

extern void **my_pgExec;
extern void **my_pgExecParams;
extern void **my_pgPrepare;

extern int PQexec(), PQexecParams(), PQprepare(),
	intercept_sqlQuery, mysql_query(),
	intercept_sqlRealQuery, mysql_real_query(),
	intercept_sqlStmtPrepare, mysql_stmt_prepare(),
	intercept_sqlite3Query, sqlite3_exec(),
	intercept_sqlite3Prepare, sqlite3_prepare(),
	intercept_sqlite3PrepareV2, sqlite3_prepare_v2(),
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
	char* filename;
};
struct mapper dlsym_mapper[] = 
{
	{"PQexec", (void**)&my_pgExec, &PQexec, NULL},
	{"PQexecParams", (void**)&my_pgExecParams, &PQexecParams, NULL},
	{"PQprepare", (void**)&my_pgPrepare, &PQprepare, NULL},
	{"mysql_query", (void**)&intercept_sqlQuery, &mysql_query, NULL},
	{"mysql_real_query", (void**)&intercept_sqlRealQuery, &mysql_real_query, NULL},
	{"sqlite3_exec", (void**)&intercept_sqlite3Query, &sqlite3_exec, NULL},
	{"sqlite3_prepare", (void**)&intercept_sqlite3Prepare, &sqlite3_prepare, NULL},
	{"sqlite3_prepare_v2", (void**)&intercept_sqlite3PrepareV2, &sqlite3_prepare_v2, NULL},
	{"system", (void**)&my_system, &system, NULL},
	{"popen", (void**)&my_popen, &popen, NULL},
	{"rcmd", (void**)&my_rcmd, &rcmd, NULL},
	{"execl", (void**)&my_execl, &execl, NULL},
	{"execlp", (void**)&my_execlp, &execlp, NULL},
	{"execle", (void**)&my_execle, &execle, NULL},
	{"execv", (void**)&my_execv, &execv, NULL},
	{"execvp", (void**)&my_execvp, &execvp, NULL},
	{"execvpe", (void**)&my_execvpe, &execvpe, NULL},
	{"execve", (void**)&my_execve, &execve, NULL},
	{"fexecve", (void**)&my_fexecve, &fexecve, NULL},
	{"SQLExecDirect", (void**)&my_SQLExecDirect, &SQLExecDirect, "libodbc.so.1"},
	{"SQLPrepare", (void**)&my_SQLPrepare, &SQLPrepare, "libodbc.so.1"},
	{NULL,NULL,NULL}
};




void* dl_sym_helper(void* handle, const char* symbol, char* callback_name, void **callback_var, void* new_sym, 
	char* filename)
{
	assert(callback_var);
	char* handle_name=NULL;
	/* if we have no handle name, assume we can intercept 
	 * if we DO have a handle name, gather it.
	 */
	if(handle!=RTLD_NEXT && handle!=RTLD_DEFAULT && filename!=NULL)
		/* this is a bit hackish, but headerfiles don't have the 
		 * structure for a handle, but it seems to be at handle+4
		 */
		handle_name=*(char**)(handle+4);

	/* If we've found the right symbol */
	if(strcmp(symbol, callback_name)==0)
	{
		//printf("Handle filename is %s, filename=%s\n", handle_name, filename);
		/* and we're not doing symbol matching, or we've matched the symbol */
 		if(handle_name==NULL || strstr(handle_name,filename)!=NULL)
		{
			if(getenv("APPFW_VERBOSE")!=0)
			{
				fprintf(stderr,"Handle filename is %s\n", handle_name);
				fprintf(stderr,"Found match for %s\n", callback_name);
			}
			/* now, this will call dlsym() library function */
			void* result = (void*)(*real_dlsym)(handle, symbol); 

			*callback_var=result;
			return new_sym;
		}
	}
//	if(getenv("APPFW_VERBOSE")!=0)
//		printf("No match found for '%s'!='%s'\n", symbol,callback_name);
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
        	fprintf(stderr,"Ha Ha...dlsym() Hooked with handle=%p, symbol=%s\n", (void*)handle,symbol);
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
				dlsym_mapper[i].new_sym,
				dlsym_mapper[i].filename
				);
			if(res) 
			{
				if(getenv("APPFW_VERBOSE")!=0)
					fprintf(stderr,"Found match in hooker: old=%p new=%p", *dlsym_mapper[i].callback_storage, res);
				return res;
			}
			i++;
		}
		
	}
		
	void* result = (void*)(*real_dlsym)(handle, symbol); /* now, this will call dlsym() library function */
	return result;
}


