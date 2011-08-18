// A variant of a problem, this
// may be an original variant
// (i.e., and unmodified Variant) or a modified variant.
class VariantIR_t : public BaseObj_t
{
    public:

        // Create a Variant from the database
        VariantIR_t(VariantID_t progid);
  
        // DB operations
        void WriteToDB();

        // accessors and mutators in one
        std::set<Function_t*>& GetFunctions() { return funcs; }
        std::set<Instruction_t*>& GetInstructions() { return insns; }
        std::set<AddressID_t*>&    GetAddresses() { return addrs; }
        std::set<File_t*>&    GetFiles() { return files; }

	// generate the spri rules into the output file, fout.
	void generate_spri(std::ostream &fout);

	// generate spri, assume that orig_varirp is the original variant. 
	void generate_spri(VariantIR_t *orig_varirp, std::ostream &fout);

	void SetBaseIDS();


    private:

	// a pointer to the original variants IR, NULL means not yet loaded.
	VariantIR_t* orig_variant_ir_p;


        void ReadFromDB();  //accesses DB

        std::set<Function_t*> funcs;
        std::set<Instruction_t*> insns;
        std::set<AddressID_t*> addrs;
        std::set<File_t*> files;
        VariantID_t progid;

	std::map<db_id_t,File_t*> ReadFilesFromDB();
	std::map<db_id_t,AddressID_t*> ReadAddrsFromDB(std::map<db_id_t,File_t*> fileMap);
	std::map<db_id_t,Function_t*> ReadFuncsFromDB
		(
		 	std::map<db_id_t,File_t*> fileMap,
			std::map<db_id_t,AddressID_t*> addrMap
		);
	std::map<db_id_t,Instruction_t*> ReadInsnsFromDB 
		(	
			std::map<db_id_t,File_t*> fileMap, 
			std::map<db_id_t,Function_t*> funcMap,
			std::map<db_id_t,AddressID_t*> addrMap
		) ;

};

