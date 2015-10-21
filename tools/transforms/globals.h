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

#include <set>
#include <string>
#include <stdlib.h>


extern bool verbose_log;

class PNOptions 
{
	public:
	// default configuration parameters go here
		PNOptions() {
			// specify defaults;
			min_stack_padding = 64;
			max_stack_padding = 64;
			recursive_min_stack_padding = 32;
			recursive_max_stack_padding = 64;
			do_canaries = true;
			do_selective_canaries = false;
			should_double_frame_size=true;
			random_seed=time(0);
			canary_value=0;
			canary_value_inited=false;
		}

		void setMinStackPadding(int val) { min_stack_padding = val; }
		void setMaxStackPadding(int val) { max_stack_padding = val; }
		void setRecursiveMinStackPadding(int val) { recursive_min_stack_padding = val; }
		void setRecursiveMaxStackPadding(int val) { recursive_max_stack_padding = val; }
		void setShouldDoubleFrameSize(bool val) { should_double_frame_size = val; }
		void setRandomSeed(bool val) { random_seed = val; }
		void setCanaryValue(int val) { canary_value = val; canary_value_inited=true; }

		int getMinStackPadding() const { return min_stack_padding; }
		int getMaxStackPadding() const { return max_stack_padding; }
		int getRecursiveMinStackPadding() const { return recursive_min_stack_padding; }
		int getRecursiveMaxStackPadding() const { return recursive_max_stack_padding; }
		bool getShouldDoubleFrameSize() const { return should_double_frame_size; }
		bool getRandomSeed() { return random_seed; }
		int getCanaryValue() 	
		{ 
			if (canary_value_inited) 
				return canary_value; 	
			else 
				return (rand()&0xffff) | (rand()<<16); 
		}

		void setDoCanaries(bool canaries) { do_canaries = canaries; }
		bool getDoCanaries() const { return do_canaries; }

		void addSelectiveCanaryFunction(std::string func) { do_selective_canaries = true; canary_functions.insert(func);}
		bool shouldCanaryFunction(std::string func) 
		{ 	
			if(do_selective_canaries)
			{
				bool notfound = (canary_functions.find(func)==canary_functions.end());
				bool found=!notfound;
				return found;
			}
			else
				return getDoCanaries();
		}

	private:
		int min_stack_padding;
		int max_stack_padding;
		int recursive_min_stack_padding;
		int recursive_max_stack_padding;
		bool do_canaries;
		bool do_selective_canaries;
		bool should_double_frame_size;
		bool random_seed;
		int canary_value;
		bool canary_value_inited;

		std::set<std::string> canary_functions;
};

extern PNOptions *pn_options;

#endif
