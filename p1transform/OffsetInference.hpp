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


#ifndef __OFFSETSTACKLAYOUTINFERENCE
#define __OFFSETSTACKLAYOUTINFERENCE
#include "PNStackLayoutInference.hpp"
#include "PNRegularExpressions.hpp"
#include <map>
#include <string>

class OffsetInference : public PNStackLayoutInference
{

protected:
	std::map<IRDB_SDK::Instruction_t*, bool> in_prologue;
	std::map<IRDB_SDK::Function_t*,PNStackLayout*> direct;
	std::map<IRDB_SDK::Function_t*,PNStackLayout*> scaled;
	std::map<IRDB_SDK::Function_t*,PNStackLayout*> all_offsets;
	std::map<IRDB_SDK::Function_t*,PNStackLayout*> p1;

	PNRegularExpressions *pn_regex;

	virtual void FindAllOffsets(IRDB_SDK::Function_t *func);
	virtual PNStackLayout* GetLayout(std::map<IRDB_SDK::Function_t*,PNStackLayout*> &mymap, IRDB_SDK::Function_t *func);
	//	virtual void getInstructions(std::vector<IRDB_SDK::Instruction_t*> &instructions,IRDB_SDK::libIRDB::BasicBlock_t *block,std::set<IRDB_SDK::libIRDB::BasicBlock_t*> &block_set);
	virtual StackLayout* SetupLayout(IRDB_SDK::Function_t *func);
public:
	OffsetInference() : pn_regex(NULL) {}
	virtual ~OffsetInference();
	virtual PNStackLayout* GetPNStackLayout(IRDB_SDK::Function_t *func);
	virtual PNStackLayout* GetDirectAccessLayout(IRDB_SDK::Function_t *func);
	virtual PNStackLayout* GetScaledAccessLayout(IRDB_SDK::Function_t *func);
	virtual PNStackLayout* GetP1AccessLayout(IRDB_SDK::Function_t *func);
	virtual std::string GetInferenceName() const;
};

#endif
