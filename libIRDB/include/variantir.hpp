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

    private:

        void ReadFromDB();  //accesses DB

        std::set<Function_t*> funcs;
        std::set<Instruction_t*> insns;
        std::set<AddressID_t*> addrs;
        std::set<File_t*> files;
        VariantID_t progid;

	std::map<db_id_t,File_t*> ReadFilesFromDB();
	std::map<db_id_t,Function_t*> ReadFuncsFromDB ( 	std::map<db_id_t,File_t*> fileMap) ;
	std::map<db_id_t,AddressID_t*> ReadAddrsFromDB  ( 	std::map<db_id_t,File_t*> fileMap, 
									std::map<db_id_t,Function_t*> funcMap) ;
	std::map<db_id_t,Instruction_t*> ReadInsnsFromDB (	std::map<db_id_t,File_t*> fileMap, 
									std::map<db_id_t,Function_t*> funcMap,
									std::map<db_id_t,AddressID_t*> addrMap
									) ;

};

