
#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h>
#include <map>
using namespace libIRDB;
using namespace std;

static map<Function_t*,db_id_t> entry_points;


static void UpdateEntryPoints(std::map<db_id_t,Instruction_t*> 	&insnMap)
{
	/* for each function, look up the instruction that's the entry point */
	for(	map<Function_t*,db_id_t>::const_iterator it=entry_points.begin();
		it!=entry_points.end();
		++it
	   )
	{
		Function_t* func=(*it).first;
		db_id_t func_entry_id=(*it).second;

		assert(insnMap[func_entry_id]);
		func->SetEntryPoint(insnMap[func_entry_id]);
//		cout<<"Function named "<<func->GetName()<< " getting entry point set to "<<insnMap[func_entry_id]->GetComment()<<"."<<endl;
	}
		
}

// Create a Variant from the database
FileIR_t::FileIR_t(const VariantID_t &newprogid, File_t* fid) : BaseObj_t(NULL)
{
	orig_variant_ir_p=NULL;
	progid=newprogid;	

	if(fid==NULL)
		fileptr=newprogid.GetMainFile();
	else
		fileptr=fid;

	if(progid.IsRegistered())
		ReadFromDB();

}
  
// DB operations
void FileIR_t::ReadFromDB()
{
	entry_points.clear();

	std::map<db_id_t,AddressID_t*> 	addrMap=ReadAddrsFromDB();
	std::map<db_id_t,Function_t*> 	funcMap=ReadFuncsFromDB(addrMap);
	std::map<db_id_t,Instruction_t*> 	insnMap=ReadInsnsFromDB(funcMap,addrMap);
	ReadRelocsFromDB(insnMap);

	UpdateEntryPoints(insnMap);
}


std::map<db_id_t,Function_t*> FileIR_t::ReadFuncsFromDB
	(
        	std::map<db_id_t,AddressID_t*> &addrMap
	)
{
	std::map<db_id_t,Function_t*> idMap;

	std::string q= "select * from " + fileptr->function_table_name + " ; ";

	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
// function_id | file_id | name | stack_frame_size | out_args_region_size | use_frame_pointer | doip_id

		db_id_t fid=atoi(dbintr->GetResultColumn("function_id").c_str());
		db_id_t entry_point_id=atoi(dbintr->GetResultColumn("entry_point_id").c_str());
		std::string name=dbintr->GetResultColumn("name");
		int sfsize=atoi(dbintr->GetResultColumn("stack_frame_size").c_str());
		int oasize=atoi(dbintr->GetResultColumn("out_args_region_size").c_str());
// postgresql encoding of boolean can be 'true', '1', 'T', 'y'
                bool useFP=false;
		const char *useFPstr= dbintr->GetResultColumn("use_frame_pointer").c_str();
                if (strlen(useFPstr) > 0)
		{
			if (useFPstr[0] == 't' || useFPstr[0] == 'T' || useFPstr[0] == '1' || useFPstr[0] == 'y' || useFPstr[0] == 'Y')
				useFP = true;
		}

		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		Function_t *newfunc=new Function_t(fid,name,sfsize,oasize,useFP,NULL); 
		entry_points[newfunc]=entry_point_id;
		
//std::cout<<"Found function "<<name<<"."<<std::endl;

		idMap[fid]=newfunc;

		funcs.insert(newfunc);

		dbintr->MoveToNextRow();
	}

	return idMap;
}


std::map<db_id_t,AddressID_t*> FileIR_t::ReadAddrsFromDB  
	(
	) 
{
	std::map<db_id_t,AddressID_t*> idMap;

	std::string q= "select * from " + fileptr->address_table_name + " ; ";


	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
//   address_id            integer PRIMARY KEY,
//  file_id               integer REFERENCES file_info,
//  vaddress_offset       text,
//  doip_id               integer DEFAULT -1


		db_id_t aid=atoi(dbintr->GetResultColumn("address_id").c_str());
		db_id_t file_id=atoi(dbintr->GetResultColumn("file_id").c_str());
		int vaddr=atoi(dbintr->GetResultColumn("vaddress_offset").c_str());
		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		AddressID_t *newaddr=new AddressID_t(aid,file_id,vaddr);

//std::cout<<"Found address "<<aid<<"."<<std::endl;

		idMap[aid]=newaddr;

		addrs.insert(newaddr);

		dbintr->MoveToNextRow();
	}

	return idMap;
}


std::map<db_id_t,Instruction_t*> FileIR_t::ReadInsnsFromDB 
	(      
        std::map<db_id_t,Function_t*> &funcMap,
        std::map<db_id_t,AddressID_t*> &addrMap
        ) 
{
	std::map<db_id_t,Instruction_t*> idMap;
	std::map<db_id_t,db_id_t> fallthroughs;
	std::map<db_id_t,db_id_t> targets;

	std::string q= "select * from " + fileptr->instruction_table_name + " ; ";


	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{

//  address_id                integer REFERENCES #PROGNAME#_address,
//  parent_function_id        integer,
//  orig_address_id           integer REFERENCES #PROGNAME#_address,
//  fallthrough_address_id    integer,
//  target_address_id         integer,
//  data                      bytea,
//  callback                  text,
//  comment                   text,
//  doip_id                   integer DEFAULT -1


		db_id_t instruction_id=atoi(dbintr->GetResultColumn("instruction_id").c_str());
		db_id_t aid=atoi(dbintr->GetResultColumn("address_id").c_str());
		db_id_t parent_func_id=atoi(dbintr->GetResultColumn("parent_function_id").c_str());
		db_id_t orig_address_id=atoi(dbintr->GetResultColumn("orig_address_id").c_str());
		db_id_t fallthrough_address_id=atoi(dbintr->GetResultColumn("fallthrough_address_id").c_str());
		db_id_t targ_address_id=atoi(dbintr->GetResultColumn("target_address_id").c_str());
		std::string data=(dbintr->GetResultColumn("data"));
		std::string callback=(dbintr->GetResultColumn("callback"));
		std::string comment=(dbintr->GetResultColumn("comment"));
		db_id_t indirect_branch_target_address_id = atoi(dbintr->GetResultColumn("ind_target_address_id").c_str());
		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		std::string isIndStr=(dbintr->GetResultColumn("ind_target_address_id"));

                AddressID_t* indTarg = NULL;
		if (indirect_branch_target_address_id != NOT_IN_DATABASE) 
			indTarg = addrMap[indirect_branch_target_address_id];

		Instruction_t *newinsn=new Instruction_t(instruction_id,
			addrMap[aid],
			funcMap[parent_func_id],
			orig_address_id,
			data, callback, comment, indTarg, doipid);
	
		if(funcMap[parent_func_id])
			funcMap[parent_func_id]->GetInstructions().insert(newinsn);

//std::cout<<"Found address "<<aid<<"."<<std::endl;

		idMap[instruction_id]=newinsn;
		fallthroughs[instruction_id]=fallthrough_address_id;
		targets[instruction_id]=targ_address_id;

		insns.insert(newinsn);

		dbintr->MoveToNextRow();
	}

	for(std::map<db_id_t,Instruction_t*>::const_iterator i=idMap.begin(); i!=idMap.end(); ++i)
	{
		Instruction_t *instr=(*i).second;
		db_id_t fallthroughid=fallthroughs[instr->GetBaseID()];
		if(idMap[fallthroughid])	
			instr->SetFallthrough(idMap[fallthroughid]);
		db_id_t targetid=targets[instr->GetBaseID()];
		if(idMap[targetid])	
			instr->SetTarget(idMap[targetid]);
	}


	return idMap;
}

void FileIR_t::ReadRelocsFromDB
	(
		std::map<db_id_t,Instruction_t*> 	&insnMap
	)
{
	std::string q= "select * from " + fileptr->relocs_table_name + " ; ";
	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
                db_id_t reloc_id=atoi(dbintr->GetResultColumn("reloc_id").c_str());
                int reloc_offset=atoi(dbintr->GetResultColumn("reloc_offset").c_str());
                std::string reloc_type=(dbintr->GetResultColumn("reloc_type"));
                db_id_t instruction_id=atoi(dbintr->GetResultColumn("instruction_id").c_str());
                db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		Relocation_t *reloc=new Relocation_t(reloc_id,reloc_offset,reloc_type);

		assert(insnMap[instruction_id]!=NULL);

		insnMap[instruction_id]->GetRelocations().insert(reloc);
		relocs.insert(reloc);

		dbintr->MoveToNextRow();
	}

}




void FileIR_t::WriteToDB()
{
	/* assign each item a unique ID */
	SetBaseIDS();

	db_id_t j=-1;

	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->instruction_table_name + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->function_table_name    + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->address_table_name     + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->relocs_table_name     + string(" cascade;"));

	/* and now that everything has an ID, let's write to the DB */
	string q=string("");
	for(std::set<Function_t*>::const_iterator i=funcs.begin(); i!=funcs.end(); ++i)
	{
		q+=(*i)->WriteToDB(fileptr,j);
		if(q.size()>1024*1024)
		{
			dbintr->IssueQuery(q);
			q=string("");
		}
			
	}
	dbintr->IssueQuery(q);

	q=string("");
	for(std::set<AddressID_t*>::const_iterator i=addrs.begin(); i!=addrs.end(); ++i)
	{
		q+=(*i)->WriteToDB(fileptr,j);
		if(q.size()>1024*1024)
		{
			dbintr->IssueQuery(q);
			q=string("");
		}
	}
	dbintr->IssueQuery(q);

	q=string("");
	for(std::set<Instruction_t*>::const_iterator i=insns.begin(); i!=insns.end(); ++i)
	{	
		q+=(*i)->WriteToDB(fileptr,j);
		if(q.size()>1024*1024)
		{
			dbintr->IssueQuery(q);
			q=string("");
		}
	}

	dbintr->IssueQuery(q);
}


void FileIR_t::SetBaseIDS()
{
#define MAX(a,b) (((a)>(b)) ? (a) : (b))

	/* find the highest database ID */
	db_id_t j=0;
	for(std::set<Function_t*>::const_iterator i=funcs.begin(); i!=funcs.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(std::set<AddressID_t*>::const_iterator i=addrs.begin(); i!=addrs.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(std::set<Instruction_t*>::const_iterator i=insns.begin(); i!=insns.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(std::set<Relocation_t*>::const_iterator i=relocs.begin(); i!=relocs.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());

	/* increment past the max ID so we don't duplicate */
	j++;

	/* for anything that's not yet in the DB, assign an ID to it */
	for(std::set<Function_t*>::const_iterator i=funcs.begin(); i!=funcs.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(std::set<AddressID_t*>::const_iterator i=addrs.begin(); i!=addrs.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(std::set<Instruction_t*>::const_iterator i=insns.begin(); i!=insns.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(std::set<Relocation_t*>::const_iterator i=relocs.begin(); i!=relocs.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
}

std::string Relocation_t::WriteToDB(File_t* fid, Instruction_t* myinsn)
{
	string q;
        q ="insert into " + fid->relocs_table_name;
	q+="(reloc_id,reloc_offset,reloc_type,instruction_id,doip_id) "+
                string(" VALUES (") +
                string("'") + to_string(GetBaseID())          + string("', ") +
                string("'") + to_string(offset)               + string("', ") +
                string("'") + (type)                          + string("', ") +
                string("'") + to_string(myinsn->GetBaseID())  + string("', ") +
                string("'") + to_string(GetDoipID())          + string("') ; ") ;
	return q;	
}
