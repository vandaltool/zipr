class ProgramID_t : public BaseObj_t
{
    public:
        ProgramID_t(/* params to be filled in */);       
        bool IsRegistered();               
        bool Register();    // accesses DB

        ProgramID_t Clone();       // accesses DB

    private:
        schema_version_t schema_ver;
        db_id_t orig_pid;       // matches pid if this is an "original"
                                // program and not a cloned variant.

        string name;
        string address_table_name;
        string function_table_name;
        string instruction_table_name;

}

