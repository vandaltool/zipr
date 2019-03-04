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
	regex_t regex_esp_scaled_nodisp;
	regex_t regex_ebp_scaled;
	regex_t regex_esp_direct;
	regex_t regex_esp_direct_negoffset;
	regex_t regex_ebp_direct;
	regex_t regex_stack_alloc;
	regex_t regex_stack_dealloc;
	regex_t regex_stack_dealloc_implicit;
	regex_t regex_lea_hack;
	regex_t regex_lea_rsp;
	regex_t regex_esp_only;
	regex_t regex_push_ebp;
	regex_t regex_push_anything;
	regex_t regex_and_esp;
	regex_t regex_scaled_ebp_index;
	regex_t regex_call;
	regex_t regex_add_rbp;

	static const int MAX_MATCHES = 10;
};

#endif
