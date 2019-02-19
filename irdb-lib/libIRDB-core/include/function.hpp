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

namespace libIRDB
{
// The basic Function of a variant.
class Function_t : public BaseObj_t, virtual public IRDB_SDK::Function_t
{
    public:
	
	virtual ~Function_t() {}
	Function_t() : BaseObj_t(NULL) {}	// create a new function not in the db 

	// create a function that's already in the DB  
	Function_t(db_id_t id, std::string name, int size, int oa_size, bool use_fp, bool is_safe, IRDB_SDK::FuncType_t *, IRDB_SDK::Instruction_t *entry);	

	InstructionSet_t& GetInstructions() { return my_insns; }
	const InstructionSet_t& getInstructions() const { return my_insns; }

        const int getStackFrameSize() const { return stack_frame_size; }
        const std::string& getName() const { return name; }
        uint32_t getOutArgsRegionSize() const { return out_args_region_size; }

        void setStackFrameSize(int size) { stack_frame_size=size; }
        void setName(std::string newname)	 { name=newname; }
        void setOutArgsRegionSize(uint32_t oa_size) {out_args_region_size=oa_size;}

	void setEntryPoint(IRDB_SDK::Instruction_t *insn) { entry_point=dynamic_cast<Instruction_t*>(insn); if(insn) assert(entry_point);  }
	IRDB_SDK::Instruction_t* getEntryPoint() const { return entry_point;}

	std::string WriteToDB(File_t *fid, db_id_t newid);

        bool getUseFramePointer() const { return use_fp; }
        void setUseFramePointer(bool useFP) { use_fp = useFP; }

        void setSafe(bool safe) { is_safe = safe; }
        bool isSafe() const { return is_safe; }

	void setType(IRDB_SDK::FuncType_t *t) ; // { function_type = dynamic_cast<FuncType_t*>(t); if(t) assert(function_type); }
	IRDB_SDK::FuncType_t* getType() const ; // { return function_type; }

	int getNumArguments() const;

    private:
	Instruction_t *entry_point;
	InstructionSet_t my_insns;
        int stack_frame_size;
        std::string name;
        uint32_t out_args_region_size;
        bool use_fp;
        bool is_safe;
	FuncType_t *function_type;
};

}
