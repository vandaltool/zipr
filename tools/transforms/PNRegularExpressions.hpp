
#ifndef __PNREGULAREXPRESSIONS
#define __PNREGULAREXPRESSIONS
#include <regex.h>

class PNRegularExpressions
{
public:
	PNRegularExpressions();

	regex_t regex_save_fp;
	regex_t regex_ret;
	regex_t regex_esp_scaled;
	regex_t regex_ebp_scaled;
	regex_t regex_esp_direct;
	regex_t regex_ebp_direct;
	regex_t regex_stack_alloc;
	regex_t regex_stack_dealloc;
	regex_t regex_stack_dealloc_implicit;
	regex_t regex_lea_hack;
	regex_t regex_esp_only;
	regex_t regex_push_ebp;
	regex_t regex_push_anything;
	regex_t regex_and_esp;
	regex_t regex_scaled_ebp_index;
	regex_t regex_call;

	static const int MAX_MATCHES = 10;
};

#endif
