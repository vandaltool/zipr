#ifndef IRDB_Objects_h
#define IRDB_Objects_h

namespace libIRDB
{

// TODO: Should really use unordered maps but was getting build errors


// *** A toolchain step should NOT delete pointers to any variant, file IR, or file object stored
//     in a IRFiles_t object. I have made pointers shared where possible to communicate this. ***
class IRDBObjects_t : virtual public IRDB_SDK::IRDBObjects_t
{
	public:
		IRDBObjects_t() 
                {
			pqxx_interface = std::unique_ptr<pqxxDB_t>(new pqxxDB_t());
			// set up interface to the sql server
			BaseObj_t::setInterface(pqxx_interface.get());
                };
		virtual ~IRDBObjects_t();

		// Add/delete file IRs for a variant.
		// Step does not have ownership of fileIR (can't make assumptions about its
		// lifetime!), and a call to DeleteFileIR will render any pointers to that fileIR dangling.            
		// AddFileIR returns the added IR or the preexistent IR if it was already added.
		IRDB_SDK::FileIR_t* addFileIR(const IRDB_SDK::DatabaseID_t variant_id, const IRDB_SDK::DatabaseID_t file_id);    // Returns NULL if no such file exists
		void deleteFileIR(const IRDB_SDK::DatabaseID_t file_id);
                // Add or delete a variant
                // Step does not have ownership of variant (can't make assumptions about its lifetime!), and a call to DeleteVariant will render any pointers to that variant dangling.
                // Deleting a variant does NOT write its files' IRs, but DOES delete them!
                // AddVariant returns the added variant or the preexistent variant if it was already added.
		IRDB_SDK::VariantID_t* addVariant(const IRDB_SDK::DatabaseID_t variant_id);
                void deleteVariant(IRDB_SDK::DatabaseID_t variant_id);
                
                // Get DB interface
		IRDB_SDK::pqxxDB_t* getDBInterface() const;
		IRDB_SDK::pqxxDB_t* resetDBInterface();

		// Write back variants and file IRs. Does NOT commit changes.
                int writeBackFileIR(const IRDB_SDK::DatabaseID_t file_id, std::ostream *verbose_logging=nullptr);
                int writeBackVariant(const IRDB_SDK::DatabaseID_t variant_id);   // Does NOT write back its files' IRs
		int writeBackAll(std::ostream* verbose_logging=nullptr);    // Returns -1 if any writes fail.
                
                void deleteAll(void);

		void tidyIR();
		
	private:
                std::unique_ptr<pqxxDB_t> pqxx_interface;
		// type aliases of maps. maps allow speed of finding needed files, file IRs 
		// and/or variants that have already been read from the DB 
		using IdToVariantMap_t = std::map<IRDB_SDK::DatabaseID_t, std::unique_ptr<VariantID_t>>; 
		struct FileIRInfo_t
		{
  		    File_t * file;
  		    std::unique_ptr<FileIR_t> fileIR;

		    FileIRInfo_t() : file(nullptr), fileIR(nullptr)
       		    {
               	    } 

		};
		using IdToFileIRInfoMap_t = std::map<IRDB_SDK::DatabaseID_t, FileIRInfo_t>; 
                
		IdToVariantMap_t variant_map;
        	IdToFileIRInfoMap_t file_IR_map;

		// minor helper
		bool filesAlreadyPresent(const FileSet_t&) const;
                
};

 
}
#endif
