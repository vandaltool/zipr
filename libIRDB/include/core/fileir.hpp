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

#include "type.hpp"

typedef std::set<Function_t*> FunctionSet_t;
typedef std::set<AddressID_t*> AddressSet_t;

// A variant of a problem, this
// may be an original variant
// (i.e., and unmodified Variant) or a modified variant.
class FileIR_t : public BaseObj_t
{
	public:

		// Create a Variant from the database
		FileIR_t(const VariantID_t &newprogid, File_t* fid=NULL);
		virtual ~FileIR_t();
	  
		// DB operations
		void WriteToDB();

		// accessors and mutators in one
		FunctionSet_t&    GetFunctions() { return funcs; }
		const FunctionSet_t&    GetFunctions() const { return funcs; }

		InstructionSet_t& GetInstructions() { return insns; }
		const InstructionSet_t& GetInstructions() const { return insns; }

		AddressSet_t&     GetAddresses() { return addrs; }
		const AddressSet_t&     GetAddresses() const { return addrs; }

		RelocationSet_t&  GetRelocations() { return relocs; }
		const RelocationSet_t&  GetRelocations() const { return relocs; }

		DataScoopSet_t&  GetDataScoops() { return scoops; }
		const DataScoopSet_t&  GetDataScoops() const { return scoops; }

		ICFSSet_t&        GetAllICFS() { return icfs_set; }
		const ICFSSet_t&        GetAllICFS() const { return icfs_set; }

		EhProgramSet_t&        GetAllEhPrograms() { return eh_pgms; }
		const EhProgramSet_t&        GetAllEhPrograms() const { return eh_pgms; }

		EhCallSiteSet_t&        GetAllEhCallSites() { return eh_css; }
		const EhCallSiteSet_t&        GetAllEhCallSites() const { return eh_css; }

		// generate the spri rules into the output file, fout.
		void GenerateSPRI(std::ostream &fout, bool with_ilr=false);

		// generate spri, assume that orig_varirp is the original variant. 
		void GenerateSPRI(FileIR_t *orig_varirp, std::ostream &fout, bool with_ilr=false);

		void SetBaseIDS();

		File_t* GetFile() const { return fileptr; }

		// Used for modifying a large number of instructions. AssembleRegistry
		// assembles the assembly isntructions for each registered instruction
		// and clears the registry. RegisterAssembly registers the instruction
		// to be assembled later. 
		void AssembleRegistry();
		void RegisterAssembly(Instruction_t *instr, std::string assembly);
		void UnregisterAssembly(Instruction_t *instr);
		std::string LookupAssembly(Instruction_t *instr);

		//Needed for inserting assembly before an instruction. 
		//if orig is not registered, the function returns, otherwise
		//the instruction/assembly mapping of orig->assembly is altered to
		//updated->assembly
		//removes the mapping for orig->assembly from the map. 
		void ChangeRegistryKey(Instruction_t* orig, Instruction_t* updated);

		static int GetArchitectureBitWidth();
		void SetArchitecture();

		// Lookup a scoop by address
		DataScoop_t* FindScoop(const libIRDB::virtual_offset_t &addr);

		void SplitScoop(DataScoop_t *tosplit, const libIRDB::virtual_offset_t &addr, size_t size, 
				DataScoop_t* &before,DataScoop_t* &containing, DataScoop_t* &after);


	private:

		static ArchitectureDescription_t *archdesc;

		#define ASM_REG_MAX_SIZE 500000

		typedef std::map<Instruction_t*,std::string> registry_type;

		// a pointer to the original variants IR, NULL means not yet loaded.
		FileIR_t* orig_variant_ir_p;

		registry_type assembly_registry;

		void ReadFromDB();	//accesses DB

		FunctionSet_t     funcs;
		InstructionSet_t  insns;
		AddressSet_t      addrs;
		RelocationSet_t   relocs;
		TypeSet_t         types;
		DataScoopSet_t    scoops;
		VariantID_t       progid;
		ICFSSet_t         icfs_set;
		File_t*           fileptr;
		EhProgramSet_t    eh_pgms;
		EhCallSiteSet_t   eh_css;

		std::map<db_id_t,AddressID_t*> ReadAddrsFromDB();
		std::map<db_id_t,EhProgram_t*> ReadEhPgmsFromDB();
		
		std::map<db_id_t,EhCallSite_t*> ReadEhCallSitesFromDB
		(
			std::map<EhCallSite_t*,db_id_t> &unresolvedEhCssLandingPads
		);

		std::map<db_id_t,Function_t*> ReadFuncsFromDB
		(
			std::map<db_id_t,AddressID_t*> &addrMap,
			std::map<db_id_t,Type_t*> &typeMap,
			std::map<Function_t*,db_id_t> &entry_points
		);

		std::map<db_id_t,DataScoop_t*> ReadScoopsFromDB
		(
			std::map<db_id_t,AddressID_t*> &addrMap,
			std::map<db_id_t,Type_t*> &typeMap
		);

		std::map<db_id_t,Instruction_t*> ReadInsnsFromDB 
		(	
			const std::map<db_id_t,Function_t*> &funcMap,
			const std::map<db_id_t,AddressID_t*> &addrMap,
			const std::map<db_id_t,EhProgram_t*> &ehpgmMap,
			const std::map<db_id_t,EhCallSite_t*> &ehcsMap,
			std::map<db_id_t,Instruction_t*> &addressToInstructionMap,
			std::map<Instruction_t*, db_id_t> &unresolvedICFS
		);

		void ReadRelocsFromDB
		(
			std::map<db_id_t,BaseObj_t*>		&insnMap
		);

		std::map<db_id_t, Type_t*> ReadTypesFromDB(TypeSet_t& types);
		void ReadAllICFSFromDB(std::map<db_id_t,Instruction_t*> &addr2insnMap,
			std::map<Instruction_t*, db_id_t> &unresolvedICFS);

		void CleanupICFS();
		void GarbageCollectICFS();
		void DedupICFS();
};

