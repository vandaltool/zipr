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

using RelocationSet_t = IRDB_SDK::RelocationSet_t; 

// The basic instruction of a variant.
class Instruction_t : public BaseObj_t, virtual public IRDB_SDK::Instruction_t
{
	public:
		virtual ~Instruction_t(){}
		Instruction_t();
		Instruction_t(db_id_t id, AddressID_t *addr, Function_t *func, db_id_t orig_id, 
		std::string data, std::string callback, std::string comment, AddressID_t *my_indTarg, db_id_t doip_id);

		IRDB_SDK::AddressID_t* getAddress() const  { return my_address; } 
		IRDB_SDK::Function_t* getFunction() const ; // { return my_function; } 
		db_id_t getOriginalAddressID() const { return orig_address_id; } 
		IRDB_SDK::Instruction_t* getFallthrough() const { return fallthrough; } 
		IRDB_SDK::Instruction_t* getTarget() const { return target; } 
		IRDB_SDK::ICFS_t* getIBTargets() const  { return icfs; }

		std::string getDataBits()  const { return data; } 
		std::string getCallback()  const { return callback; } 
		std::string getComment()   const { return comment; } 
		IRDB_SDK::EhProgram_t* getEhProgram() const ; // { return eh_pgm; }
		IRDB_SDK::EhCallSite_t* getEhCallSite() const ; // { return eh_cs; }

  
		void setAddress(IRDB_SDK::AddressID_t* newaddr)  ; // { my_address=newaddr; }
		void setFunction(IRDB_SDK::Function_t* func   )  ; // { my_function=func;}
		void setOriginalAddressID(db_id_t origid) { orig_address_id=origid; /* you shouldn't do this, unless you know what you're doing! */}
		void setFallthrough(IRDB_SDK::Instruction_t* i) ; // { fallthrough=i; }
		void setTarget(IRDB_SDK::Instruction_t* i)	  ; // { target=i; }
		void setIBTargets(IRDB_SDK::ICFS_t *p_icfs)	 ; // { icfs=p_icfs; }
		void setDataBits(std::string orig)	{ data=orig; }
		void setCallback(std::string orig)	{ callback=orig; }
		void setComment(std::string orig)	 { comment=orig; }
		void setEhProgram(IRDB_SDK::EhProgram_t* orig)	 ; // { eh_pgm=orig; }
		void setEhCallSite(IRDB_SDK::EhCallSite_t* orig); // 	 { eh_cs=orig; }

		IRDB_SDK::AddressID_t* getIndirectBranchTargetAddress() const; // { return indTarg; }
		void setIndirectBranchTargetAddress(IRDB_SDK::AddressID_t* myIndTarg) ; // { indTarg=myIndTarg; }

		void WriteToDB() { assert(0); }
		std::vector<std::string> WriteToDB(File_t *fid, db_id_t newid); 
		// int Disassemble(DISASM &d) const; 
		std::string getDisassembly() const;
		bool assemble(std::string assembly);

		bool isFunctionExit() const;

		//static bool SetsStackPointer(DISASM *disasm);
		//static bool SetsStackPointer(ARGTYPE* arg);

		bool IsSyscall() { return getDisassembly().find("int 0x80") != std::string::npos; }

	private:
		AddressID_t*	my_address;
		Function_t*	 my_function;
		db_id_t		 orig_address_id;		// const, should not change.
		Instruction_t*  fallthrough;
		Instruction_t*  target;
		std::string	 data;
		std::string	 callback;	// name of callback handler (if any)
		std::string	 comment;
		AddressID_t*	indTarg;
		ICFS_t*		 icfs;
		EhProgram_t*	 eh_pgm;
		EhCallSite_t*	 eh_cs;
};
}
