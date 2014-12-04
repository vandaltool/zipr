/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#ifndef zipr_stats_t
#define zipr_stats_t

class Stats_t
{
	public:
		Stats_t() {
			for (int i=0; i<Optimizations_t::NumberOfOptimizations; i++)
			{
				Hits[i] = Misses[i] = 0;
			}
			total_dollops = 0;
			total_dollop_space = 0;
			total_dollop_instructions = 0;
			truncated_dollops = 0;
			total_trampolines = 0;
			total_2byte_pins = 0;
			total_5byte_pins = 0;
			total_tramp_space = 0;
			total_other_space = 0;
			total_free_ranges = 0;
		};

		void PrintStats(Options_t opts, std::ostream &out);

		/*
		 * General stats tracking.
		 */

		int total_dollops;
		int total_dollop_space;
		int total_dollop_instructions;
		int truncated_dollops;
		int total_trampolines;
		int total_2byte_pins;
		int total_5byte_pins;
		int total_tramp_space;
		int total_other_space;
		int total_free_ranges;

		/*
		 * Optimization stats tracking.
		 */
		int Hits[Optimizations_t::NumberOfOptimizations];
		int Misses[Optimizations_t::NumberOfOptimizations];
		void Hit(Optimizations_t::OptimizationName_t opt) { Hits[opt]++; };
		void Missed(Optimizations_t::OptimizationName_t opt) { Misses[opt]++; };
};
#endif
