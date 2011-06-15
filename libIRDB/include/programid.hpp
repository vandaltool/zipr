class ProgramID_t : public BaseObj_t
{
    public:
        ProgramID_t();        		// create a program ID not in the database 
        ProgramID_t(db_id_t pid);       // read from the DB 

        bool IsRegistered();               
        bool Register();    // accesses DB

        ProgramID_t Clone();       // accesses DB

	void WriteToDB();

	friend std::ostream& libIRDB::operator<<(std::ostream& out, const ProgramID_t& pid);

    private:
        schema_version_t schema_ver;
        db_id_t orig_pid;       // matches pid if this is an "original"
                                // program and not a cloned variant.

        std::string name;
        std::string address_table_name;
        std::string function_table_name;
        std::string instruction_table_name;

};

std::ostream& operator<<(std::ostream& out, const ProgramID_t& pid);
