
#ifndef back_search_hpp
#define back_search_hpp
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

#include <iostream>
#include <limits>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <regex.h>
#include <ctype.h>
#include <stdio.h>
#include <cctype>

using namespace IRDB_SDK;
using namespace std;

/*
 * defines 
 */
#define ALLOF(a) begin(a),end(a)


// a way to map an instruction to its set of (direct) predecessors. 
map< Instruction_t* , InstructionSet_t > preds;

void calc_preds(FileIR_t* firp)
{
        preds.clear();
        for(auto insn : firp->getInstructions())
        {
                if(insn->getTarget())
                        preds[insn->getTarget()].insert(insn);
                if(insn->getFallthrough())
                        preds[insn->getFallthrough()].insert(insn);
        }
}


bool backup_until(const string &insn_type_regex_str, 
		  Instruction_t *& prev, 
		  Instruction_t* orig, 
		  const string & stop_if_set="", 
		  bool recursive=false, 
		  uint32_t max_insns=10000u, 
		  uint32_t max_recursions=5u)
{

	const auto find_or_build_regex=[&] (const string& s) -> regex_t&
		{
			// declare a freer for regexs so they go away when the program ends.
			const auto regex_freer=[](regex_t* to_free)  -> void
			{
				regfree(to_free);
				delete to_free;
			};
			// keep the map safe from anyone but me using it.
			using regex_unique_ptr_t=unique_ptr<regex_t, decltype(regex_freer)>;
			static map<string, regex_unique_ptr_t > regexs_used;

			if(s=="")
			{
				static regex_t empty;
				return empty;
			}
			const auto it=regexs_used.find(s);
			if(it==regexs_used.end())
			{
				// allocate a new regex ptr
				regexs_used.insert(pair<string,regex_unique_ptr_t>(s,move(regex_unique_ptr_t(new regex_t, regex_freer))));
				// and compile it.
				auto &regex_ptr=regexs_used.at(s);
				const auto ret=regcomp(regex_ptr.get(), s.c_str(), REG_EXTENDED);
				// error check
				assert(ret==0);
			}
			return *regexs_used.at(s).get();
		};


	// build regexs.
	const auto &preg            = find_or_build_regex(insn_type_regex_str);
	const auto &stop_expression = find_or_build_regex(stop_if_set);


	prev=orig;
	while(preds[prev].size()==1 && max_insns > 0)
	{
		// dec max for next loop 
		max_insns--;

		// get the only item in the list.
		prev=*(preds[prev].begin());
	

       		// get I7's disassembly
		const auto disasm=DecodedInstruction_t::factory(prev);

       		// check it's the requested type
       		if(regexec(&preg, disasm->getDisassembly().c_str(), 0, nullptr, 0) == 0)
			return true;

		if(stop_if_set!="")
		{
			for(const auto operand : disasm->getOperands())
			{
				if(operand->isWritten() && regexec(&stop_expression, operand->getString().c_str(), 0, nullptr, 0) == 0)
					return false;
			}
		}

		// otherwise, try backing up again.
	}
	if(recursive && max_insns > 0 && max_recursions > 0 )
	{
		const auto myprev=prev;
		// can't just use prev because recursive call will update it.
		const auto &mypreds=preds[myprev];
		for(const auto pred : mypreds)
		{
			prev=pred;// mark that we are here, in case we return true here.
			const auto disasm=DecodedInstruction_t::factory(pred);
       			// check it's the requested type
       			if(regexec(&preg, disasm->getDisassembly().c_str(), 0, nullptr, 0) == 0)
				return true;
			if(stop_if_set!="")
			{
				for(const auto operand : disasm->getOperands())
				{
					if(operand->isWritten() && regexec(&stop_expression, operand->getString().c_str(), 0, nullptr, 0) == 0)
						return false;
				}
			}
			if(backup_until(insn_type_regex_str, prev, pred, stop_if_set, recursive, max_insns, max_recursions/mypreds.size()))
				return true;

			// reset for next call
			prev=myprev;
		}
	}
	return false;
}


#endif
