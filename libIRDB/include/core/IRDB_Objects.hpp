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
		using FileSet_t = std::set<File_t*>;
		IRDBObjects_t() 
                {
			pqxx_interface = std::unique_ptr<pqxxDB_t>(new pqxxDB_t());
			// set up interface to the sql server
			BaseObj_t::SetInterface(pqxx_interface.get());
                };
		~IRDBObjects_t();

		// Add/delete file IRs for a variant.
                // Cannot delete a file IR if it is also owned outside this object.
		// AddFileIR returns the added IR or the preexistent IR if it was already added.
		std::shared_ptr<FileIR_t> addFileIR(const db_id_t variant_id, const db_id_t file_id);    // Returns NULL if no such file exists
		void deleteFileIR(const db_id_t file_id);
                // Add or delete a variant
                // Cannot delete a variant if it or its files are also owned outside this object.
                // Deleting a variant does NOT write its files' IRs, but DOES delete them!
                // AddVariant returns the added variant or the preexistent variant if it was already added.
                std::shared_ptr<VariantID_t> addVariant(const db_id_t variant_id);
                void deleteVariant(db_id_t variant_id);
                
                // Get DB interface
                pqxxDB_t* getDBInterface() const;
		pqxxDB_t* resetDBInterface();

		// Write back variants and file IRs. Does NOT commit changes.
                int writeBackFileIR(const db_id_t file_id);
                int writeBackVariant(const db_id_t variant_id);   // Does NOT write back its files' IRs
		int writeBackAll(void);    // Returns -1 if any writes fail.
                
                void deleteAll(void);
		
	private:
                std::unique_ptr<pqxxDB_t> pqxx_interface;
		// type aliases of maps. maps allow speed of finding needed files, file IRs 
		// and/or variants that have already been read from the DB 
		using IdToVariantMap_t = std::map<db_id_t, std::shared_ptr<VariantID_t>>; 
		struct FileIRInfo_t
		{
  		    File_t * file;
  		    std::shared_ptr<FileIR_t> fileIR;
		};
		using IdToFileIRInfoMap_t = std::map<db_id_t, FileIRInfo_t>; 
                
		IdToVariantMap_t variant_map;
        	IdToFileIRInfoMap_t file_IR_map;
                
                // minor helpers (used to check assertions)
                bool filesAlreadyPresent(const FileSet_t& the_files) const;
                bool filesBeingShared(const VariantID_t* the_variant) const;
};

#endif
 
