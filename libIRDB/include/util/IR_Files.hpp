#ifndef IR_Files_h
#define IR_Files_h

#include <map>
#include <utility>
#include <memory>
#include <iterator>

class IRFiles_t
{
	public:
		IRFiles_t() {};
		~IRFiles_t();

		// add/remove file IRs (have option to write back to DB upon removal)
		// If choosing to write back to DB, may throw a database error exception.
		// If the write back fails, the FileIR will not be destroyed.
		void AddFileIR(FileIR_t&);
		void RemoveFileIR(FileIR_t&, bool);
		void RemoveFileIR(File_t&, bool);
	
		// has file IR?
		bool HasFileIR(File_t&);
		bool HasFileIR(FileIR_t&);

		// get file IR
		FileIR_t* GetFileIR(File_t&);
		
	private:
		// map for speed of finding if a needed file
		// has already been read from the DB
		std::map<File_t*, std::shared_ptr<FileIR_t>> file_IR_map;	

};

#endif
