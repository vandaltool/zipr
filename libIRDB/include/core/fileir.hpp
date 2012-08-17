

// A variant of a problem, this
// may be an original variant
// (i.e., and unmodified Variant) or a modified variant.
class FileIR_t : public BaseObj_t
{
    public:

        // Create a Variant from the database
        FileIR_t(const VariantID_t &newprogid, File_t* fid=NULL);
  
        // DB operations
        void WriteToDB();

        // accessors and mutators in one
        std::set<Function_t*>& GetFunctions() { return funcs; }
        std::set<Instruction_t*>& GetInstructions() { return insns; }
        std::set<AddressID_t*>&    GetAddresses() { return addrs; }
        std::set<Relocation_t*>&    GetRelocations() { return relocs; }

	// generate the spri rules into the output file, fout.
	void GenerateSPRI(std::ostream &fout);

	// generate spri, assume that orig_varirp is the original variant. 
	void GenerateSPRI(FileIR_t *orig_varirp, std::ostream &fout);

	void SetBaseIDS();

	File_t* GetFile() { return fileptr; }

	// Used for modifying a large number of instructions. AssembleRegistry
	// assembles the assembly isntructions for each registered instruction
	// and clears the registry. RegisterAssembly registers the instruction
	// to be assembled later. 
	void AssembleRegistry();
	void RegisterAssembly(Instruction_t *instr, std::string assembly);
	//Needed for inserting assembly before an instruction. 
	//if orig is not registered, the function returns, otherwise
	//the instruction/assembly mapping of orig->assembly is altered to
	//updated->assembly
	//removes the mapping for orig->assembly from the map. 
	void ChangeRegistryKey(Instruction_t* orig, Instruction_t* updated);

    private:
	#define ASM_REG_MAX_SIZE 500000

	typedef std::map<Instruction_t*,std::string> registry_type;

	// a pointer to the original variants IR, NULL means not yet loaded.
	FileIR_t* orig_variant_ir_p;

	registry_type assembly_registry;

        void ReadFromDB();  //accesses DB

        std::set<Function_t*> funcs;
        std::set<Instruction_t*> insns;
        std::set<AddressID_t*> addrs;
        std::set<Relocation_t*> relocs;
        VariantID_t progid;
	File_t* fileptr;

	std::map<db_id_t,AddressID_t*> ReadAddrsFromDB();
	std::map<db_id_t,Function_t*> ReadFuncsFromDB
		(
			std::map<db_id_t,AddressID_t*> &addrMap
		);
	std::map<db_id_t,Instruction_t*> ReadInsnsFromDB 
		(	
			std::map<db_id_t,Function_t*> &funcMap,
			std::map<db_id_t,AddressID_t*> &addrMap
		) ;
	void ReadRelocsFromDB
       	 	(
                	std::map<db_id_t,Instruction_t*>        &insnMap
        	);



};

