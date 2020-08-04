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
#include <keystone/keystone.h>

namespace libIRDB
{

using namespace std;
using FunctionSet_t = IRDB_SDK::FunctionSet_t;
using AddressSet_t  = IRDB_SDK::AddressSet_t ;

// A variant of a problem, this
// may be an original variant
// (i.e., and unmodified Variant) or a modified variant.
class FileIR_t : public BaseObj_t, virtual public IRDB_SDK::FileIR_t
{
	public:

		// Create a Variant from the database
		FileIR_t(const VariantID_t &newprogid, File_t* fid=NULL);
		virtual ~FileIR_t();
	  
		// DB operations
		void writeToDB(std::ostream *verbose_logging=&std::cerr);

		// accessors and mutators in one
		FunctionSet_t&          GetFunctions()       { return funcs; }
		const FunctionSet_t&    getFunctions() const { return funcs; }

		InstructionSet_t&       GetInstructions()       { return insns; }
		const InstructionSet_t& getInstructions() const { return insns; }

		AddressSet_t&           GetAddresses()       { return addrs; }
		const AddressSet_t&     getAddresses() const { return addrs; }

		RelocationSet_t&        GetRelocations()       { return relocs; }
		const RelocationSet_t&  getRelocations() const { return relocs; }

		DataScoopSet_t&         GetDataScoops()       { return scoops; }
		const DataScoopSet_t&   getDataScoops() const { return scoops; }

		ICFSSet_t&              GetAllICFS()       { return icfs_set; }
		const ICFSSet_t&        getAllICFS() const { return icfs_set; }

		EhProgramSet_t&         GetAllEhPrograms()       { return eh_pgms; }
		const EhProgramSet_t&   getAllEhPrograms() const { return eh_pgms; }

		EhCallSiteSet_t&        GetAllEhCallSites()       { return eh_css; }
		const EhCallSiteSet_t&  getAllEhCallSites() const { return eh_css; }

		// generate the spri rules into the output file, fout.
		void GenerateSPRI(std::ostream &fout, bool with_ilr=false);

		// generate spri, assume that orig_varirp is the original variant. 
		void GenerateSPRI(FileIR_t *orig_varirp, std::ostream &fout, bool with_ilr=false);

		void setBaseIDS();
		IRDB_SDK::DatabaseID_t getMaxBaseID() const;

		File_t* getFile() const { return fileptr; }

		// Used for modifying a large number of instructions. AssembleRegistry
		// assembles the assembly isntructions for each registered instruction
		// and clears the registry. RegisterAssembly registers the instruction
		// to be assembled later. 
		static void assemblestr(ks_engine * &ks, IRDB_SDK::Instruction_t *ins, const char * instruct, char * &encode, size_t &size, size_t &count);
		void assembleRegistry();
		string assemblevect(vector<string> assembly_vec);
		void registerAssembly(IRDB_SDK::Instruction_t *instr, string assembly);
		void unregisterAssembly(IRDB_SDK::Instruction_t *instr);
		string lookupAssembly(IRDB_SDK::Instruction_t *instr);

		//Needed for inserting assembly before an instruction. 
		//if orig is not registered, the function returns, otherwise
		//the instruction/assembly mapping of orig->assembly is altered to
		//updated->assembly
		//removes the mapping for orig->assembly from the map. 
		void changeRegistryKey(IRDB_SDK::Instruction_t* orig,  IRDB_SDK::Instruction_t* updated);

		void setArchitecture();

		static void setArchitecture(const int width, const  IRDB_SDK::ADMachineType_t mt);
		// static const  IRDB_SDK::ArchitectureDescription_t* getArchitecture() { return archdesc; }

		// Lookup a scoop by address
		IRDB_SDK::DataScoop_t* findScoop(const IRDB_SDK::VirtualOffset_t &addr) const;

		void splitScoop(IRDB_SDK::DataScoop_t *tosplit, const IRDB_SDK::VirtualOffset_t &addr, size_t size, 
				IRDB_SDK::DataScoop_t* &before, IRDB_SDK::DataScoop_t* &containing,  IRDB_SDK::DataScoop_t* &after, IRDB_SDK::DatabaseID_t *max_id=NULL);

		virtual IRDB_SDK::EhCallSite_t* addEhCallSite(IRDB_SDK::Instruction_t* for_insn, const uint64_t enc=0, IRDB_SDK::Instruction_t* lp=nullptr) ;

		virtual IRDB_SDK::Relocation_t* addNewRelocation(
                        IRDB_SDK::BaseObj_t* from_obj,
                        int32_t _offset,
                        string _type,
                        IRDB_SDK::BaseObj_t* p_wrt_obj=nullptr,
                        int32_t p_addend=0) ;
		virtual EhProgram_t* addEhProgram(
		        IRDB_SDK::Instruction_t* insn=nullptr,
			const uint64_t caf=1,
			const int64_t daf=1,
			const uint8_t rr=1,
			const uint8_t p_ptrsize=8,
			const EhProgramListing_t& p_cie_program={},
			const EhProgramListing_t& p_fde_program={}
			) ;
		virtual IRDB_SDK::AddressID_t* addNewAddress(const IRDB_SDK::DatabaseID_t& myfileID, const IRDB_SDK::VirtualOffset_t& voff=0) ;
		virtual IRDB_SDK::ICFS_t*      addNewICFS   (
			IRDB_SDK::Instruction_t* insn=nullptr, 
			const IRDB_SDK::InstructionSet_t& targets={}, 
			const IRDB_SDK::ICFSAnalysisStatus_t& status=IRDB_SDK::iasAnalysisIncomplete);
		virtual IRDB_SDK::Instruction_t* addNewInstruction(
			IRDB_SDK::AddressID_t* addr=nullptr, 
			IRDB_SDK::Function_t* func=nullptr, 
			const string& bits="", 
			const string& comment="user-added", 
			IRDB_SDK::AddressID_t* indTarg=nullptr);

			virtual IRDB_SDK::DataScoop_t* addNewDataScoop(
				const string& p_name="user-added",
				IRDB_SDK::AddressID_t* p_start=nullptr,
				IRDB_SDK::AddressID_t* p_end=nullptr,
				IRDB_SDK::Type_t* p_type=nullptr,
				uint8_t p_permissions=0,
				bool p_is_relro=false,
				const string &p_contents="",
				IRDB_SDK::DatabaseID_t id=BaseObj_t::NOT_IN_DATABASE);

		virtual void removeScoop(IRDB_SDK::DataScoop_t* s) ;
		virtual void moveRelocation(IRDB_SDK::Relocation_t* reloc, IRDB_SDK::Instruction_t* from, IRDB_SDK::Instruction_t* to) ;
		virtual IRDB_SDK::EhProgram_t* copyEhProgram(const IRDB_SDK::EhProgram_t& orig);

		virtual void   setAllEhPrograms(const EhProgramSet_t& new_pgms)  { eh_pgms=new_pgms; }







	private:

		static ArchitectureDescription_t *archdesc;

		#define ASM_REG_MAX_SIZE 500000

		typedef std::map<Instruction_t*,string> registry_type;

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
	        VariantID_t&      progid;    // Not owned by fileIR
		ICFSSet_t         icfs_set;
		File_t*           fileptr;   // Owned by variant, not fileIR
		EhProgramSet_t    eh_pgms;
		EhCallSiteSet_t   eh_css;

		map<db_id_t,AddressID_t*> ReadAddrsFromDB();
		map<db_id_t,EhProgram_t*> ReadEhPgmsFromDB();
		
		map<db_id_t,EhCallSite_t*> ReadEhCallSitesFromDB
		(
			map<EhCallSite_t*,db_id_t> &unresolvedEhCssLandingPads
		);

		map<db_id_t,Function_t*> ReadFuncsFromDB
		(
			map<db_id_t,AddressID_t*> &addrMap,
			map<db_id_t,Type_t*> &typeMap,
			map<Function_t*,db_id_t> &entry_points
		);

		map<db_id_t,DataScoop_t*> ReadScoopsFromDB
		(
			map<db_id_t,AddressID_t*> &addrMap,
			map<db_id_t,Type_t*> &typeMap
		);

		map<db_id_t,Instruction_t*> ReadInsnsFromDB 
		(	
			const map<db_id_t,Function_t*> &funcMap,
			const map<db_id_t,AddressID_t*> &addrMap,
			const map<db_id_t,EhProgram_t*> &ehpgmMap,
			const map<db_id_t,EhCallSite_t*> &ehcsMap,
			map<db_id_t,Instruction_t*> &addressToInstructionMap,
			map<Instruction_t*, db_id_t> &unresolvedICFS
		);

		void ReadRelocsFromDB
		(
			map<db_id_t,BaseObj_t*>		&insnMap
		);

		map<db_id_t, Type_t*> ReadTypesFromDB(TypeSet_t& types);
		void ReadAllICFSFromDB(map<db_id_t,Instruction_t*> &addr2insnMap,
			map<Instruction_t*, db_id_t> &unresolvedICFS);

		void CleanupICFS(ostream *verbose_logging=&cerr);
		void GarbageCollectICFS(ostream *verbose_logging=&cerr);
		void DedupICFS(ostream *verbose_logging=&cerr);


		clock_t ReadIRDB_start;
		clock_t ReadIRDB_end;

		friend IRDB_SDK::FileIR_t;

};

}
