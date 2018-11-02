#include <map>
#include <libIRDB-core.hpp>
#include <utils.hpp>
#include <utility>
#include <memory>
#include <assert.h>


using namespace libIRDB;
using namespace std;


IRDBObjects_t::~IRDBObjects_t()
{
        // All dynamically allocated DB objects
        // are held as shared pointers and don't need to be 
        // explicitly deleted.
}


shared_ptr<FileIR_t> IRDBObjects_t::addFileIR(db_id_t variant_id, db_id_t file_id)
{
        const auto it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
        {
            shared_ptr<FileIR_t> null_fileIR;
            return null_fileIR;
        }
        else
        {            
            if(it->second.second == NULL)
            {
                File_t *const the_file = it->second.first; 
                assert(the_file != NULL);
            
                assert(variant_map.find(variant_id) != variant_map.end());
                VariantID_t& the_variant = *(variant_map.at(variant_id).get());
                
                it->second.second = make_shared<FileIR_t>(the_variant, the_file);
                assert(it->second.second != NULL);
            }
	    // make sure static variable is set in the calling module -- IMPORTANT
	    (it->second.second)->SetArchitecture();
            return it->second.second;
        }
}


int IRDBObjects_t::writeBackFileIR(db_id_t file_id)
{
        const auto it = file_IR_map.find(file_id);
        
        if(it != file_IR_map.end())
        {
            File_t *const the_file = it->second.first;
            assert(the_file != NULL);
                    
            try
            {
                cout<<"Writing changes for "<<the_file->GetURL()<<endl;
		// make sure static variable is set in the calling module -- IMPORTANT
                (it->second.second)->SetArchitecture();
	        (it->second.second)->WriteToDB();
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
        }
        else
        {
            return 1;  
        }
        
        return 0;
}


void IRDBObjects_t::deleteFileIR(db_id_t file_id)
{
        const auto it = file_IR_map.find(file_id);
        
        if(it != file_IR_map.end())
        {
            if(it->second.second != NULL)
            {
                assert(it->second.second.use_count() <= 2);
                (it->second.second).reset();
            }
        } 
}


bool IRDBObjects_t::filesAlreadyPresent(const set<File_t*>& the_files) const
{
        for(set<File_t*>::const_iterator it=the_files.begin();
            it!=the_files.end();
            ++it
           )
        {    
            if(file_IR_map.find((*it)->GetBaseID()) != file_IR_map.end())
            {
                return true;
            }
        }
        
        return false;
}


shared_ptr<VariantID_t> IRDBObjects_t::addVariant(db_id_t variant_id)
{
        const auto var_it = variant_map.find(variant_id);      

        if(var_it != variant_map.end())
        {
            return var_it->second;
        }

        const auto the_variant = make_shared<VariantID_t>(variant_id);      

        assert(the_variant->IsRegistered()==true);
        // disallow variants that share shallow copies to both be read in
        // to prevent desynchronization. 
        assert(!filesAlreadyPresent(the_variant->GetFiles()));
        
        const auto var_pair = make_pair(variant_id, the_variant);
        variant_map.insert(var_pair);

        // add files
        for(set<File_t*>::const_iterator it=the_variant->GetFiles().begin();
            it!=the_variant->GetFiles().end();
            ++it
           )
        {            
	    File_t *const curr_file = *it;
            shared_ptr<FileIR_t> curr_file_IR;
                    
            auto file_IR_pair = make_pair(curr_file, curr_file_IR);
            auto file_map_pair = make_pair((*it)->GetBaseID(), file_IR_pair);
            
            file_IR_map.insert(file_map_pair);
        }
        
        return var_pair.second;
}


bool IRDBObjects_t::filesBeingShared(const shared_ptr<VariantID_t>& the_variant) const
{
        for(set<File_t*>::const_iterator file_it=the_variant->GetFiles().begin();
                file_it!=the_variant->GetFiles().end();
                ++file_it
               )
        {
            assert(file_IR_map.find((*file_it)->GetBaseID()) != file_IR_map.end());
            const pair<File_t*, shared_ptr<FileIR_t>> file_IR_pair = file_IR_map.at((*file_it)->GetBaseID());
            if(file_IR_pair.second.use_count() > 2)
            {
                return true;
            }
        }
        
        return false;
}


int IRDBObjects_t::writeBackVariant(db_id_t variant_id)
{
        const auto it = variant_map.find(variant_id);
        
        if(it != variant_map.end())
        {
            try
            {
                cout<<"Writing changes for variant "<<variant_id<<endl;
                it->second->WriteToDB();
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
        }
        else
        {
            return 1;
        }
        
        return 0;
}


void IRDBObjects_t::deleteVariant(db_id_t variant_id)
{
        const auto var_it = variant_map.find(variant_id);
        
        if(var_it != variant_map.end())
        {
            // To prevent reading in the same files again while they are being used
            // somewhere else, which could lead to desynchronization
            assert(!filesBeingShared(var_it->second));
            assert(var_it->second.use_count() <= 2);
            
            // remove files and file IRs
            for(set<File_t*>::const_iterator file_it=var_it->second->GetFiles().begin();
                file_it!=var_it->second->GetFiles().end();
                ++file_it
               )
            {
                file_IR_map.erase((*file_it)->GetBaseID());
            }
            
            // remove variant
            variant_map.erase(variant_id);
        } 
}


int IRDBObjects_t::writeBackAll(void)
{
        int ret_status = 0;

        // Write back FileIRs
        for(auto file_it = file_IR_map.begin(); 
                 file_it != file_IR_map.end();
                 ++file_it 
            )
        {
            const int result = IRDBObjects_t::writeBackFileIR((file_it->second.first)->GetBaseID());
            if(result != 0)
            {
                ret_status = -1;
            }
        }

        // Write back Variants
        for(auto var_it = variant_map.begin(); 
                 var_it != variant_map.end();
                 ++var_it
            )
        {
            const int result = IRDBObjects_t::writeBackVariant((var_it->second)->GetBaseID());
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
        for(auto it = variant_map.begin(); 
                 it != variant_map.end();
                 ++it
            )
        {
            IRDBObjects_t::deleteVariant((it->second)->GetBaseID());
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

