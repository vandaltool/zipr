
#include "PNRegularExpressions.hpp"
#include <cstdio>
#include <cstdlib>

using namespace std;

//TODO: for now the constructor exits the program in compilation of the regex fails
//Is throwing an exception a better option?
PNRegularExpressions::PNRegularExpressions()
{
  
    if (regcomp(&regex_and_esp, "[[:blank:]]*and[[:blank:]]+esp[[:blank:]]*,[[:blank:]]*(.+)[[:blank:]]*", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for and esp to compile\n");
	exit(1);
    }

    if (regcomp(&regex_ret, "^ret[[:blank:]]*$", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for ret failed to compile\n");
	exit(1);
    }

    /* match lea <anything> dword [<stuff>]*/
    if (regcomp(&regex_lea_hack, "(.*lea.*,.*)dword(.*)", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for lea hack failed to compile\n");
	exit(1);
    }
    
    if(regcomp(&regex_esp_only, ".*\\[(esp)\\].*",REG_EXTENDED | REG_ICASE) !=0)
    {
	fprintf(stderr,"Error: regular expression for esp scaled addresses failed\n");
	exit(1);
    }

    if(regcomp(&regex_esp_scaled, ".*\\[esp[+].*[+](.+)\\].*",REG_EXTENDED | REG_ICASE) !=0)
    {
	fprintf(stderr,"Error: regular expression for esp scaled addresses failed\n");
	exit(1);
    }

    if(regcomp(&regex_ebp_scaled,".*\\[ebp[+].*[-](.+)\\].*",REG_EXTENDED | REG_ICASE) !=0)
    {
	fprintf(stderr,"Error: regular expression for ebp scaled addresses failed\n");
	exit(1);
    }

    if(regcomp(&regex_esp_direct,".*\\[esp[+](.+)\\].*",REG_EXTENDED | REG_ICASE) !=0)
    {
	fprintf(stderr,"Error: regular expression for esp direct addresses failed\n");
	exit(1);    
    }

    if(regcomp(&regex_ebp_direct,".*\\[ebp[-](.+)\\].*",REG_EXTENDED | REG_ICASE) !=0)
    {
	fprintf(stderr,"Error: regular expression for esp direct addresses failed\n");
	exit(1);    
    }

    // stack allocation: match sub esp , K
    if (regcomp(&regex_stack_alloc, "[[:blank:]]*sub[[:blank:]]+esp[[:blank:]]*,[[:blank:]]*(.+)[[:blank:]]*", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for <sub esp, K> failed to compile\n");
	exit(1);
    }

    // stack deallocation: match add esp , K
    if (regcomp(&regex_stack_dealloc, "[[:blank:]]*add[[:blank:]]+esp[[:blank:]]*,[[:blank:]]*(.+)[[:blank:]]*", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for <add esp, K> failed to compile\n");
	exit(1);
    }

   // stack deallocation that does not use an offset
    if (regcomp(&regex_stack_dealloc_implicit, "([[:blank:]]*mov[[:blank:]]+esp[[:blank:]]*,[[:blank:]]*ebp[[:blank:]]*)|([[:blank:]]*leave[[:blank:]]*)|([[:blank:]]*lea[[:blank:]]*esp[[:blank:]]*,[[:blank:]]*\\[ebp[-].*\\][[:blank:]]*)", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for stack_dealloc_implicit failed to compile\n");
	exit(1);
    }

    if (regcomp(&regex_push_ebp, ".*push[[:blank:]]+(ebp).*", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for push ebp failed to compile\n");
	exit(1);
    }

    if (regcomp(&regex_push_anything, ".*push[[:blank:]]+(.*)", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for push (anything) failed to compile\n");
	exit(1);
    }

    //looking for scaled accesses using ebp as the index
    //eg. [ecx + ebp*1 - 0x21]
    //Unlike other expressions, there are two pattern matches here
    //the first is the scaling factor (if one exists), the second is the
    //offset.
    if (regcomp(&regex_scaled_ebp_index, ".*\\[.*[+]ebp[*]?(.*)[-](.+)\\].*", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for scaled ebp index failed to compile\n");
	exit(1);
    }

   if (regcomp(&regex_call, "^call (.*)$", REG_EXTENDED | REG_ICASE) != 0)
    {
	fprintf(stderr,"Error: regular expression for call failed to compile\n");
	exit(1);
    }


}
