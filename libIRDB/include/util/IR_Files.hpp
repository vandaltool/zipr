#ifndef IR_Files_h
#define IR_Files_h

// TODO: This should really use unordered maps and sets,
//       but I was having trouble building with those.
#include <map>
#include <utility>
#include <memory>
#include <set>

// If our toolchain use paradigm changes, this class can be extended
// to support adding, removing, writing back, and getting individual file IRs
// in a variant (may require adding more maps).

// *** A toolchain step should NOT delete pointers to any variant, file IR, or file object stored
//     in a IRFiles_t object. I have made pointers shared where possible to communicate this. ***
class IRFiles_t
{
	public:
		IRFiles_t() {};
		~IRFiles_t();

		// Add/remove file IRs.
		// Adding file IRs also adds the variant the files belong to.
		// Removing file IRs also removes the variant the files belong to.
		// When removing file IRs, have the option to write back to the DB.
		int  AddFileIRs(db_id_t variant_id);
		int  RemoveFileIRs(db_id_t variant_id, bool write_to_DB);
			
		// get a variant
		std::shared_ptr<VariantID_t> GetVariant(db_id_t variant_id);
		// get file IRs (returns a shared ptr to a set of shared ptrs to file IRs)
		std::shared_ptr<std::set<std::shared_ptr<FileIR_t>>> GetFileIRs(db_id_t variant_id);

		// Write back all variants and file IRs stored in this IRFiles_t object.
		int WriteBackAll(void);
		
	private:
		// maps for speed of finding needed file IRs and/or variants
		// that have already been read from the DB
		std::map<db_id_t, std::shared_ptr<VariantID_t>> variant_map;
		std::map<db_id_t, std::shared_ptr<std::set<std::shared_ptr<FileIR_t>>>> file_IRs_map;	

};

#endif
