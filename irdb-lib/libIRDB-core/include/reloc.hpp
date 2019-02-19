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

// An ELF file as represented by the DB
class Relocation_t : public BaseObj_t, virtual public IRDB_SDK::Relocation_t
{
    public:

        // create new item.
        virtual ~Relocation_t() {}
        Relocation_t() : BaseObj_t(NULL), offset(0), wrt_obj(NULL), addend(0)  {}	// new reloc w/no data

	// a reloc read from the DB 
        Relocation_t(db_id_t reloc_id, int _offset, std::string _type, IRDB_SDK::BaseObj_t* p_wrt_obj=NULL, int32_t p_addend=0) :
		BaseObj_t(NULL), offset(_offset), type(_type), wrt_obj(p_wrt_obj), addend(p_addend) { setBaseID(reloc_id); }


        void WriteToDB() { assert(0); }   // writes to DB ID is not -1.
        std::vector<std::string> WriteToDB(File_t* fid, BaseObj_t* insn);    // writes to DB, ID is not -1.

	void setOffset(int off) { offset=off;}
	int getOffset() const { return offset; }
	void setType(std::string ty) { type=ty;}
	std::string getType() const { return type; }

	void setAddend(uint32_t p_addend) { addend=p_addend;}
	uint32_t getAddend() const { return addend; }

	/* get and set "with respect to" field */
	void setWRT(IRDB_SDK::BaseObj_t* WRT) { wrt_obj=dynamic_cast<BaseObj_t*>(WRT); if(WRT) assert(wrt_obj); }
	IRDB_SDK::BaseObj_t* getWRT() const { return wrt_obj; }

        friend class FileIR_t;
        friend class Function_t;
        friend class AddressID_t;
        friend class Instruction_t;
        friend class VariantID_t;

    private:
	int offset;		// how far into the instruction the relocation should be applied.
	std::string type;	// a string that describes the relocation type.  
				// possible values:  32-bit, pcrel

	IRDB_SDK::BaseObj_t* wrt_obj;
	int32_t addend;
};
}
