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


#include "PNRegularExpressions.hpp"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <assert.h>
#include "irdb-core"

using namespace std;

#define FIRN(s) fill_in_reg_name((s))
#define HEXNUM "([0][xX][0123456789abcdefABCDEF]+)|([01234356789]+)"
#define REGSTRING "[[:blank:]]*[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]+[[:blank:]]*"
#define SCALE "[[:blank:]]*[*][[:blank:]]*[1248][[:blank:]]*"
#define WS "[[:blank:]]*"

static char* fill_in_reg_name(const char *instring)
{

	int width=IRDB_SDK::FileIR_t::getArchitectureBitWidth();
	static char outstring[200];

	assert(strlen(instring)<sizeof(outstring));

	strcpy(outstring,instring);
	
	char* p=outstring;
	while((p=strstr(p,"%"))!=NULL)
	{
		assert(*p=='%');
		assert(*(p+1)=='b' || *(p+1)=='s');	 // sanity check that it's %sp or %bp
		assert(*(p+2)=='p');
		if(width==32)
			*p='e';
		else if(width==64)
			*p='r';
		else
			assert(0),abort();
	}
	return outstring;
}

//TODO: for now the constructor exits the program in compilation of the regex fails
//Is throwing an exception a better option?
PNRegularExpressions::PNRegularExpressions()
{
	int errcode;
  
	// match "and esp, *"
	if (regcomp(&regex_and_esp, FIRN("[[:blank:]]*and[[:blank:]]+%sp[[:blank:]]*,[[:blank:]]*(.+)[[:blank:]]*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for and esp to compile\n");
		exit(1);
	}
	// match "ret"
	if (regcomp(&regex_ret, FIRN("^ret[[:blank:]]*$"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for ret failed to compile\n");
		exit(1);
	}

	/* match lea <anything> dword [<stuff>]*/
	if (regcomp(&regex_lea_hack, FIRN("(.*lea.*,.*)dword(.*)"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for lea hack failed to compile\n");
		exit(1);
	}

	// match "[esp]"	
	if(regcomp(&regex_esp_only, FIRN(".*\\[(%sp)\\].*"),REG_EXTENDED | REG_ICASE) !=0)
	{
		fprintf(stderr,"Error: regular expression for esp scaled addresses failed\n");
		exit(1);
	}

	// match "[esp+reg*scale+disp]"	
	if(regcomp(&regex_esp_scaled, FIRN(".*\\[%sp" WS "[+].*[+](.+)\\].*"),REG_EXTENDED | REG_ICASE) !=0)
	{
		fprintf(stderr,"Error: regular expression for esp scaled addresses failed\n");
		exit(1);
	}
	if((errcode=regcomp(&regex_lea_rsp, FIRN(".*lea.*\\[.*%sp.*(" HEXNUM ").*\\].*"),REG_EXTENDED | REG_ICASE)) !=0)
	{
		char buf[1000];
		regerror(errcode,&regex_lea_rsp,buf,sizeof(buf));
		fprintf(stderr,"Error: regular expression for regex_lea_rsp failed, code: %s\n", buf);
		exit(1);
	}
	if((errcode=regcomp(&regex_esp_scaled_nodisp, FIRN(".*\\[%sp" WS "[+]" WS "" REGSTRING SCALE "(\\]).*"),REG_EXTENDED | REG_ICASE)) !=0)
	{
		char buf[1000];
		regerror(errcode,&regex_esp_scaled_nodisp,buf,sizeof(buf));
		fprintf(stderr,"Error: regular expression for esp scaled w/o displacement failed, code: %s\n", buf);
		exit(1);
	}

	if(regcomp(&regex_ebp_scaled,FIRN(".*\\[%bp" WS "[+]" WS ".*[-](.+)\\].*"),REG_EXTENDED | REG_ICASE) !=0)
	{
		fprintf(stderr,"Error: regular expression for ebp scaled addresses failed\n");
		exit(1);
	}

	if((errcode=regcomp(&regex_esp_direct,          FIRN(".*\\[%sp" WS "[+]" WS "(" HEXNUM ")\\].*"),REG_EXTENDED | REG_ICASE)) !=0)
	{
		char buf[1000];
		regerror(errcode,&regex_esp_direct,buf,sizeof(buf));
		fprintf(stderr,"Error: regular expression for esp direct addresses failed, code: %s\n",buf);
		exit(1);	
	}

	if((errcode=regcomp(&regex_esp_direct_negoffset,FIRN(".*\\[%sp" WS "[-]" WS "(" HEXNUM ")\\].*"),REG_EXTENDED | REG_ICASE)) !=0)
	{
		char buf[1000];
		regerror(errcode,&regex_esp_direct_negoffset,buf,sizeof(buf));
		fprintf(stderr,"Error: regular expression for esp direct addresses failed, code: %s\n",buf);
		exit(1);	
	}


	if(regcomp(&regex_ebp_direct,FIRN(".*\\[%bp" WS "[-]" WS "(" HEXNUM ")\\].*"),REG_EXTENDED | REG_ICASE) !=0)
	{
		fprintf(stderr,"Error: regular expression for esp direct addresses failed\n");
		exit(1);	
	}

	// stack allocation: match sub esp , K
	if (regcomp(&regex_stack_alloc, FIRN("[[:blank:]]*sub[[:blank:]]+%sp[[:blank:]]*,[[:blank:]]*(.+)[[:blank:]]*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for <sub esp, K> failed to compile\n");
		exit(1);
	}

	// stack deallocation: match add esp , K
	if (regcomp(&regex_stack_dealloc, FIRN("[[:blank:]]*add[[:blank:]]+%sp[[:blank:]]*,[[:blank:]]*(.+)[[:blank:]]*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for <add esp, K> failed to compile\n");
		exit(1);
	}

	// stack deallocation that does not use an offset
	if (regcomp(&regex_stack_dealloc_implicit, FIRN("([[:blank:]]*mov[[:blank:]]+%sp[[:blank:]]*,[[:blank:]]*%bp[[:blank:]]*)|([[:blank:]]*leave[[:blank:]]*)|([[:blank:]]*lea[[:blank:]]*%sp[[:blank:]]*,[[:blank:]]*\\[%bp[-].*\\][[:blank:]]*)"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for stack_dealloc_implicit failed to compile\n");
		exit(1);
	}

	if (regcomp(&regex_push_ebp, FIRN(".*push[[:blank:]]+(%bp).*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for push ebp failed to compile\n");
		exit(1);
	}

	if (regcomp(&regex_save_fp, FIRN(".*mov[[:blank:]]+(%bp)[[:blank:]]*,[[:blank:]]*(%sp).*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for save fp failed to compile\n");
		exit(1);
	}

	if (regcomp(&regex_push_anything, FIRN(".*push[[:blank:]]+(.*)"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for push (anything) failed to compile\n");
		exit(1);
	}

	//looking for scaled accesses using ebp as the index
	//eg. [ecx + ebp*1 - 0x21]
	//Unlike other expressions, there are two pattern matches here
	//the first is the scaling factor (if one exists), the second is the
	//offset.
	if (regcomp(&regex_scaled_ebp_index, FIRN(".*\\[.*[+]" WS "%bp[*]?(.*)[-](.+)\\].*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for scaled ebp index failed to compile\n");
		exit(1);
	}

	if (regcomp(&regex_call, FIRN("^call (.*)$"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for call failed to compile\n");
		exit(1);
	}


        if((errcode=regcomp(&regex_add_rbp,FIRN("add (" REGSTRING "), *%bp *"),REG_EXTENDED | REG_ICASE)) !=0)
        {
                char buf[1000];
                regerror(errcode,&regex_add_rbp,buf,sizeof(buf));
                fprintf(stderr,"Error: regular expression for regex_add_rbp failed, code: %s\n",buf);
                exit(1);
        }



}
