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
                    BaseObj_t::SetInterface(&pqxx_interface);
                };
		~IRDBObjects_t();

		// Add/delete file IRs for a variant. Takes in the base ID of the
                // file (File_t) that the FileIR_t is paired with. 
                // Cannot delete a file IR if it is also owned outside this object.
		// When deleting file IRs, have the option to write back to the DB.
                // If the write fails, the IR is still deleted.
		int AddFileIR(db_id_t variant_id, db_id_t file_id);
		int DeleteFileIR(db_id_t file_id, bool write_to_DB);
                // Add or delete a variant
                // When deleting a variant, have the option to write back to the DB.
                // Cannot delete a variant if it or its files are also owned outside this object.
                // If the write fails, the variant is still deleted.
                // Writing a variant does NOT write its files' IRs, but DOES delete them!
                int AddVariant(db_id_t variant_id); 
                int DeleteVariant(db_id_t variant_id, bool write_to_DB);
                
                // get a variant
                // returns shared_ptr(NULL) if no such variant
		std::shared_ptr<VariantID_t> GetVariant(db_id_t variant_id);
		// get a file IR
                // returns shared_ptr(NULL) if no such file IR
		std::shared_ptr<FileIR_t> GetFileIR(db_id_t file_id);
                // get a file
                // returns shared_ptr(NULL) if no such file
                std::shared_ptr<File_t> GetFile(db_id_t file_id);
                // Get DB interface
                pqxxDB_t* GetDBInterface();

		// Write back all variants and file IRs stored in this IRFiles_t object.
                // Also removes all variants and file IRs even if some writes fail.
                // Returns 1 if any writes fail.
                // Does NOT commit changes.
		bool WriteBackAll(void);
		
	private:
                pqxxDB_t pqxx_interface;
		// maps for speed of finding needed files, file IRs and/or variants
		// that have already been read from the DB
                
                // maps variant id to variant
		std::map<db_id_t, std::shared_ptr<VariantID_t>> variant_map;
                // maps file id to (file, file ir)
		std::map<db_id_t, std::pair<std::shared_ptr<File_t>, std::shared_ptr<FileIR_t>>> file_IR_map;
                
                // minor helpers (used to check assertions)
                bool FilesAlreadyPresent(std::set<File_t*> the_files);
                bool FilesBeingShared(std::shared_ptr<VariantID_t> the_variant);
};

#endif
 
