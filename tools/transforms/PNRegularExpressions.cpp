
#include "PNRegularExpressions.hpp"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <assert.h>
#include "libIRDB-core.hpp"

using namespace std;

#define FIRN(s) fill_in_reg_name((s))
#define HEXNUM "[[:blank:]]*0x[0123456789abcdefABCDEFxX]+[[:blank:]]*"
#define REGSTRING "[[:blank:]]*[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]+[[:blank:]]*"
#define OPTSCALE "[[:blank:]]*[*][[:blank:]]*[1248][[:blank:]]*|[[:blank:]]*"

static char* fill_in_reg_name(const char *instring)
{

	int width=libIRDB::FileIR_t::GetArchitectureBitWidth();
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
  
	if (regcomp(&regex_and_esp, FIRN("[[:blank:]]*and[[:blank:]]+%sp[[:blank:]]*,[[:blank:]]*(.+)[[:blank:]]*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for and esp to compile\n");
		exit(1);
	}

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
	
	if(regcomp(&regex_esp_only, FIRN(".*\\[(%sp)\\].*"),REG_EXTENDED | REG_ICASE) !=0)
	{
		fprintf(stderr,"Error: regular expression for esp scaled addresses failed\n");
		exit(1);
	}

	if(regcomp(&regex_esp_scaled, FIRN(".*\\[%sp[+].*[+](.+)\\].*"),REG_EXTENDED | REG_ICASE) !=0)
	{
		fprintf(stderr,"Error: regular expression for esp scaled addresses failed\n");
		exit(1);
	}
	if((errcode=regcomp(&regex_esp_scaled_nodisp, FIRN(".*\\[%sp[+]"REGSTRING OPTSCALE"(\\]).*"),REG_EXTENDED | REG_ICASE)) !=0)
	{
		char buf[1000];
		regerror(errcode,&regex_esp_scaled_nodisp,buf,sizeof(buf));
		fprintf(stderr,"Error: regular expression for esp scaled w/o displacement failed, code: %s\n", buf);
		exit(1);
	}

	if(regcomp(&regex_ebp_scaled,FIRN(".*\\[%bp[+].*[-](.+)\\].*"),REG_EXTENDED | REG_ICASE) !=0)
	{
		fprintf(stderr,"Error: regular expression for ebp scaled addresses failed\n");
		exit(1);
	}

	if((errcode=regcomp(&regex_esp_direct,FIRN(".*\\[%sp[+](.+)"HEXNUM"*\\].*"),REG_EXTENDED | REG_ICASE)) !=0)
	{
		char buf[1000];
		regerror(errcode,&regex_esp_direct,buf,sizeof(buf));
		fprintf(stderr,"Error: regular expression for esp direct addresses failed, code: %s\n",buf);
		exit(1);	
	}


	if(regcomp(&regex_ebp_direct,FIRN(".*\\[%bp[-]"HEXNUM"(.+)\\].*"),REG_EXTENDED | REG_ICASE) !=0)
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
	if (regcomp(&regex_scaled_ebp_index, FIRN(".*\\[.*[+]%bp[*]?(.*)[-](.+)\\].*"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for scaled ebp index failed to compile\n");
		exit(1);
	}

	if (regcomp(&regex_call, FIRN("^call (.*)$"), REG_EXTENDED | REG_ICASE) != 0)
	{
		fprintf(stderr,"Error: regular expression for call failed to compile\n");
		exit(1);
	}


}
