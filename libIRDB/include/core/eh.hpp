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
typedef std::vector<EhProgramInstruction_t> EhProgramListing_t;

class EhProgram_t : public BaseObj_t
{
	public:

	EhProgram_t(db_id_t id, const uint64_t caf, const int64_t daf, const uint8_t p_ptrsize)
		: BaseObj_t(NULL), code_alignment_factor(caf), data_alignment_factor(daf), ptrsize(p_ptrsize) 
	{ SetBaseID(id); }


	EhProgramListing_t& GetCIEProgram() { return cie_program; }
	const EhProgramListing_t& GetCIEProgram() const { return cie_program; }

	EhProgramListing_t& GetFDEProgram() { return fde_program; }
	const EhProgramListing_t& GetFDEProgram() const { return fde_program; }

        uint64_t GetCodeAlignmentFactor() const { return code_alignment_factor; }
        void SetCodeAlignmentFactor(const uint64_t caf) { code_alignment_factor=caf; }

        int64_t GetDataAlignmentFactor() const { return data_alignment_factor; }
        void SetDataAlignmentFactor(const int64_t daf) { data_alignment_factor=daf; }

        std::string WriteToDB(File_t* fid);    // writes to DB, ID is not -1.


	friend bool operator<(const EhProgram_t&a, const EhProgram_t&b);

	void print() const;

	private:


	EhProgramListing_t cie_program;
	EhProgramListing_t fde_program;
        uint64_t code_alignment_factor;
        int64_t data_alignment_factor;
	uint8_t ptrsize; // needed for interpreting programs

};
bool operator<(const EhProgram_t&a, const EhProgram_t&b);

/*
struct EhProgramComparator_t {
	bool operator() (const EhProgram_t* a, const EhProgram_t* b) { return *a < *b; }
};
*/

typedef std::set<EhProgram_t*> EhProgramSet_t;

class EhCallSite_t : public BaseObj_t
{
	public:

	EhCallSite_t(const db_id_t id, const uint64_t enc=0, Instruction_t* lp=NULL)  :
		BaseObj_t(NULL), tt_encoding(enc), landing_pad(lp)
	{ SetBaseID(id); }

	uint64_t GetTTEncoding() const { return tt_encoding; }
	void SetTTEncoding(const uint64_t p_tt) { tt_encoding=p_tt; }

	Instruction_t* GetLandingPad() const { return landing_pad; }
	void SetLandingPad(Instruction_t* lp) { landing_pad=lp; }

        std::string WriteToDB(File_t* fid);    // writes to DB, ID is not -1.

	private:

	uint64_t tt_encoding;
	Instruction_t* landing_pad;
};

typedef std::set<EhCallSite_t*> EhCallSiteSet_t;
