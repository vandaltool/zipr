
#define CURRENT_SCHEMA 2

class VariantID_t;

std::ostream& operator<<(std::ostream& out, const libIRDB::VariantID_t& pid);

class VariantID_t : public BaseObj_t
{
    public:
        VariantID_t();        		// create a Variant ID not in the database 
        VariantID_t(db_id_t pid);       // read from the DB 

        bool IsRegistered();               
        bool Register();    // accesses DB

        VariantID_t* Clone(bool deep=true);       // accesses DB

	void WriteToDB();

	void DropFromDB();

        std::set<File_t*>&    GetFiles() { return files; }

	std::string GetName() { return name; }
	void SetName(std::string newname) { name=newname;}

	File_t* GetMainFile() const;

	friend std::ostream& libIRDB::operator<<(std::ostream& out, const VariantID_t& pid);
	friend class FileIR_T;
	friend class Function_t;
	friend class AddressID_t;
	friend class Instruction_t;

	db_id_t GetOriginalVariantID() const { return orig_pid;}
	
	void CloneFiles(std::set<File_t*>& files);
	File_t* CloneFile(File_t* fptr);

    private:
        schema_version_t schema_ver;
        db_id_t orig_pid;       // matches pid if this is an "original"
                                // Variant and not a cloned variant.

        std::string name;

	void CreateTables();	// create the address, function and instruction tables 

        std::set<File_t*> files;

        void  ReadFilesFromDB();




};

