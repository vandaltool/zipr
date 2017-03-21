/*
 * Copyright (c) 2017 - Zephyr Software LLC
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


typedef std::string EhProgramInstruction_t;
typedef vector<EhProgramInstruction_t> EhProgramListing_t;

class EhProgram_t : public BaseObj_t;
{
	public:


	EhProgramListing_t& GetCieProgram() { return cie_program; }
	EhProgramListing_t& GetFDEProgram() { return fde_program; }

        uint64_t GetCodeAlignmentFactor() const { return code_alignment_factor; }
        void SetCodeAlignmentFactor(const uint64_t caf) { code_alignment_factor=caf; }

        int64_t GetDataAlignmentFactor() const { return data_alignment_factor=daf; }
        void SetDataAlignmentFactor(const int64_t daf) { return data_alignment_factor; }

        std::string WriteToDB(File_t* fid, BaseObj_t* insn);    // writes to DB, ID is not -1.



	private:

	EhProgramListing_t cie_program;
	EhProgramListing_t fde_program;
        uint64_t code_alignment_factor;
        int64_t data_alignment_factor;
	uint8_t ptrsize; // needed for interpreting programs

};

typedef vector<const Relocation_t*> TypeTable_t; 

class EhCallSite_t : public BaseObj_t
{
	public:

	EhCallSite_t(const db_id_t id, const uint64_t enc=0, const Instruction_t* lp, const TypeTable_t& tt)  :
		BaseObj_t(NULL), tt_encoding(enc), landing_pad(lp), type_table(tt) { SetBaseID(id); }

	uint64_t GetTTEncoding() const { return tt_encoding; }
	void SetTTEncoding(const uint64_t p_tt) { tt_encoding=p_tt; }

	Instruction_t* GetLandingPad() const { return landing_pad; }
	void  SetLandingPad(const Instruction_t* lp) const { landing_pad=lp; }

	TypeTable_t& GetTypeTable() { return type_table; }

	private:

	uint64_t tt_encoding;
	Instruction_t* landing_pad;
	TypeTable_t type_table; // pointers to the entries in the type table.
};

