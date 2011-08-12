
#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
#include <map>
using namespace libIRDB;
using namespace std;

// Create a Variant from the database
VariantIR_t::VariantIR_t(VariantID_t newprogid) : BaseObj_t(NULL)
{
	orig_variant_ir_p=NULL;
	progid=newprogid;	
	if(progid.IsRegistered())
		ReadFromDB();

}
  
// DB operations
void VariantIR_t::ReadFromDB()
{
	std::map<db_id_t,File_t*>	fileMap=ReadFilesFromDB();
	std::map<db_id_t,Function_t*> 	funcMap=ReadFuncsFromDB(fileMap);
	std::map<db_id_t,AddressID_t*> 	addrMap=ReadAddrsFromDB(fileMap,funcMap);
	std::map<db_id_t,Instruction_t*> 	insnMap=ReadInsnsFromDB(fileMap,funcMap,addrMap);
}

std::map<db_id_t,File_t*> VariantIR_t::ReadFilesFromDB()
{
	std::map<db_id_t,File_t*> idMap;

	std::string q= "select  file_info.file_id, file_info.url, file_info.hash,"
		" file_info.arch, file_info.type, file_info.elfoid, file_info.doip_id "
		" from file_info,variant_dependency "
		" where variant_dependency.variant_id = '" + to_string(progid.GetBaseID()) + "' AND "
		" file_info.file_id = variant_dependency.file_id ; "; 

	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
// file_info.file_id, file_info.url, file_info.hash, file_info.arch, file_info.type, file_info.doip_id

		db_id_t file_id=atoi(dbintr->GetResultColumn("file_id").c_str());
		std::string url=dbintr->GetResultColumn("url");
		std::string hash=dbintr->GetResultColumn("hash");
		std::string type=dbintr->GetResultColumn("type");
		int oid=atoi(dbintr->GetResultColumn("elfoid").c_str());
		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		File_t *newfile=new File_t(file_id,url,hash,type,oid,doipid);

//std::cout<<"Found file "<<file_id<<"."<<std::endl;

		idMap[file_id]=newfile;

		files.insert(newfile);

		dbintr->MoveToNextRow();
	}

	return idMap;
}

std::map<db_id_t,Function_t*> VariantIR_t::ReadFuncsFromDB
	(
		std::map<db_id_t,File_t*> fileMap
	)
{
	std::map<db_id_t,Function_t*> idMap;

	std::string q= "select * from " + progid.function_table_name + " ; ";


	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
// function_id | file_id | name | stack_frame_size | out_args_region_size | use_frame_pointer | doip_id

		db_id_t fid=atoi(dbintr->GetResultColumn("function_id").c_str());
		db_id_t file_id=atoi(dbintr->GetResultColumn("file_id").c_str());
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

		Function_t *newfunc=new Function_t(fid,name,sfsize,oasize,useFP,fileMap[file_id]);

//std::cout<<"Found function "<<name<<"."<<std::endl;

		idMap[fid]=newfunc;

		funcs.insert(newfunc);

		dbintr->MoveToNextRow();
	}

	return idMap;
}

std::map<db_id_t,AddressID_t*> VariantIR_t::ReadAddrsFromDB  (         std::map<db_id_t,File_t*> fileMap,
                                                                        std::map<db_id_t,Function_t*> funcMap) 
{
	std::map<db_id_t,AddressID_t*> idMap;

	std::string q= "select * from " + progid.address_table_name + " ; ";


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


std::map<db_id_t,Instruction_t*> VariantIR_t::ReadInsnsFromDB (      std::map<db_id_t,File_t*> fileMap,
                                                                        std::map<db_id_t,Function_t*> funcMap,
                                                                        std::map<db_id_t,AddressID_t*> addrMap
                                                                        ) 
{
	std::map<db_id_t,Instruction_t*> idMap;
	std::map<db_id_t,db_id_t> fallthroughs;
	std::map<db_id_t,db_id_t> targets;

	std::string q= "select * from " + progid.instruction_table_name + " ; ";


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


void VariantIR_t::WriteToDB()
{
	/* assign each item a unique ID */
	SetBaseIDS();

	db_id_t j=-1;

	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ progid.instruction_table_name + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ progid.function_table_name    + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ progid.address_table_name     + string(" cascade;"));

	/* and now that everything has an ID, let's write to the DB */
	string q=string("");
	for(std::set<Function_t*>::const_iterator i=funcs.begin(); i!=funcs.end(); ++i)
		q+=(*i)->WriteToDB(&progid,j);
	dbintr->IssueQuery(q);

	q=string("");
	for(std::set<AddressID_t*>::const_iterator i=addrs.begin(); i!=addrs.end(); ++i)
		q+=(*i)->WriteToDB(&progid,j);
	dbintr->IssueQuery(q);

	q=string("");
	for(std::set<Instruction_t*>::const_iterator i=insns.begin(); i!=insns.end(); ++i)
		q+=(*i)->WriteToDB(&progid,j);
	dbintr->IssueQuery(q);
}



void VariantIR_t::SetBaseIDS()
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
	for(std::set<File_t*>::const_iterator i=files.begin(); i!=files.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());

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
	for(std::set<File_t*>::const_iterator i=files.begin(); i!=files.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
}
