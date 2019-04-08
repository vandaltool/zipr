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
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>



extern bool verbose_log;

#define DEFAULT_DETECTION_EXIT_CODE 189

// make sure these match values in detector_handlers.h in the strata library
enum mitigation_policy 
{
	P_NONE=0, 
	P_CONTINUE_EXECUTION, 
	P_CONTROLLED_EXIT, 
	P_CONTINUE_EXECUTION_SATURATING_ARITHMETIC, 
	P_CONTINUE_EXECUTION_WARNONLY,
	P_HARD_EXIT
};



class PNOptions 
{
	public:
	// default configuration parameters go here
		PNOptions() {
			// specify defaults;
			min_stack_padding = 128;
			max_stack_padding = min_stack_padding*2;
			recursive_min_stack_padding = 64;
			recursive_max_stack_padding = recursive_min_stack_padding*2;
			do_canaries = true;
			do_breadcrumbs = false;
			do_selective_canaries = false;
			should_double_frame_size=true;
			random_seed=getpid();
			canary_value=0;
			canary_value_inited=false;
			double_threshold=32*1024; // 32kb 
			spri_validate=false;
			detection_policy=P_HARD_EXIT;
			detection_exit_code=DEFAULT_DETECTION_EXIT_CODE;
		}

		void setMinStackPadding(int val) { min_stack_padding = val; }
		void setMaxStackPadding(int val) { max_stack_padding = val; }
		void setRecursiveMinStackPadding(int val) { recursive_min_stack_padding = val; }
		void setRecursiveMaxStackPadding(int val) { recursive_max_stack_padding = val; }
		void setShouldDoubleFrameSize(bool val) { should_double_frame_size = val; }
		void setRandomSeed(int val) { random_seed = val; }
		void setCanaryValue(int val) { canary_value = val; canary_value_inited=true; }
		void setDoubleThreshold(int val) { double_threshold = val; }

		int getMinStackPadding() const { return min_stack_padding; }
		int getMaxStackPadding() const { return max_stack_padding; }
		int getRecursiveMinStackPadding() const { return recursive_min_stack_padding; }
		int getRecursiveMaxStackPadding() const { return recursive_max_stack_padding; }
		bool getShouldDoubleFrameSize() const { return should_double_frame_size; }
		bool getShouldSpriValidate() const { return spri_validate; }
		int getDoubleThreshold() { return double_threshold; }
		int getRandomSeed() { return random_seed; }
		int getCanaryValue() 	
		{ 
			if (canary_value_inited) 
				return canary_value; 	
			else 
				return (rand()&0xffff) | (rand()<<16); 
		}

		void setDoCanaries   (bool canaries   ) { do_canaries = canaries; }
		void setDoBreadcrumbs(bool breadcrumbs) { do_breadcrumbs = breadcrumbs; }

		bool getDoCanaries()    const { return do_canaries;    }
		bool getDoBreadcrumbs() const { return do_breadcrumbs; }

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

		void setDetectionPolicy(mitigation_policy p_policy) { detection_policy = p_policy; } 
		mitigation_policy getDetectionPolicy() const { return detection_policy; } 
		unsigned getDetectionExitCode() const { return detection_exit_code; }
		void setDetectionExitCode(unsigned p_exitCode) { detection_exit_code = p_exitCode; }

	private:
		int min_stack_padding;
		int max_stack_padding;
		int recursive_min_stack_padding;
		int recursive_max_stack_padding;
		bool do_canaries;
		bool do_breadcrumbs;
		bool do_selective_canaries;
		bool should_double_frame_size;
		int random_seed;
		int canary_value;
		bool canary_value_inited;

		int double_threshold;
		bool spri_validate;

		std::set<std::string> canary_functions;

		mitigation_policy detection_policy;
		unsigned detection_exit_code;
};

extern PNOptions *pn_options;

#endif
