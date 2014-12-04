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

#include <zipr_all.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>

using namespace zipr;

static void PrintStat(std::ostream &out, std::string description, int value)
{
	out << description << ": " << std::dec << value << std::endl;
}
static void PrintStat(std::ostream &out, std::string description, double value)
{
	out << description << ": " << std::dec << value << std::endl;
}

void Stats_t::PrintStats(Options_t opts, std::ostream &out)
{
	PrintStat(out, "Total dollops", total_dollops);
	PrintStat(out, "Total dollop size", total_dollop_space);
	PrintStat(out, "Total dollop instructions",total_dollop_instructions);
	PrintStat(out, "Truncated dollops",truncated_dollops);
	PrintStat(out, "Ave dollop size",
		(double)total_dollop_space/(double)total_dollops);
	PrintStat(out, "Ave dollop instructions",
		(double)total_dollop_instructions/(double)total_dollops);
	PrintStat(out, "Truncated dollop fraction",
		(double)truncated_dollops/(double)total_dollops);

	/*
	 * Optimizations
	 */
	if(opts.IsEnabledOptimization(Optimizations_t::OptimizationFallthroughPinned))
	{
		PrintStat(out, "Optimization: FallthroughPinned hit rate",
			(double)Hits[Optimizations_t::OptimizationFallthroughPinned]/
			(Hits[Optimizations_t::OptimizationFallthroughPinned] +
			Misses[Optimizations_t::OptimizationFallthroughPinned]));
	}

	PrintStat(out, "Total trampolines", total_trampolines);
	PrintStat(out, "Total 2-byte pin trampolines", total_2byte_pins);
	PrintStat(out, "Total 5-byte pin trampolines", total_5byte_pins);
	PrintStat(out, "Total trampoline space pins", total_tramp_space);
	PrintStat(out, "Other space", total_other_space);
	PrintStat(out, "Total free ranges", total_free_ranges);
}
