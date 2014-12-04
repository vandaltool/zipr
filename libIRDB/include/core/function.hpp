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

// The basic Function of a variant.
class Function_t : public BaseObj_t
{
    public:
	
	Function_t() : BaseObj_t(NULL) {}	// create a new function not in the db 

	// create a function that's already in the DB  
	Function_t(db_id_t id, std::string name, int size, int oa_size, bool use_fp, Instruction_t *entry);	

        std::set<Instruction_t*>& GetInstructions() { return my_insns; }

        int GetStackFrameSize() { return stack_frame_size; }
        std::string GetName()	{ return name; }
        int GetOutArgsRegionSize() {return out_args_region_size; }

        void SetStackFrameSize(int size) { stack_frame_size=size; }
        void SetName(std::string newname)	 { name=newname; }
        void SetOutArgsRegionSize(int oa_size) {out_args_region_size=oa_size;}

	void SetEntryPoint(Instruction_t *insn) {entry_point=insn;}
	Instruction_t* GetEntryPoint() { return entry_point;}

	void WriteToDB();		// we need the variant ID to write into a program.
	std::string WriteToDB(File_t *fid, db_id_t newid);

        bool GetUseFramePointer() { return use_fp; }
        void SetUseFramePointer(bool useFP) { use_fp = useFP; }


    private:
	Instruction_t *entry_point;
        std::set<Instruction_t*> my_insns;
        int stack_frame_size;
        std::string name;
        int out_args_region_size;
        bool use_fp;
};

