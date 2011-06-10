// A variant of a problem, this
// may be an original variant
// (i.e., and unmodified program) or a modified variant.
class ProgramIR_t : public BaseObj_t
{
    public:

        // Create a program from the database
        ProgramIR_t(char* db_name, ProgramID progid);

        // create a program from scratch and store it in the DB.
        void ProgramIR_t(/* params to be specified */);
  
        // DB operations
        void ReadFromDB(const char* dbname, const ProgramID_t& id);  //accesses DB

        // accessors and mutators in one
        set<Functions_t*>& GetFunctions();
        set<Instructions_t*>& GetInstructions();
        set<Address_t*>&    GetAddresses();
    private:
        set<Function_t*> funcs;
        set<Instruction_t*> insns;
        set<Address_t*> addrs;
        set<File_t*> files;
        char* db_name;
        ProgramID progid;
}

