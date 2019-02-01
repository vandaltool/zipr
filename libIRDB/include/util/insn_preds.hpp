/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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

#ifndef insn_preds_h
#define insn_preds_h


class InstructionPredecessors_t : public IRDB_SDK::InstructionPredecessors_t
{
	private:
	typedef std::map<const IRDB_SDK::Instruction_t*, InstructionSet_t> PredMap_t;

	public:
	InstructionPredecessors_t(const IRDB_SDK::FileIR_t* f=NULL) {Init(); if(f) AddFile(f);}
        virtual ~InstructionPredecessors_t() {;}
	virtual void AddFile(const IRDB_SDK::FileIR_t* );

	virtual const InstructionSet_t& getPredecessors(const IRDB_SDK::Instruction_t* i)  const { return (*this)[i]; } 

	InstructionSet_t& operator[] (const IRDB_SDK::Instruction_t* i)  
		{
			return pred_map[i];
		}
	const InstructionSet_t& operator[] (const IRDB_SDK::Instruction_t* i)  const
		{ 
			PredMap_t::const_iterator it=pred_map.find(i);
			if (it!= pred_map.end()) 
				return it->second;
			static InstructionSet_t empty;
			return empty;
		}

	protected:
	virtual void Init() {};

	private:

	virtual void AddPred(const IRDB_SDK::Instruction_t* before, const IRDB_SDK::Instruction_t* after);
	virtual void AddPreds(const IRDB_SDK::Instruction_t* before, const IRDB_SDK::InstructionSet_t& after);

	PredMap_t pred_map;
};

#endif
