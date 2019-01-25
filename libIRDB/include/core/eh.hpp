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

namespace libIRDB
{
using EhProgramInstruction_t = IRDB_SDK::EhProgramInstruction_t;
using EhProgramListing_t     = IRDB_SDK::EhProgramListing_t;
using EhProgramSet_t         = IRDB_SDK::EhProgramSet_t;
using TTOrderVector_t        = IRDB_SDK::TTOrderVector_t;
using EhCallSiteSet_t        = IRDB_SDK::EhCallSiteSet_t;

class EhProgram_t : public BaseObj_t, virtual public IRDB_SDK::EhProgram_t
{
	public:

	virtual ~EhProgram_t(){}
	EhProgram_t(const EhProgram_t& orig)
		: 
		BaseObj_t(NULL)
	{
		cie_program=orig.cie_program;
		fde_program=orig.fde_program;
		code_alignment_factor=orig.code_alignment_factor;
		data_alignment_factor=orig.data_alignment_factor;
		return_register=orig.return_register;
		ptrsize=orig.ptrsize;
		setBaseID(BaseObj_t::NOT_IN_DATABASE);
		GetRelocations()=orig.getRelocations();
		
	}
	EhProgram_t(db_id_t id, const uint64_t caf, const int64_t daf, const uint8_t rr, const uint8_t p_ptrsize,
			const EhProgramListing_t& ciep, const EhProgramListing_t& fdep)
		: 
		BaseObj_t(NULL), 
		code_alignment_factor(caf), 
		data_alignment_factor(daf), 
		return_register(rr), 
		ptrsize(p_ptrsize) ,
		cie_program(ciep),
		fde_program(fdep)
	{ 
		setBaseID(id); 
	}


	EhProgramListing_t& GetCIEProgram() { return cie_program; }
	const EhProgramListing_t& getCIEProgram() const { return cie_program; }

	EhProgramListing_t& GetFDEProgram() { return fde_program; }
	const EhProgramListing_t& getFDEProgram() const { return fde_program; }

        uint64_t getCodeAlignmentFactor() const { return code_alignment_factor; }
        void setCodeAlignmentFactor(const uint64_t caf) 
	{ 	
		if ( ((uint8_t)caf) != caf  ) throw std::logic_error(std::string()+"Invalid code alignment factor in call to "+__FUNCTION__);
		code_alignment_factor=(uint8_t)caf; 
	}

        int64_t getDataAlignmentFactor() const { return data_alignment_factor; }
        void setDataAlignmentFactor(const int64_t daf) 
	{ 
		if ( (( int8_t)daf) != daf  ) throw std::logic_error(std::string()+"Invalid datat alignment factor in call to "+__FUNCTION__);
		data_alignment_factor=(int8_t)daf; 
	}

        int64_t getReturnRegNumber() const { return return_register; }
        void setReturnRegNumber(const uint8_t rr) { return_register=rr; }

        std::vector<std::string> WriteToDB(File_t* fid);    // writes to DB, ID is not -1.

        uint8_t getPointerSize() const { return ptrsize; }


	friend bool operator<(const EhProgram_t&a, const EhProgram_t&b);

	void print() const;

	private:

        uint8_t code_alignment_factor;
        int8_t data_alignment_factor;
        int8_t return_register;
	uint8_t ptrsize; // needed for interpreting programs
	EhProgramListing_t cie_program;
	EhProgramListing_t fde_program;

};
bool operator<(const EhProgram_t&a, const EhProgram_t&b);


class EhCallSite_t : public BaseObj_t, virtual public IRDB_SDK::EhCallSite_t
{
	public:

	virtual ~EhCallSite_t(){}
	EhCallSite_t(const db_id_t id, const uint64_t enc=0, IRDB_SDK::Instruction_t* lp=NULL) : 
		BaseObj_t(NULL), 
		tt_encoding(enc), 
		landing_pad(lp)
	{ setBaseID(id); }

	uint64_t getTTEncoding() const { return tt_encoding; }
	void setTTEncoding(const uint64_t p_tt) { tt_encoding=p_tt; }

	IRDB_SDK::Instruction_t* getLandingPad() const { return landing_pad; }
	void setLandingPad(IRDB_SDK::Instruction_t* lp) { landing_pad=dynamic_cast<Instruction_t*>(lp); if(lp) assert(landing_pad);  }

	bool getHasCleanup() const ;
	void setHasCleanup(bool p_has_cleanup=true) ;

	TTOrderVector_t& GetTTOrderVector() { return ttov; }
	const TTOrderVector_t& getTTOrderVector() const { return ttov; }
	void setTTOrderVector(const TTOrderVector_t& p_ttov) { ttov=p_ttov; }

        std::string WriteToDB(File_t* fid);    // writes to DB, ID is not -1.


	private:

	uint64_t tt_encoding;
	IRDB_SDK::Instruction_t* landing_pad;
	TTOrderVector_t ttov;
};

}
