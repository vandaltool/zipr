/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */



#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace std;


/*
 * Create a new variant ID that is not yet in the database
 */
VariantID_t::VariantID_t() :
	BaseObj_t(NULL)
{
        schema_ver=CURRENT_SCHEMA;
        orig_pid=-1;       
        name="";
}


void VariantID_t::CreateTables()
{
	// note:  this tables are now part of File_t.
	assert(0);
}

VariantID_t::VariantID_t(db_id_t pid) : BaseObj_t(NULL)
{
	std::string q="select * from Variant_info where variant_id = " ;
	q+=to_string(pid);
	q+=";";


	try 
	{
		BaseObj_t::dbintr->IssueQuery(q);
	}
	catch (const std::exception &e)
	{
        	schema_ver=-1;
        	orig_pid=-1;       
        	name="";

		throw DatabaseError_t(DatabaseError_t::VariantTableNotRegistered); 
	};

	if(BaseObj_t::dbintr->IsDone())
		throw DatabaseError_t(DatabaseError_t::VariantNotInDatabase); 

        SetBaseID(atoi(BaseObj_t::dbintr->GetResultColumn("variant_id").c_str()));
        schema_ver=atoi(BaseObj_t::dbintr->GetResultColumn("schema_version_id").c_str());
        orig_pid=atoi(BaseObj_t::dbintr->GetResultColumn("orig_variant_id").c_str());
        name=(BaseObj_t::dbintr->GetResultColumn("name"));


	BaseObj_t::dbintr->MoveToNextRow();
	assert(BaseObj_t::dbintr->IsDone());

        ReadFilesFromDB();
}

bool VariantID_t::IsRegistered()
{
	return GetBaseID()!=BaseObj_t::NOT_IN_DATABASE;
}

bool VariantID_t::Register()
{
	assert(!IsRegistered());

	std::string q;
	q="insert into variant_info (schema_version_id,name) "
		"values('";
	q+=to_string(schema_ver);
	q+="','";
	q+=to_string(name);
	q+="')";
	q+="returning variant_id;";

	dbintr->IssueQuery(q);
	assert(!BaseObj_t::dbintr->IsDone());

	db_id_t newid=atoi(dbintr->GetResultColumn("variant_id").c_str());

	/* set IDs */
	SetBaseID(newid);
	if(NOT_IN_DATABASE==orig_pid)
		orig_pid=newid;

	BaseObj_t::dbintr->MoveToNextRow();
	assert(BaseObj_t::dbintr->IsDone());

}    

VariantID_t* VariantID_t::Clone(bool deep)
{
	assert(IsRegistered());	// cannot clone something that's not registered 

	// create the new program id 
	VariantID_t *ret=new VariantID_t;
	
	// set the inhereted fields 
	ret->SetName(name+"_cloneof"+to_string(GetBaseID()));
	ret->orig_pid=orig_pid;

	// register the new VID to the database. 
	ret->Register();
	// and write it to the database
	ret->WriteToDB();

	// (shallow) clone the file_info table entries 
	std::string q;

	// lastly update the variant_dependency table to make a copy of the rows in which 
	// the old variant depended upon.  The new rows will indicate that the 
	// new variant also depends on those files 
	q="insert into variant_dependency (variant_id, file_id, doip_id) select '";
	q+=to_string(ret->GetBaseID());
	q+="', file_id, doip_id from variant_dependency where variant_id='";
	q+=to_string(GetBaseID());
	q+="';";
	dbintr->IssueQuery(q);

	if(deep)
		ret->CloneFiles(files);

	return ret;
}       

void VariantID_t::CloneFiles(set<File_t*> &files)
{
	for(set<File_t*>::iterator fiter=files.begin(); fiter!=files.end(); ++fiter)
		files.insert(CloneFile(*fiter));
}

File_t* VariantID_t::CloneFile(File_t* fptr)
{
	std::string q;

	q ="insert into file_info "
	    "("
  	    "orig_file_id," 
  	    "url," 
  	    "hash," 
  	    "arch," 
  	    "elfoid," 
  	    "doip_id" 
	    ") "
	    "values ( '" + 
            to_string(fptr->orig_fid) + "', '" +
            to_string(fptr->url) + "', '" +
            to_string(fptr->hash) + "', '" +
            to_string(fptr->arch) + "', '" +
            to_string(fptr->elfoid) + "', '" +
            to_string(fptr->GetDoipID()) + 
	    "' ) ";
	q+=" returning file_id; ";


        dbintr->IssueQuery(q);
        assert(!BaseObj_t::dbintr->IsDone());

        db_id_t newfid=atoi(dbintr->GetResultColumn("file_id").c_str());

	std::string atn="atnfid"+to_string(newfid);
	std::string ftn="ftnfid"+to_string(newfid);
	std::string itn="itnfid"+to_string(newfid);
	std::string icfs="icfsfid"+to_string(newfid);
	std::string icfsmap="icfsmapfid"+to_string(newfid);
	std::string rtn="rtnfid"+to_string(newfid);
	std::string typ="typfid"+to_string(newfid);

	q ="update file_info set address_table_name='";
	q+=atn;
	q+="', function_table_name='";
	q+=ftn;
	q+="', instruction_table_name='";
	q+=itn;
	q+="', icfs_table_name='";
	q+=icfs;
	q+="', icfs_map_table_name='";
	q+=icfsmap;
	q+="', relocs_table_name='";
	q+=rtn;
	q+="', types_table_name='";
	q+=typ;
	q+="' where file_id='";
	q+=to_string(newfid);
	q+="' ; ";
	
        dbintr->IssueQuery(q);

	File_t* newfile=new File_t(newfid, fptr->orig_fid, fptr->url, fptr->hash, fptr->arch, fptr->elfoid, atn, ftn, itn, icfs, icfsmap, rtn, typ, fptr->GetDoipID());

	newfile->CreateTables();

        // first drop the old values
        q="drop table ";
        q+=itn;
        q+=" ; ";
        dbintr->IssueQuery(q);

        q="drop table ";
        q+=icfs;
        q+=" ; ";
        dbintr->IssueQuery(q);

        q="drop table ";
        q+=icfsmap;
        q+=" ; ";
        dbintr->IssueQuery(q);

        q="drop table ";
        q+=atn;
        q+=" ; ";
        dbintr->IssueQuery(q);

        q="drop table ";
        q+=ftn;
        q+=" ; ";
        dbintr->IssueQuery(q);

        q="drop table ";
        q+=rtn;
        q+=" ; ";
        dbintr->IssueQuery(q);

        q="drop table ";
        q+=typ;
        q+=" ; ";
        dbintr->IssueQuery(q);

        // next issue SQL to clone each table
        q="select * into ";
        q+=atn;
        q+=" from ";
        q+=fptr->address_table_name;
        q+=" ;";
        dbintr->IssueQuery(q);

        q="select * into ";
        q+=itn;
        q+=" from ";
        q+=fptr->instruction_table_name;
        q+=" ;";
        dbintr->IssueQuery(q);

        q="select * into ";
        q+=icfs;
        q+=" from ";
        q+=fptr->icfs_table_name;
        q+=" ;";
        dbintr->IssueQuery(q);

        q="select * into ";
        q+=icfsmap;
        q+=" from ";
        q+=fptr->icfs_map_table_name;
        q+=" ;";
        dbintr->IssueQuery(q);

        q="select * into ";
        q+=ftn;
        q+=" from ";
        q+=fptr->function_table_name;
        q+=" ;";
        dbintr->IssueQuery(q);

        q="select * into ";
        q+=rtn;
        q+=" from ";
        q+=fptr->relocs_table_name;
        q+=" ;";
        dbintr->IssueQuery(q);

        q="select * into ";
        q+=typ;
        q+=" from ";
        q+=fptr->types_table_name;
        q+=" ;";
        dbintr->IssueQuery(q);

	// update the variant dependency table to represent the deep clone 

	// update the variant dependency table to represent the deep clone 
	q =     "update variant_dependency set file_id='" + 
		to_string(newfid) + 
		"' where variant_id='" +
		to_string(GetBaseID()) + 
		"' AND file_id='" +
		to_string(fptr->GetBaseID()) + 
		"' ;";
        dbintr->IssueQuery(q);


	return newfile;

}

void VariantID_t::WriteToDB()
{
	assert(IsRegistered());

	std::string q="update variant_info SET ";
	q+=" schema_version_id = '" + to_string(schema_ver) + "', ";
	q+=" name = '"  + name  + "', ";
	q+=" orig_variant_id = '" + to_string(orig_pid) + "', ";
	q+=" doip_id = '" + to_string(GetDoipID()) + "' ";
	q+=" where variant_id = '" + to_string(GetBaseID()) + "';";

	dbintr->IssueQuery(q);
}

std::ostream& libIRDB::operator<<(std::ostream& out, const VariantID_t& pid)
{

	out << "("
		"variant_id="<<pid.GetBaseID()<<":"
		"schema="<<pid.schema_ver<<":"
		"orig_pid="<<pid.orig_pid<<":"
		"name="<<pid.name<<":"
		")" ;
	return out;
}


void VariantID_t::DropFromDB()
{
	assert(IsRegistered());

	string q;
	q+=string("delete from variant_dependency where variant_id = '") + to_string(GetBaseID()) + string("';");
	q+=string("delete from variant_info where variant_id = '") + to_string(GetBaseID()) + string("';");

	dbintr->IssueQuery(q);

	SetBaseID(NOT_IN_DATABASE);
	orig_pid=NOT_IN_DATABASE;
        schema_ver=CURRENT_SCHEMA;
}


File_t* VariantID_t::GetMainFile() const
{
	for(
		set<File_t*>::iterator it=files.begin();
		it!=files.end();
		++it
	   )
	{
		const char* name=(*it)->GetURL().c_str();
		if(strstr(name,"a.ncexe")!=NULL)
			return *it;
	}
	/* we should have found the main file somewhere. */
	assert(0);
}



void VariantID_t::ReadFilesFromDB()
{

	std::string q= "select  file_info.orig_file_id, file_info.address_table_name, file_info.instruction_table_name, file_info.icfs_table_name,file_info.icfs_map_table_name,"
		" file_info.function_table_name, file_info.relocs_table_name, file_info.types_table_name, file_info.file_id, file_info.url, file_info.hash,"
		" file_info.arch, file_info.type, file_info.elfoid, file_info.doip_id "
		" from file_info,variant_dependency "
		" where variant_dependency.variant_id = '" + to_string(GetBaseID()) + "' AND "
		" file_info.file_id = variant_dependency.file_id ; "; 

	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
// file_info.file_id, file_info.url, file_info.hash, file_info.arch, file_info.type, file_info.doip_id

		db_id_t file_id=atoi(dbintr->GetResultColumn("file_id").c_str());
		db_id_t orig_fid=atoi(dbintr->GetResultColumn("orig_file_id").c_str());
		std::string url=dbintr->GetResultColumn("url");
		std::string hash=dbintr->GetResultColumn("hash");
		std::string type=dbintr->GetResultColumn("type");
		int oid=atoi(dbintr->GetResultColumn("elfoid").c_str());
		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());
        	std::string atn=(BaseObj_t::dbintr->GetResultColumn("address_table_name"));
        	std::string ftn=(BaseObj_t::dbintr->GetResultColumn("function_table_name"));
        	std::string itn=(BaseObj_t::dbintr->GetResultColumn("instruction_table_name"));
        	std::string icfs=(BaseObj_t::dbintr->GetResultColumn("icfs_table_name"));
        	std::string icfs_map=(BaseObj_t::dbintr->GetResultColumn("icfs_map_table_name"));
        	std::string rtn=(BaseObj_t::dbintr->GetResultColumn("relocs_table_name"));
        	std::string typ=(BaseObj_t::dbintr->GetResultColumn("types_table_name"));


		File_t *newfile=new File_t(file_id,orig_fid,url,hash,type,oid,atn,ftn,itn,icfs,icfs_map,rtn,typ,doipid);

std::cout<<"Found file "<<file_id<<"."<<std::endl;
std::cout<<"  atn: " << atn << " ftn: " << ftn << " rtn: " << rtn << " typ: " << typ << std::endl;

		files.insert(newfile);

		dbintr->MoveToNextRow();
	}
}
