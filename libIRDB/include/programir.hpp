// A variant of a problem, this
// may be an original variant
// (i.e., and unmodified program) or a modified variant.
class ProgramIR_t : public BaseObj_t
{
    public:

        // Create a program from the database
        ProgramIR_t(char* db_name, ProgramID_t progid);

        // create a program from scratch and store it in the DB.
        ProgramIR_t(/* params to be specified */);
  
        // DB operations
        void ReadFromDB(const char* dbname, const ProgramID_t& id);  //accesses DB
        void WriteToDB();

        // accessors and mutators in one
        std::set<Function_t*>& GetFunctions();
        std::set<Instruction_t*>& GetInstructions();
        std::set<Address_t*>&    GetAddresses();
        std::set<File_t*>&    GetFiles();
    private:
        std::set<Function_t*> funcs;
        std::set<Instruction_t*> insns;
        std::set<Address_t*> addrs;
        std::set<File_t*> files;
        char* db_name;
        ProgramID_t progid;
};

