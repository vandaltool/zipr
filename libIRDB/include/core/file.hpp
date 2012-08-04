// An ELF file as represented by the DB
class File_t : public BaseObj_t
{
    public:
        // create new item.
        File_t(db_id_t file_id, db_id_t orig_fid, std::string url, std::string hash, std::string arch, int elfoid, 
		std::string atn, std::string ftn, std::string itn, std::string rtn, db_id_t doipid);

        File_t(db_id_t file_id) : BaseObj_t(NULL) { assert(0);}          // read from DB       
        void WriteToDB() { assert(0); }   // writes to DB ID is not -1.

        std::string GetAddressTableName() { return address_table_name; }
        std::string GetFunctionTableName() { return function_table_name; }
        std::string GetInstructionTableName() { return instruction_table_name; }
        std::string GetRelocationsTableName() { return relocs_table_name; }
        std::string GetURL() { return url; }

	void CreateTables();

	int GetELFOID() { return elfoid; };

        friend class FileIR_t;
        friend class Function_t;
        friend class AddressID_t;
        friend class Instruction_t;
        friend class VariantID_t;
        friend class Relocation_t;



    private:
	db_id_t orig_fid;
        std::string url;
        std::string hash;
        std::string arch;       
        std::string address_table_name;
        std::string function_table_name;
        std::string instruction_table_name;
        std::string relocs_table_name;
	int elfoid;
};
