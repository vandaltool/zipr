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

class Instruction_t;
typedef std::set<Instruction_t*> InstructionSet_t;

// Keep track of instruction control flow sets
class ICFS_t : public InstructionSet_t, public BaseObj_t
{
	public:
		ICFS_t(): BaseObj_t(NULL), is_complete(false) {}
		ICFS_t(db_id_t set_id, bool p_is_complete = false) : BaseObj_t(NULL)
		{
			SetBaseID(set_id);
			SetComplete(p_is_complete);	
		}
		std::string WriteToDB(File_t *fid);
		 
		void SetTargets(const InstructionSet_t &other) 
		{
			InstructionSet_t::operator=(other);
		}
		void SetComplete(bool complete) { is_complete = complete; }
		bool IsComplete() const { return is_complete; }			

	private:
		bool is_complete;
};

typedef std::set<ICFS_t*> ICFSSet_t;
