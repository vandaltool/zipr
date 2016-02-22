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

// An ELF file as represented by the DB
class Relocation_t : public BaseObj_t
{
    public:

        // create new item.
        Relocation_t() : BaseObj_t(NULL), offset(0), wrt_obj(NULL)  {}	// new reloc w/no data

	// a reloc read from the DB 
        Relocation_t(db_id_t reloc_id, int _offset, std::string _type, BaseObj_t* p_wrt_obj=NULL) :
		BaseObj_t(NULL), offset(_offset), type(_type), wrt_obj(p_wrt_obj) { SetBaseID(reloc_id); }

        Relocation_t(db_id_t reloc_id) : BaseObj_t(NULL), type(""), wrt_obj(NULL) { assert(0);}          // read from DB       

        void WriteToDB() { assert(0); }   // writes to DB ID is not -1.
        std::string WriteToDB(File_t* fid, BaseObj_t* insn);    // writes to DB, ID is not -1.

	void SetOffset(int off) { offset=off;}
	int GetOffset() { return offset; }
	void SetType(std::string ty) { type=ty;}
	std::string GetType() { return type; }

	/* get and set "with respect to" field */
	void SetWRT(libIRDB::BaseObj_t* WRT) { wrt_obj=WRT;}
	libIRDB::BaseObj_t* GetWRT() { return wrt_obj; }

        friend class FileIR_t;
        friend class Function_t;
        friend class AddressID_t;
        friend class Instruction_t;
        friend class VariantID_t;

    private:
	int offset;		// how far into the instruction the relocation should be applied.
	std::string type;	// a string that describes the relocation type.  
				// possible values:  32-bit, pcrel

	BaseObj_t* wrt_obj;
};
