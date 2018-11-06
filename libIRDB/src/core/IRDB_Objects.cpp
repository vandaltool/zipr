#include <map>
#include <libIRDB-core.hpp>
#include <utils.hpp>
#include <utility>
#include <memory>
#include <assert.h>


using namespace libIRDB;
using namespace std;

#define ALLOF(a) begin(a),end(a)


IRDBObjects_t::~IRDBObjects_t()
{
        // All dynamically allocated DB objects
        // are held as shared pointers and don't need to be 
        // explicitly deleted.
}

shared_ptr<FileIR_t> IRDBObjects_t::addFileIR(const db_id_t variant_id, const db_id_t file_id)
{
        const auto it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
        {
		return nullptr;
        }
        else
        {           
	const auto the_file = (it->second).file;
	auto& the_fileIR = (it->second).fileIR;

	if(the_fileIR == NULL)
	{
		assert(the_file != NULL);

		assert(variant_map.find(variant_id) != variant_map.end());
		const auto & the_variant = *(variant_map.at(variant_id).get());

		the_fileIR = make_shared<FileIR_t>(the_variant, the_file);
		assert(the_fileIR != NULL);
	}

	// make sure static variable is set in the calling module -- IMPORTANT
	the_fileIR->SetArchitecture();
	return the_fileIR;
        }
}

int IRDBObjects_t::writeBackFileIR(const db_id_t file_id)
{
        const auto it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
            return 1;  
            const auto& the_file = (it->second).file;
            assert(the_file != NULL);
                    
	try
	{
		cout<<"Writing changes for "<<the_file->GetURL()<<endl;
		// make sure static variable is set in the calling module -- IMPORTANT
		const auto & the_fileIR = (it->second).fileIR;
		the_fileIR->SetArchitecture();
		the_fileIR->WriteToDB();
        	return 0;
	}
	catch (DatabaseError_t pnide)
	{
		cerr << "Unexpected database error: " << pnide << "file url: " << the_file->GetURL() << endl;
		return -1;
	}
	catch (...)
	{
		cerr << "Unexpected error file url: " << the_file->GetURL() << endl;
		return -1;
	}
	assert(0); // should not reach
        
}

void IRDBObjects_t::deleteFileIR(const db_id_t file_id)
{
        const auto it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
		return;

	auto& the_fileIR = (it->second).fileIR;
	if(the_fileIR != NULL)
	{
		// can't se just reset the_fileIR and let the shared_ptr delete when it's appropriate?  If the xform is holding a shared_ptr, who cares?
		// stated another way:  why bother asserting here?  is it a problem if someone else holds the fileIR shared ptr?
		assert(the_fileIR.use_count() <= 2);
		the_fileIR.reset();
	}
}

bool IRDBObjects_t::filesAlreadyPresent(const set<File_t*>& the_files) const
{
	// look for a missing file
	const auto missing_file_it=find_if(ALLOF(the_files), [&](const File_t* const f)
		{
			return (file_IR_map.find(f->GetBaseID()) == file_IR_map.end());
		});
        
	// return true if no files missing
        return missing_file_it==the_files.end();
}

shared_ptr<VariantID_t> IRDBObjects_t::addVariant(const db_id_t variant_id)
{
        const auto var_it = variant_map.find(variant_id);      

        if(var_it != variant_map.end())
        {
            return var_it->second;
        }

        auto the_variant = make_shared<VariantID_t>(variant_id);      

        assert(the_variant->IsRegistered()==true);
        // disallow variants that share shallow copies to both be read in
        // to prevent desynchronization. 
        assert(!filesAlreadyPresent(the_variant->GetFiles()));
	variant_map[variant_id]=the_variant;

        // add files
	for(auto &curr_file : the_variant->GetFiles())
	{
            auto curr_file_IR = shared_ptr<FileIR_t>();
            auto file_IR_info = FileIRInfo_t({curr_file, curr_file_IR});
            file_IR_map[curr_file->GetBaseID()]=file_IR_info;
        }
        
        return the_variant;
}

bool IRDBObjects_t::filesBeingShared(const VariantID_t* the_variant) const
{

	const auto &all_files=the_variant->GetFiles();
	const auto shared_file_it=find_if(ALLOF(all_files), [&](const File_t* file)
		{
		    const auto file_IR_info = file_IR_map.at(file->GetBaseID());
		    return (file_IR_info.fileIR.use_count() > 2);
		});
	return shared_file_it!=all_files.end();
}


int IRDBObjects_t::writeBackVariant(const db_id_t variant_id)
{
        const auto it = variant_map.find(variant_id);
        
        if(it == variant_map.end())
		return 1;
	try
	{
		cout<<"Writing changes for variant "<<variant_id<<endl;
		it->second->WriteToDB();
        	return 0;
	}
	catch (DatabaseError_t pnide)
	{
		cerr << "Unexpected database error: " << pnide << "variant ID: " << variant_id << endl;
		return -1;
	}
	catch (...)
	{
		cerr << "Unexpected error variant ID: " << variant_id << endl;
		return -1;
	}
	assert(0); // unreachable.
        
}

void IRDBObjects_t::deleteVariant(const db_id_t variant_id)
{
        const auto var_it = variant_map.find(variant_id);
        
        if(var_it == variant_map.end())
		return;

	// To prevent reading in the same files again while they are being used
	// somewhere else, which could lead to desynchronization
	assert(!filesBeingShared(var_it->second.get()));
	assert(var_it->second.use_count() <= 2);

	// remove files and file IRs
	for(const auto file : var_it->second->GetFiles())
	{
		file_IR_map.erase(file->GetBaseID());
	}
    
	// remove variant
	variant_map.erase(variant_id);
}

int IRDBObjects_t::writeBackAll(void)
{
        int ret_status = 0;

        // Write back FileIRs
	for(auto &file_pair : file_IR_map)
        {
		const int result = IRDBObjects_t::writeBackFileIR((file_pair.second.file)->GetBaseID());
		if(result != 0)
		{
			ret_status = -1;
		}
        }

        // Write back Variants
	for(auto & variant_pair : variant_map)
        {
		const int result = IRDBObjects_t::writeBackVariant((variant_pair.second)->GetBaseID());
		if(result != 0)
		{
			ret_status = -1;
		}
        }
        return ret_status;
}

void IRDBObjects_t::deleteAll(void)
{
        // Delete Variants (also deletes all files)
	for( auto &variant_pair : variant_map)
        {
            IRDBObjects_t::deleteVariant((variant_pair.second)->GetBaseID());
        }
}

pqxxDB_t* IRDBObjects_t::getDBInterface() const
{
        return pqxx_interface.get();
}

pqxxDB_t* IRDBObjects_t::resetDBInterface()
{
	pqxx_interface.reset(new pqxxDB_t());  // Aborts if Commit() has not been called
	BaseObj_t::SetInterface(pqxx_interface.get());
	return pqxx_interface.get();
}

