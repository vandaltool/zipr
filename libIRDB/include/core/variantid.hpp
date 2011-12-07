
#define CURRENT_SCHEMA 1

class VariantID_t : public BaseObj_t
{
    public:
        VariantID_t();        		// create a Variant ID not in the database 
        VariantID_t(db_id_t pid);       // read from the DB 

        bool IsRegistered();               
        bool Register();    // accesses DB

        VariantID_t* Clone();       // accesses DB

	void WriteToDB();

	void DropFromDB();

	std::string GetName() { return name; }
	void SetName(std::string newname) { name=newname;}

	std::string GetAddressTableName() { return address_table_name; }
	std::string GetFunctionTableName() { return function_table_name; }
	std::string GetInstructionTableName() { return instruction_table_name; }

	friend std::ostream& libIRDB::operator<<(std::ostream& out, const VariantID_t& pid);
	friend class VariantIR_t;
	friend class Function_t;
	friend class AddressID_t;
	friend class Instruction_t;

	db_id_t GetOriginalVariantID() const { return orig_pid;}

    private:
        schema_version_t schema_ver;
        db_id_t orig_pid;       // matches pid if this is an "original"
                                // Variant and not a cloned variant.

        std::string name;
        std::string address_table_name;
        std::string function_table_name;
        std::string instruction_table_name;

	void CreateTables();	// create the address, function and instruction tables 


};

std::ostream& operator<<(std::ostream& out, const VariantID_t& pid);
