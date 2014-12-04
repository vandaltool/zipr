/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

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

typedef void* (*dlsym_t)(void*, const char*);
dlsym_t real_dlsym=NULL;

struct mapper
{
	char* callback_name;
	void** callback_storage;
	void* new_sym; 
	char* filename;
	void* handle;
	unsigned int group;
};
struct mapper dlsym_mapper[] = 
{
	{"PQexec", (void**)&my_pgExec, &PQexec, NULL, NULL, 1},
	{"PQexecParams", (void**)&my_pgExecParams, &PQexecParams, NULL, NULL, 1},
	{"PQprepare", (void**)&my_pgPrepare, &PQprepare, NULL, NULL, 1},
	{"mysql_query", (void**)&intercept_sqlQuery, &mysql_query, NULL, NULL, 2},
	{"mysql_real_query", (void**)&intercept_sqlRealQuery, &mysql_real_query, NULL, NULL, 2},
	{"mysql_stmt_prepare", (void**)&intercept_sqlStmtPrepare, &mysql_stmt_prepare, NULL, NULL, 2},
	{"sqlite3_exec", (void**)&intercept_sqlite3Query, &sqlite3_exec, NULL, NULL, 3},
	{"sqlite3_prepare", (void**)&intercept_sqlite3Prepare, &sqlite3_prepare, NULL, NULL, 3},
	{"sqlite3_prepare_v2", (void**)&intercept_sqlite3PrepareV2, &sqlite3_prepare_v2, NULL, NULL, 3},
	{"system", (void**)&my_system, &system, NULL, NULL, 0},
	{"popen", (void**)&my_popen, &popen, NULL, NULL, 0},
	{"rcmd", (void**)&my_rcmd, &rcmd, NULL, NULL, 0},
	{"execl", (void**)&my_execl, &execl, NULL, NULL, 0},
	{"execlp", (void**)&my_execlp, &execlp, NULL, NULL, 0},
	{"execle", (void**)&my_execle, &execle, NULL, NULL, 0},
	{"execv", (void**)&my_execv, &execv, NULL, NULL, 0},
	{"execvp", (void**)&my_execvp, &execvp, NULL, NULL, 0},
	{"execvpe", (void**)&my_execvpe, &execvpe, NULL, NULL, 0},
	{"execve", (void**)&my_execve, &execve, NULL, NULL, 0},
	{"fexecve", (void**)&my_fexecve, &fexecve, NULL, NULL, 0},
	{"SQLExecDirect", (void**)&my_SQLExecDirect, &SQLExecDirect, "libodbc.so.1", NULL, 4},
	{"SQLPrepare", (void**)&my_SQLPrepare, &SQLPrepare, "libodbc.so.1", NULL, 4},
	{NULL,NULL,NULL,NULL,NULL,0}
};


dlsym_t get_real_dlsym()
{
	char *verbose= getenv("APPFW_VERY_VERBOSE");
	if (real_dlsym == NULL)
	{
		if(verbose !=0)
        		fprintf(stderr, "Initing dlsym handle\n");
		void* handler = dlopen("libdl.so", RTLD_LAZY);
		assert(handler);
		extern void* __libc_dlsym(void*, const char*);
		real_dlsym  = (void*)__libc_dlsym(handler, "dlsym"); /* now, this will call dlsym() library function */
		assert(real_dlsym);

		if(verbose !=0)
		fprintf(stderr, "Finished initing dlsym handle\n");
	}
	return real_dlsym;
}


void* dl_sym_helper(void* handle, const char* symbol, struct mapper *sym_map)
{
	char* callback_name = sym_map->callback_name;
	void **callback_var = sym_map->callback_storage;
	void* new_sym = sym_map->new_sym;
	char* filename = sym_map->filename;
	char* handle_name=NULL;

	assert(callback_var);

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
		/* check if we already mapped this symbol from the same handle */
		if (*callback_var && handle == sym_map->handle)
			return new_sym;

		sym_map->handle = handle;
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

			// load all other symbols in the same group
			if (result && sym_map->group)
			{
				int i=0;
				while(dlsym_mapper[i].callback_name!=NULL)
				{
					if (dlsym_mapper[i].group == sym_map->group
					    && strcmp(dlsym_mapper[i].callback_name, symbol))
					{
						void *r = (void*)(*real_dlsym)(handle,
									       dlsym_mapper[i].callback_name);
						if (r)
						{
							*(dlsym_mapper[i].callback_storage) = r;
							dlsym_mapper[i].handle = handle;
						}
					}
					i++;
				}
			}
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
	char *verbose= getenv("APPFW_VERY_VERBOSE");
	static init = 0;
	if(verbose !=0)
        	fprintf(stderr, "Ha Ha...dlsym() Hooked with handle=%p, symbol=%s\n", (void*)handle,symbol);

	if(init == 0)
	{
		/* init it all */
		get_real_dlsym();
		sqlfw_init();
		oscfw_init();
		xqfw_init();
		appfw_ldap_init();
		init = 1;
	}

	if(handle!=RTLD_NEXT && handle!=RTLD_DEFAULT)
	{
		int i=0;
		while(dlsym_mapper[i].callback_name!=NULL)
		{
			void* res=dl_sym_helper(handle,symbol, dlsym_mapper+i);
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


