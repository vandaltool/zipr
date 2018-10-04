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
        // All dynamically allocated members (DB objects)
        // are held as shared pointers and don't need to be 
        // explicitly deleted.
}


int IRDBObjects_t::AddFileIR(db_id_t variant_id, db_id_t file_id)
{
        map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator 
            it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
        {
            return 1;
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
            return 0;
        }
}


int IRDBObjects_t::DeleteFileIR(db_id_t file_id, bool write_to_DB)
{
        int ret_status = 0;
    
        map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator 
            it = file_IR_map.find(file_id);
        
        if(it == file_IR_map.end())
        {
            ret_status = 1;
        }
        else
        {
            if(it->second.second != NULL)
            {
                assert(it->second.second.unique());
                
                if(write_to_DB)
                {
                    shared_ptr<File_t> the_file = it->second.first;
                    assert(the_file != NULL);
                    
                    try
                    {
                        cout<<"Writing changes for "<<the_file->GetURL()<<endl;
                        (it->second.second)->WriteToDB();
                    }
                    catch (DatabaseError_t pnide)
                    {
                        cerr << "Unexpected database error: " << pnide << "file url: " << the_file->GetURL() << endl;
                        ret_status = 2;
                    }
                    catch (...)
                    {
                        cerr << "Unexpected error file url: " << the_file->GetURL() << endl;
                        ret_status = 2;
                    }
                }
                (it->second.second).reset();
            }
        }       
        
        return ret_status;
}


int IRDBObjects_t::AddVariant(db_id_t variant_id)
{
        shared_ptr<VariantID_t> the_variant = make_shared<VariantID_t>(variant_id);        
        assert(the_variant->IsRegistered()==true);
        
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
        
        return 0;
}


int IRDBObjects_t::DeleteVariant(db_id_t variant_id, bool write_to_DB)
{
        int ret_status = 0;
        map<db_id_t, shared_ptr<VariantID_t>>::iterator var_it = variant_map.find(variant_id);
        
        if(var_it == variant_map.end())
        {
            ret_status = 1;
        }
        else
        {
            bool files_being_shared = false;
            for(set<File_t*>::iterator file_it=var_it->second->GetFiles().begin();
                file_it!=var_it->second->GetFiles().end();
                ++file_it
               )
            {
                assert(file_IR_map.find((*file_it)->GetBaseID()) != file_IR_map.end());
                pair<shared_ptr<File_t>, shared_ptr<FileIR_t>> file_IR_pair = file_IR_map.at((*file_it)->GetBaseID());
                if(!file_IR_pair.first.unique() || !file_IR_pair.second.unique())
                {
                    files_being_shared = true;
                }
            }
            
            assert(!files_being_shared);
            assert(var_it->second.unique());
            
            if(write_to_DB)
            {
                try
                {
                    cout<<"Writing changes for variant "<<variant_id<<endl;
                    var_it->second->WriteToDB();
                }
                catch (DatabaseError_t pnide)
                {
                    cerr << "Unexpected database error: " << pnide << "variant ID: " << variant_id << endl;
                    ret_status = 2;
                }
                catch (...)
                {
                    cerr << "Unexpected error variant ID: " << variant_id << endl;
                    ret_status = 2;
                }
            }
            // remove files and file IRs
            for(set<File_t*>::iterator file_it2=var_it->second->GetFiles().begin();
                file_it2!=var_it->second->GetFiles().end();
                ++file_it2
               )
            {
                file_IR_map.erase((*file_it2)->GetBaseID());
            }
            
            // remove variant
            variant_map.erase(variant_id);
        }
        
        return ret_status;
}


bool IRDBObjects_t::WriteBackAll(void)
{
        bool all_successes = true;
    
        // Write back FileIRs
        for(map<db_id_t, pair<shared_ptr<File_t>, shared_ptr<FileIR_t>>>::iterator
                it = file_IR_map.begin(); 
                it != file_IR_map.end();
                ++it 
            )
        {
            int result = IRDBObjects_t::DeleteFileIR((it->second.first)->GetBaseID(), true);
            if(result != 0)
            {
                all_successes = false;
            }
        }
    
        // Write back Variants
        for(map<db_id_t, shared_ptr<VariantID_t>>::iterator
                it2 = variant_map.begin(); 
                it2 != variant_map.end();
                ++it2
            )
        {
            int result = IRDBObjects_t::DeleteVariant((it2->second)->GetBaseID(), true);
            if(result != 0)
            {
                all_successes = false;
            }
        }
        
        return all_successes;
}


shared_ptr<VariantID_t> IRDBObjects_t::GetVariant(db_id_t variant_id)
{
        map<db_id_t, shared_ptr<VariantID_t>>::iterator it = variant_map.find(variant_id);
        
        if(it == variant_map.end())
        {
            shared_ptr<VariantID_t> null_var;
            return null_var;
        }
        else
        {
            return it->second;
        }
}


shared_ptr<FileIR_t> IRDBObjects_t::GetFileIR(db_id_t file_id)
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
            return it->second.second;
        }
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
        return &pqxx_interface;
}

