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


VariantID_t::~VariantID_t()
{
	for(auto it : files)	
	{
		delete it;
	}
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
		BaseObj_t::dbintr->issueQuery(q);
	}
	catch (const std::exception &e)
	{
        	schema_ver=-1;
        	orig_pid=-1;       
        	name="";

		throw DatabaseError_t(DatabaseError_t::VariantTableNotRegistered); 
	};
	
	if(BaseObj_t::dbintr->isDone())
		throw DatabaseError_t(DatabaseError_t::VariantNotInDatabase); 

        setBaseID(atoi(BaseObj_t::dbintr->getResultColumn("variant_id").c_str()));
        schema_ver=atoi(BaseObj_t::dbintr->getResultColumn("schema_version_id").c_str());
        orig_pid=atoi(BaseObj_t::dbintr->getResultColumn("orig_variant_id").c_str());
        name=(BaseObj_t::dbintr->getResultColumn("name"));


	BaseObj_t::dbintr->moveToNextRow();
	assert(BaseObj_t::dbintr->isDone());

        ReadFilesFromDB();
}

bool VariantID_t::isRegistered() const
{
	return getBaseID()!=BaseObj_t::NOT_IN_DATABASE;
}

bool VariantID_t::registerID()
{
	assert(!isRegistered());

	std::string q;
	q="insert into variant_info (schema_version_id,name) "
		"values('";
	q+=to_string(schema_ver);
	q+="','";
	q+=to_string(name);
	q+="')";
	q+="returning variant_id;";

	dbintr->issueQuery(q);
	assert(!BaseObj_t::dbintr->isDone());

	db_id_t newid=atoi(dbintr->getResultColumn("variant_id").c_str());

	/* set IDs */
	setBaseID(newid);
	if(NOT_IN_DATABASE==orig_pid)
		orig_pid=newid;

	BaseObj_t::dbintr->moveToNextRow();
	assert(BaseObj_t::dbintr->isDone());

	return true;
}    

IRDB_SDK::VariantID_t* VariantID_t::clone(bool deep)
{
	assert(isRegistered());	// cannot clone something that's not registered 

	// create the new program id 
	VariantID_t *ret=new VariantID_t;
	
	// set the inhereted fields 
	ret->setName(name+"_cloneof"+to_string(getBaseID()));
	ret->orig_pid=orig_pid;

	// register the new VID to the database. 
	ret->registerID();
	// and write it to the database
	ret->WriteToDB();

	// (shallow) clone the file_info table entries 
	std::string q;

	// lastly update the variant_dependency table to make a copy of the rows in which 
	// the old variant depended upon.  The new rows will indicate that the 
	// new variant also depends on those files 
	q="insert into variant_dependency (variant_id, file_id, doip_id) select '";
	q+=to_string(ret->getBaseID());
	q+="', file_id, doip_id from variant_dependency where variant_id='";
	q+=to_string(getBaseID());
	q+="';";
	dbintr->issueQuery(q);

	if(deep)
		ret->CloneFiles(files);

	return ret;
}       

void VariantID_t::CloneFiles(FileSet_t &files)
{
	for(auto fiter=files.begin(); fiter!=files.end(); ++fiter)
	{
		auto the_file=dynamic_cast<File_t*>(*fiter);
		files.insert(CloneFile(the_file));
	}
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
            to_string(fptr->getDoipID()) + 
	    "' ) ";
	q+=" returning file_id; ";


        dbintr->issueQuery(q);
        assert(!BaseObj_t::dbintr->isDone());

        db_id_t newfid=atoi(dbintr->getResultColumn("file_id").c_str());

	std::string atn="atnfid"+to_string(newfid);
	std::string ftn="ftnfid"+to_string(newfid);
	std::string itn="itnfid"+to_string(newfid);
	std::string icfs="icfsfid"+to_string(newfid);
	std::string icfsmap="icfsmapfid"+to_string(newfid);
	std::string rtn="rtnfid"+to_string(newfid);
	std::string dtn="dtnfid"+to_string(newfid);
	std::string dtn_part2="dtnfid"+to_string(newfid)+"_part2";
	std::string typ="typfid"+to_string(newfid);
	std::string ehp="ehpfid"+to_string(newfid);
	std::string css="cssfid"+to_string(newfid);

	q ="update file_info set address_table_name='"+atn;
	q+="', function_table_name='"+ftn;
	q+="', instruction_table_name='"+itn;
	q+="', icfs_table_name='"+icfs;
	q+="', icfs_map_table_name='"+icfsmap;
	q+="', relocs_table_name='"+rtn;
	q+="', types_table_name='"+typ;
	q+="', scoop_table_name='"+dtn;
	q+="', ehpgm_table_name='"+ehp;
	q+="', ehcss_table_name='"+css;
	q+="' where file_id='";
	q+=to_string(newfid);
	q+="' ; ";
	
        dbintr->issueQuery(q);

	File_t* newfile=new File_t(newfid, fptr->orig_fid, fptr->url, fptr->hash, fptr->arch, 
		fptr->elfoid, atn, ftn, itn, icfs, icfsmap, rtn, typ, dtn, ehp, css, fptr->getDoipID());

	newfile->CreateTables();

        // first drop the old values
        q="drop table ";
        q+=itn;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=icfsmap;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=icfs;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=atn;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=ftn;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=rtn;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=typ;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=dtn;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=dtn_part2;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=ehp;
        q+=" ; ";
        dbintr->issueQuery(q);

        q="drop table ";
        q+=css;
        q+=" ; ";
        dbintr->issueQuery(q);


        // next issue SQL to clone each table
        q="select * into ";
        q+=atn;
        q+=" from ";
        q+=fptr->address_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=itn;
        q+=" from ";
        q+=fptr->instruction_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=icfs;
        q+=" from ";
        q+=fptr->icfs_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=icfsmap;
        q+=" from ";
        q+=fptr->icfs_map_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=ftn;
        q+=" from ";
        q+=fptr->function_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=rtn;
        q+=" from ";
        q+=fptr->relocs_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=typ;
        q+=" from ";
        q+=fptr->types_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=dtn;
        q+=" from ";
        q+=fptr->scoop_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=dtn_part2;
        q+=" from ";
        q+=fptr->scoop_table_name+"_part2";
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=ehp;
        q+=" from ";
        q+=fptr->ehpgm_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

        q="select * into ";
        q+=css;
        q+=" from ";
        q+=fptr->ehcss_table_name;
        q+=" ;";
        dbintr->issueQuery(q);

	// update the variant dependency table to represent the deep clone 
	q =     "update variant_dependency set file_id='" + 
		to_string(newfid) + 
		"' where variant_id='" +
		to_string(getBaseID()) + 
		"' AND file_id='" +
		to_string(fptr->getBaseID()) + 
		"' ;";
        dbintr->issueQuery(q);


	return newfile;

}

void VariantID_t::WriteToDB()
{
	assert(isRegistered());

	std::string q="update variant_info SET ";
	q+=" schema_version_id = '" + to_string(schema_ver) + "', ";
	q+=" name = '"  + name  + "', ";
	q+=" orig_variant_id = '" + to_string(orig_pid) + "', ";
	q+=" doip_id = '" + to_string(getDoipID()) + "' ";
	q+=" where variant_id = '" + to_string(getBaseID()) + "';";

	dbintr->issueQuery(q);
}

std::ostream& IRDB_SDK::operator<<(std::ostream& out, const IRDB_SDK::VariantID_t& pid)
{

	out << "(" << 
		"variant_id=" << pid.getBaseID()            << ":" <<
		"orig_pid="   << pid.getOriginalVariantID() << ":" <<
		"name="       << pid.getName()              << 
		")" ;
	return out;
}


void VariantID_t::DropFromDB()
{
	assert(isRegistered());

	string q;
	q+=string("delete from variant_dependency where variant_id = '") + to_string(getBaseID()) + string("';");
	q+=string("delete from variant_info where variant_id = '") + to_string(getBaseID()) + string("';");

	dbintr->issueQuery(q);

	setBaseID(NOT_IN_DATABASE);
	orig_pid=NOT_IN_DATABASE;
        schema_ver=CURRENT_SCHEMA;
}


IRDB_SDK::File_t* VariantID_t::getMainFile() const
{
	for(
		auto it=files.begin();
		it!=files.end();
		++it
	   )
	{
		if ((*it)->getURL().find("a.ncexe") != string::npos)
			return *it;
	}
	/* we should have found the main file somewhere. */
	assert(0);
}



void VariantID_t::ReadFilesFromDB()
{

	std::string q= "select file_info.orig_file_id, file_info.address_table_name, "
		" file_info.instruction_table_name, file_info.icfs_table_name,file_info.icfs_map_table_name, "
		" file_info.function_table_name, file_info.relocs_table_name, file_info.types_table_name, "
		" file_info.scoop_table_name, file_info.ehpgm_table_name, file_info.ehcss_table_name, file_info.file_id, file_info.url, file_info.hash,"
		" file_info.arch, file_info.type, file_info.elfoid, file_info.doip_id "
		" from file_info,variant_dependency "
		" where variant_dependency.variant_id = '" + to_string(getBaseID()) + "' AND "
		" file_info.file_id = variant_dependency.file_id ; "; 

	dbintr->issueQuery(q);

	while(!dbintr->isDone())
	{
// file_info.file_id, file_info.url, file_info.hash, file_info.arch, file_info.type, file_info.doip_id

		db_id_t file_id=atoi(dbintr->getResultColumn("file_id").c_str());
		db_id_t orig_fid=atoi(dbintr->getResultColumn("orig_file_id").c_str());
		std::string url=dbintr->getResultColumn("url");
		std::string hash=dbintr->getResultColumn("hash");
		std::string type=dbintr->getResultColumn("type");
		int oid=atoi(dbintr->getResultColumn("elfoid").c_str());
		db_id_t doipid=atoi(dbintr->getResultColumn("doip_id").c_str());
        	std::string atn=(BaseObj_t::dbintr->getResultColumn("address_table_name"));
        	std::string ftn=(BaseObj_t::dbintr->getResultColumn("function_table_name"));
        	std::string itn=(BaseObj_t::dbintr->getResultColumn("instruction_table_name"));
        	std::string dtn=(BaseObj_t::dbintr->getResultColumn("scoop_table_name"));
        	std::string ehp=(BaseObj_t::dbintr->getResultColumn("ehpgm_table_name"));
        	std::string css=(BaseObj_t::dbintr->getResultColumn("ehcss_table_name"));
        	std::string icfs=(BaseObj_t::dbintr->getResultColumn("icfs_table_name"));
        	std::string icfs_map=(BaseObj_t::dbintr->getResultColumn("icfs_map_table_name"));
        	std::string rtn=(BaseObj_t::dbintr->getResultColumn("relocs_table_name"));
        	std::string typ=(BaseObj_t::dbintr->getResultColumn("types_table_name"));


		File_t *newfile=new File_t(file_id,orig_fid,url,hash,type,oid,atn,ftn,itn,icfs,icfs_map,rtn,typ,dtn,ehp,css,doipid);

// std::cout<<"Found file "<<file_id<<"."<<std::endl;
// std::cout<<"  atn: " << atn << " ftn: " << ftn << " rtn: " << rtn << " typ: " << typ << std::endl;

		files.insert(newfile);

		dbintr->moveToNextRow();
	}
}


namespace IRDB_SDK
{

unique_ptr<VariantID_t> VariantID_t::factory(const DatabaseID_t& id)
{
        return unique_ptr<VariantID_t>(new libIRDB::VariantID_t(id));
}

unique_ptr<FileIR_t> FileIR_t::factory(VariantID_t *p_progid, File_t* p_fid)
{
	const auto progid=dynamic_cast<libIRDB::VariantID_t*>(p_progid);
	const auto fid=dynamic_cast<libIRDB::File_t*>(p_fid);
        return unique_ptr<FileIR_t>(new libIRDB::FileIR_t(*progid,fid));
}


}

