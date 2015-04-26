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

#ifndef __GLOBALS
#define __GLOBALS

extern bool verbose_log;

class PNOptions {
	public:
	// default configuration parameters go here
		PNOptions() {
			min_stack_padding = 64;
			max_stack_padding = 64;
			recursive_min_stack_padding = 32;
			recursive_max_stack_padding = 64;
			do_canaries = true;
		}

		void setMinStackPadding(int val) { min_stack_padding = val; }
		void setMaxStackPadding(int val) { max_stack_padding = val; }
		void setRecursiveMinStackPadding(int val) { recursive_min_stack_padding = val; }
		void setRecursiveMaxStackPadding(int val) { recursive_max_stack_padding = val; }

		int getMinStackPadding() const { return min_stack_padding; }
		int getMaxStackPadding() const { return max_stack_padding; }
		int getRecursiveMinStackPadding() const { return recursive_min_stack_padding; }
		int getRecursiveMaxStackPadding() const { return recursive_max_stack_padding; }

		void setDoCanaries(bool canaries) { do_canaries = canaries; }
		bool getDoCanaries() const { return do_canaries; }

	private:
		int min_stack_padding;
		int max_stack_padding;
		int recursive_min_stack_padding;
		int recursive_max_stack_padding;
		bool do_canaries;
};

extern PNOptions *pn_options;

#endif
