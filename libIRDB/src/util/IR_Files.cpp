#include <map>
#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp>
#include <utility>
#include <memory>
#include <iterator>


using namespace libIRDB;
using namespace std;


IRFiles_t::~IRFiles_t()
{
	file_IR_map.clear(); // doesn't write back to DB
}


void IRFiles_t::AddFileIR(FileIR_t& IR_ref)
{
	File_t* file_ptr = IR_ref.GetFile();

	assert(file_ptr);

	file_IR_map.insert(pair<File_t*, shared_ptr<FileIR_t>>(file_ptr, shared_ptr<FileIR_t>(&IR_ref)));
}


// may throw a database error exception if writing back to DB.
// If the write back fails, the FileIR will not be destroyed.
void IRFiles_t::RemoveFileIR(FileIR_t& IR_ref, bool write_to_DB)
{
	if(!HasFileIR(IR_ref))
		return;

	if(write_to_DB)
		IR_ref.WriteToDB();

	File_t* file_ptr = IR_ref.GetFile();

	assert(file_ptr);

	file_IR_map.erase(file_ptr);	
}


// may throw a database error exception if writing back to DB.
// If the write back fails, the FileIR will not be destroyed.
void IRFiles_t::RemoveFileIR(File_t& file_ref, bool write_to_DB)
{
	if(!HasFileIR(file_ref))
                return;

        if(write_to_DB)
                GetFileIR(file_ref)->WriteToDB();

        file_IR_map.erase(&file_ref);
}


bool IRFiles_t::HasFileIR(File_t& file_ref)
{
	map<File_t*, shared_ptr<FileIR_t>>::iterator got = file_IR_map.find(&file_ref);

  	if(got == file_IR_map.end())
		return false;
	else
		return true;
}


bool IRFiles_t::HasFileIR(FileIR_t& IR_ref)
{
	File_t* file_ptr = IR_ref.GetFile();

	assert(file_ptr);	

	map<File_t*, shared_ptr<FileIR_t>>::iterator got = file_IR_map.find(file_ptr);

        if(got == file_IR_map.end())
                return false;
        else
                return true;
}


FileIR_t* IRFiles_t::GetFileIR(File_t& file_ref)
{
	map<File_t*, shared_ptr<FileIR_t>>::iterator got = file_IR_map.find(&file_ref);

        if(got == file_IR_map.end())
                return NULL;
        else
                return (got->second).get();		
}

