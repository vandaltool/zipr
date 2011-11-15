
#include "PNRegularExpressions.hpp"
#include <cstdio>
#include <cstdlib>

using namespace std;

//TODO: for now the constructor exits the program in compilation of the regex fails
//Is throwing an exception a better option?
PNRegularExpressions::PNRegularExpressions()
{
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

    if(regcomp(&regex_ebp_scaled,".*\\[ebp[+].*+[-](.+)\\].*",REG_EXTENDED | REG_ICASE) !=0)
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

}
