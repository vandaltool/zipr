#include <map>
#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp>
#include <utility>
#include <memory>
#include <assert.h>


using namespace libIRDB;
using namespace std;


IRDBObjects_t::~IRDBObjects_t()
{
	delete pqxx_interface;
        // All dynamically allocated DB objects
        // are held as shared pointers and don't need to be 
        // explicitly deleted.
}


shared_ptr<FileIR_t> IRDBObjects_t::AddFileIR(db_id_t variant_id, db_id_t file_id)
{
        map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator 
            it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
        {
            shared_ptr<FileIR_t> null_fileIR;
            return null_fileIR;
        }
        else
        {            
            if(it->second.second == NULL)
            {
                File_t* the_file = (it->second.first).get(); 
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


int IRDBObjects_t::WriteBackFileIR(db_id_t file_id)
{
        map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator 
            it = file_IR_map.find(file_id);
        
        if(it != file_IR_map.end())
        {
            shared_ptr<File_t> the_file = it->second.first;
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


int IRDBObjects_t::DeleteFileIR(db_id_t file_id)
{
        map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator 
            it = file_IR_map.find(file_id);
        
        if(it != file_IR_map.end())
        {
            if(it->second.second != NULL)
            {
                assert(it->second.second.unique());
                (it->second.second).reset();
            }
            return 0;
        }
        else
        {
            return -1;
        }
}


bool IRDBObjects_t::FilesAlreadyPresent(set<File_t*> the_files)
{
        for(set<File_t*>::iterator it=the_files.begin();
            it!=the_files.end();
            ++it
           )
        {       
            if(GetFile((*it)->GetBaseID()) != NULL)
            {
                return true;
            }
        }
        
        return false;
}


shared_ptr<VariantID_t> IRDBObjects_t::AddVariant(db_id_t variant_id)
{
        map<db_id_t, shared_ptr<VariantID_t>>::iterator var_it = variant_map.find(variant_id);      

        if(var_it != variant_map.end())
        {
            return var_it->second;
        }

        shared_ptr<VariantID_t> the_variant = make_shared<VariantID_t>(variant_id);      

        assert(the_variant->IsRegistered()==true);
        // disallow variants that share shallow copies to both be read in
        // to prevent desynchronization. 
        assert(!FilesAlreadyPresent(the_variant->GetFiles()));
        
        pair<db_id_t, shared_ptr<VariantID_t>> var_pair = make_pair(variant_id, the_variant);
        variant_map.insert(var_pair);

        // add files
        for(set<File_t*>::iterator it=the_variant->GetFiles().begin();
            it!=the_variant->GetFiles().end();
            ++it
           )
        {            
            shared_ptr<File_t> curr_file(*it);
            shared_ptr<FileIR_t> curr_file_IR;
                    
            pair<shared_ptr<File_t>, shared_ptr<FileIR_t>> file_IR_pair = make_pair(curr_file, curr_file_IR);
            pair<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>> file_map_pair = make_pair((*it)->GetBaseID(), file_IR_pair);
            
            file_IR_map.insert(file_map_pair);
        }
        
        return var_pair.second;
}


bool IRDBObjects_t::FilesBeingShared(shared_ptr<VariantID_t> the_variant)
{
        for(set<File_t*>::iterator file_it=the_variant->GetFiles().begin();
                file_it!=the_variant->GetFiles().end();
                ++file_it
               )
        {
            assert(file_IR_map.find((*file_it)->GetBaseID()) != file_IR_map.end());
            pair<shared_ptr<File_t>, shared_ptr<FileIR_t>> file_IR_pair = file_IR_map.at((*file_it)->GetBaseID());
            if(!file_IR_pair.first.unique() || !file_IR_pair.second.unique())
            {
                return true;
            }
        }
        
        return false;
}


int IRDBObjects_t::WriteBackVariant(db_id_t variant_id)
{
        map<db_id_t, shared_ptr<VariantID_t>>::iterator it = variant_map.find(variant_id);
        
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


int IRDBObjects_t::DeleteVariant(db_id_t variant_id)
{
        map<db_id_t, shared_ptr<VariantID_t>>::iterator var_it = variant_map.find(variant_id);
        
        if(var_it != variant_map.end())
        {
            // To prevent reading in the same files again while they are being used
            // somewhere else, which could lead to desynchronization
            assert(!FilesBeingShared(var_it->second));
            assert(var_it->second.unique());
            
            // remove files and file IRs
            for(set<File_t*>::iterator file_it=var_it->second->GetFiles().begin();
                file_it!=var_it->second->GetFiles().end();
                ++file_it
               )
            {
                file_IR_map.erase((*file_it)->GetBaseID());
            }
            
            // remove variant
            variant_map.erase(variant_id);
            return 0;
        }
        else
        {
            return -1;
        }
}


int IRDBObjects_t::WriteBackAll(void)
{
        int ret_status = 0;

        // Write back FileIRs
        for(map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator
                file_it = file_IR_map.begin(); 
                file_it != file_IR_map.end();
                ++file_it 
            )
        {
            int result = IRDBObjects_t::WriteBackFileIR((file_it->second.first)->GetBaseID());
            if(result != 0)
            {
                ret_status = -1;
            }
        }

        // Write back Variants
        for(map<db_id_t, shared_ptr<VariantID_t>>::iterator
                var_it = variant_map.begin(); 
                var_it != variant_map.end();
                ++var_it
            )
        {
            int result = IRDBObjects_t::WriteBackVariant((var_it->second)->GetBaseID());
            if(result != 0)
            {
                ret_status = -1;
            }
        }
        
        return ret_status;
}


int IRDBObjects_t::DeleteAll(void)
{
        // Delete Variants (also deletes all files)
        for(map<db_id_t, shared_ptr<VariantID_t>>::iterator
                it = variant_map.begin(); 
                it != variant_map.end();
                ++it
            )
        {
            int result = IRDBObjects_t::DeleteVariant((it->second)->GetBaseID());
            assert(result == 0);
        }
        
        return 0;
}


shared_ptr<File_t> IRDBObjects_t::GetFile(db_id_t file_id)
{
        map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator 
            it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
        {
            shared_ptr<File_t> null_file;
            return null_file;
        }
        else
        {
            return it->second.first;
        }
}


pqxxDB_t* IRDBObjects_t::GetDBInterface()
{
        return pqxx_interface;
}


pqxxDB_t* IRDBObjects_t::ResetDBInterface()
{
	delete pqxx_interface;  // Aborts if Commit() has not been called 
	pqxx_interface = new pqxxDB_t();
	BaseObj_t::SetInterface(pqxx_interface);
	return pqxx_interface;
}

