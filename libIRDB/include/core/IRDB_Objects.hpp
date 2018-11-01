#ifndef IRDB_Objects_h
#define IRDB_Objects_h

#include <map>
#include <utility>
#include <memory>


// TODO: Should really use unordered maps but was getting build errors


// *** A toolchain step should NOT delete pointers to any variant, file IR, or file object stored
//     in a IRFiles_t object. I have made pointers shared where possible to communicate this. ***
class IRDBObjects_t
{
	public:
		IRDBObjects_t() 
                {
                    // set up interface to the sql server
                    BaseObj_t::SetInterface(pqxx_interface);
                };
		~IRDBObjects_t();

		// Add/delete file IRs for a variant.
                // Cannot delete a file IR if it is also owned outside this object.
		// AddFileIR returns the added IR or the preexistent IR if it was already added.
		std::shared_ptr<FileIR_t> AddFileIR(db_id_t variant_id, db_id_t file_id);    // Returns NULL if no such file exists
		int DeleteFileIR(db_id_t file_id);
                // Add or delete a variant
                // Cannot delete a variant if it or its files are also owned outside this object.
                // Deleting a variant does NOT write its files' IRs, but DOES delete them!
                // AddVariant returns the added variant or the preexistent variant if it was already added.
                std::shared_ptr<VariantID_t> AddVariant(db_id_t variant_id);
                int DeleteVariant(db_id_t variant_id);
                
                // Get DB interface
                pqxxDB_t* GetDBInterface();
		pqxxDB_t* ResetDBInterface();

		// Write back variants and file IRs. Does NOT commit changes.
                int WriteBackFileIR(db_id_t file_id);
                int WriteBackVariant(db_id_t variant_id);   // Does NOT write back its files' IRs
		int WriteBackAll(void);    // Returns -1 if any writes fail.
                
                int DeleteAll(void);
		
	private:
                pqxxDB_t *pqxx_interface = new pqxxDB_t();
		// type aliases of maps. maps allow speed of finding needed files, file IRs 
		// and/or variants that have already been read from the DB 
		using IdToVariantMap_t = std::map<db_id_t, const std::shared_ptr<VariantID_t>>; 

		using FileIRInfo_t = std::pair<File_t *const, std::shared_ptr<FileIR_t>>;
		using IdToFileIRInfoMap_t = std::map<db_id_t, FileIRInfo_t>; 
                
		IdToVariantMap_t variant_map;
        	IdToFileIRInfoMap_t file_IR_map;
                
                // minor helpers (used to check assertions)
                bool FilesAlreadyPresent(const std::set<File_t*>& the_files) const;
                bool FilesBeingShared(const std::shared_ptr<VariantID_t>& the_variant) const;
};

#endif
 
