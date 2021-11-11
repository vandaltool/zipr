
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

#include <keystone/keystone.h>
#include <all.hpp>
#include <irdb-util>
#include <cstdlib>
#include <map>
#include <fstream>
#include <elf.h>
#include <stdlib.h>
#include <algorithm>
#include <sys/wait.h>
#include <iomanip>
#include <irdb-util>
#include <endian.h>

#include "cmdstr.hpp"
#include "assemblestr.hpp"

#include <pqxx/tablewriter.hxx>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

using namespace libIRDB;
using namespace std;


#define SCOOP_CHUNK_SIZE (10*1024*1024)  /* 10 mb  */
#define ALLOF(a) begin(a),end(a)


#undef EIP

static void UpdateEntryPoints(
	const std::map<db_id_t,Instruction_t*> 	&insnMap,
	const map<Function_t*,db_id_t>& entry_points)
{
	/* for each function, look up the instruction that's the entry point */
	for(	map<Function_t*,db_id_t>::const_iterator it=entry_points.begin();
		it!=entry_points.end();
		++it
	   )
	{
		Function_t* func=(*it).first;
		db_id_t func_entry_id=(*it).second;

		assert(func_entry_id==-1 || insnMap.at(func_entry_id));
		func->setEntryPoint(insnMap.at(func_entry_id));
//		cout<<"Function named "<<func->getName()<< " getting entry point set to "<<insnMap[func_entry_id]->getComment()<<"."<<endl;
	}
		
}

static void UpdateUnresolvedEhCallSites(
	const std::map<db_id_t,Instruction_t*> 	&insnMap,
	const std::map<EhCallSite_t*,db_id_t> & unresolvedEhcss)
{
	for(const auto &i : unresolvedEhcss)
	{
		const auto& ehcs=i.first; 
		const auto& insnid=i.second;
		const auto& insn=insnMap.at(insnid);
		assert(insn);
		ehcs->setLandingPad(insn);
	}
}

static virtual_offset_t strtovo(std::string s)
{
        return IRDB_SDK::strtoint<virtual_offset_t>(s);
}

// Create a Variant from the database
FileIR_t::FileIR_t(const VariantID_t &newprogid, File_t* fid) 
			: BaseObj_t(NULL), progid((VariantID_t&) newprogid)
	
{
	orig_variant_ir_p=NULL;

	if(fid==NULL)
		fileptr=dynamic_cast<File_t*>(newprogid.getMainFile());
	else
		fileptr=fid;

	if(progid.isRegistered())
	{
		ReadFromDB();
		setArchitecture();
	}

}

FileIR_t::~FileIR_t()
{
	for(auto i : funcs ) delete i;
	for(auto i : insns ) delete i;
	for(auto i : addrs ) delete i;
	for(auto i : relocs) delete i;
	for(auto i : types ) delete i;

	// @todo: clear icfs_t
}
  
// DB operations
void FileIR_t::ReadFromDB()
{
    	ReadIRDB_start = clock();


	auto entry_points=map<Function_t*,db_id_t>();
	auto unresolvedICFS=std::map<Instruction_t*, db_id_t>();
	auto unresolvedEhCallSites=std::map<EhCallSite_t*,db_id_t>();
	auto objMap=std::map<db_id_t,BaseObj_t*>();
	auto addressToInstructionMap=std::map<db_id_t,Instruction_t*>();

	auto ehpgmMap 	= ReadEhPgmsFromDB(); 
	auto ehcsMap 	= ReadEhCallSitesFromDB(unresolvedEhCallSites); 
	auto typesMap 	= ReadTypesFromDB(types); 
	auto addrMap	= ReadAddrsFromDB();
	auto funcMap	= ReadFuncsFromDB(addrMap, typesMap,entry_points);
	auto scoopMap	= ReadScoopsFromDB(addrMap, typesMap);
	auto insnMap	= ReadInsnsFromDB(funcMap,addrMap,ehpgmMap,ehcsMap,addressToInstructionMap, unresolvedICFS);


	ReadAllICFSFromDB(addressToInstructionMap, unresolvedICFS);

	// put the scoops, instructions, ehpgms, eh call sites into the object map.
	// if relocs end up on other objects, we'll need to add them to.  for now only these things.
	objMap.insert(insnMap.begin(), insnMap.end());
	objMap.insert(scoopMap.begin(), scoopMap.end());
	objMap.insert(ehcsMap.begin(), ehcsMap.end());
	objMap.insert(ehpgmMap.begin(), ehpgmMap.end());
	ReadRelocsFromDB(objMap);

	UpdateEntryPoints(insnMap,entry_points);
	UpdateUnresolvedEhCallSites(insnMap,unresolvedEhCallSites);

	ReadIRDB_end = clock();


}


void FileIR_t::changeRegistryKey(IRDB_SDK::Instruction_t *p_orig, IRDB_SDK::Instruction_t *p_updated)
{
	auto orig=dynamic_cast<libIRDB::Instruction_t*>(p_orig);
	auto updated=dynamic_cast<libIRDB::Instruction_t*>(p_updated);

	if(assembly_registry.find(orig) != assembly_registry.end())
	{
		assembly_registry[updated] = assembly_registry[orig];

		assembly_registry.erase(orig);
	}
}


void FileIR_t::assembleRegistry()
{
	if(assembly_registry.size() == 0)
		return;

	const auto bits = getArchitectureBitWidth();
	auto count = (size_t)0;
	auto *encode = (char *)NULL;
	auto size = (size_t)0;

	const auto mode = (bits == 32) ? KS_MODE_32 : 
                      (bits == 64) ? KS_MODE_64 :
                      throw std::invalid_argument("Cannot map IRDB bit size to keystone bit size");
    
	const auto machinetype = getArchitecture()->getMachineType();
	const auto arch = (machinetype == IRDB_SDK::admtI386 || machinetype == IRDB_SDK::admtX86_64) ? KS_ARCH_X86 :
	                  (machinetype == IRDB_SDK::admtArm32) ? KS_ARCH_ARM :
	                  (machinetype == IRDB_SDK::admtAarch64) ? KS_ARCH_ARM64 : 
	                  (machinetype == IRDB_SDK::admtMips64 || machinetype == IRDB_SDK::admtMips32) ? KS_ARCH_MIPS :
	                  throw std::invalid_argument("Cannot map IRDB architecture to keystone architure");

	auto ks = (ks_engine *)NULL;
	const auto err = ks_open(arch, mode, &ks);
	assert(err == KS_ERR_OK);

	ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_NASM);

	//Build and set assembly string
	for(auto it : assembly_registry) 
	{
		assemblestr(ks, it.first, it.second.c_str(), encode, size, count);
	}

	ks_close(ks);
	assembly_registry.clear();
}

void FileIR_t::registerAssembly(IRDB_SDK::Instruction_t *p_instr, string assembly)
{
	auto instr=dynamic_cast<Instruction_t*>(p_instr);
	assert(instr);
	assembly_registry[instr] = assembly;
}

void FileIR_t::unregisterAssembly(IRDB_SDK::Instruction_t *p_instr)
{
	auto instr=dynamic_cast<Instruction_t*>(p_instr);
	assert(instr);
	assembly_registry.erase(instr);
}

std::string FileIR_t::lookupAssembly(IRDB_SDK::Instruction_t *p_instr)
{
	auto instr=dynamic_cast<Instruction_t*>(p_instr);
	assert(instr);
	if (assembly_registry.find(instr) != assembly_registry.end())
		return assembly_registry[instr];
	else
		return std::string();
}

std::map<db_id_t,Function_t*> FileIR_t::ReadFuncsFromDB
	(
        	std::map<db_id_t,AddressID_t*> &addrMap,
		std::map<db_id_t,Type_t*> &typesMap,
		map<Function_t*,db_id_t> &entry_points
	)
{
	auto idMap=std::map<db_id_t,Function_t*> ();

	auto q=std::string("select * from ") + fileptr->function_table_name + " ; ";

	dbintr->issueQuery(q);

	while(!dbintr->isDone())
	{
// function_id | file_id | name | stack_frame_size | out_args_region_size | use_frame_pointer | is_safe | doip_id

		db_id_t fid=atoi(dbintr->getResultColumn("function_id").c_str());
		db_id_t entry_point_id=atoi(dbintr->getResultColumn("entry_point_id").c_str());
		std::string name=dbintr->getResultColumn("name");
		int sfsize=atoi(dbintr->getResultColumn("stack_frame_size").c_str());
		int oasize=atoi(dbintr->getResultColumn("out_args_region_size").c_str());
		db_id_t function_type_id=atoi(dbintr->getResultColumn("type_id").c_str());
// postgresql encoding of boolean can be 'true', '1', 'T', 'y'
		bool useFP=false;
		bool isSafe=false;
		string useFPString=dbintr->getResultColumn("use_frame_pointer"); 
		string isSafeString=dbintr->getResultColumn("is_safe"); 
		const char *useFPstr=useFPString.c_str();
		const char *isSafestr=isSafeString.c_str();
		if (strlen(useFPstr) > 0)
		{
			if (useFPstr[0] == 't' || useFPstr[0] == 'T' || useFPstr[0] == '1' || useFPstr[0] == 'y' || useFPstr[0] == 'Y')
				useFP = true;
		}

		if (strlen(isSafestr) > 0)
		{
			if (isSafestr[0] == 't' || isSafestr[0] == 'T' || isSafestr[0] == '1' || isSafestr[0] == 'y' || isSafestr[0] == 'Y')
				isSafe = true;
		}

		// handle later?
		//db_id_t doipid=atoi(dbintr->getResultColumn("doip_id").c_str());

		FuncType_t* fnType = NULL;
		if (typesMap.count(function_type_id) > 0)
			fnType = dynamic_cast<FuncType_t*>(typesMap[function_type_id]);
		Function_t *newfunc=new Function_t(fid,name,sfsize,oasize,useFP,isSafe,fnType, NULL); 
		assert(newfunc);
		entry_points[newfunc]=entry_point_id;
		
//std::cout<<"Found function "<<name<<"."<<std::endl;

		idMap[fid]=newfunc;

		funcs.insert(newfunc);

		dbintr->moveToNextRow();
	}

	return idMap;
}


std::map<db_id_t,EhCallSite_t*> FileIR_t::ReadEhCallSitesFromDB
	(
		map<EhCallSite_t*,db_id_t> &unresolvedEhCssLandingPads // output arg.
	)
{
	auto ehcsMap=std::map<db_id_t,EhCallSite_t*>();

	std::string q= "select * from " + fileptr->getEhCallSiteTableName() + " ; ";

	for(dbintr->issueQuery(q); !dbintr->isDone(); dbintr->moveToNextRow())
	{
		const auto string_to_vec=[&](const string& str ) ->  vector<int>
		{
			auto out=vector<int>();
			istringstream s(str);
			auto in=(int)0;
			while ( s >> in)
				out.push_back(in);
			return out;
		};
		/* 
		 * ehcs_id         integer,        -- id of this object.
		 * tt_encoding     integer,        -- the encoding of the type table.
		 * lp_insn_id      integer         -- the landing pad instruction's id.
		 */


		const auto eh_cs_id=atoi(dbintr->getResultColumn("ehcs_id").c_str());
		const auto tt_encoding=atoi(dbintr->getResultColumn("tt_encoding").c_str());
		const auto &ttov=string(dbintr->getResultColumn("ttov").c_str());
		const auto lp_insn_id=atoi(dbintr->getResultColumn("lp_insn_id").c_str());

		auto newEhCs=new EhCallSite_t(eh_cs_id,tt_encoding,NULL); // create the call site with an unresolved LP
		newEhCs->GetTTOrderVector()=string_to_vec(ttov);
		eh_css.insert(newEhCs);					  // record that it exists.
		ehcsMap[eh_cs_id]=newEhCs;				  // record the map for when we read instructions.
		if(lp_insn_id != BaseObj_t::NOT_IN_DATABASE)
			unresolvedEhCssLandingPads[newEhCs]=lp_insn_id;		  // note that the LP is unresolved
	}

	return ehcsMap;
}

std::map<db_id_t,EhProgram_t*> FileIR_t::ReadEhPgmsFromDB()
{
	auto idMap = std::map<db_id_t,EhProgram_t*>();

	auto q=std::string("select * from ") + fileptr->ehpgm_table_name + " ; ";
	dbintr->issueQuery(q);

	auto decode_pgm=[](const string& encoded_pgm, EhProgramListing_t& decoded_pgm)
	{

		auto split=[](const string& str, const string& delim, EhProgramListing_t& tokens) -> void
		{
			auto prev = size_t(0);
			auto pos = size_t(0);
			do
			{
				pos = str.find(delim, prev);
				if (pos == string::npos) pos = str.length();
				string token = str.substr(prev, pos-prev);
				if (!token.empty()) tokens.push_back(token);
				prev = pos + delim.length();
			}
			while (pos < str.length() && prev < str.length());
		};

		auto decode_in_place=[](string& to_decode) -> void
		{
			auto charToHex=[](uint8_t value) -> uint8_t
			{
				if (value >= '0' && value <= '9') return value - '0';
				else if (value >= 'A' && value <= 'F') return value - 'A' + 10;
				else if (value >= 'a' && value <= 'f') return value - 'a' + 10;
				assert(false);
			};


			auto out=string("");
			while(to_decode.size() > 0)
			{
				// to-decode should have pairs of characters that represent individual bytes.
				assert(to_decode.size() >= 2);
				auto val =  uint8_t ( charToHex(to_decode[0])*16  + charToHex(to_decode[1]) ); 
				out += val;
				to_decode.erase(0,2);
			}

			to_decode=out;
		};

		split(encoded_pgm, ",", decoded_pgm);

		// decode each one
		for(auto& i : decoded_pgm)
			decode_in_place(i);
		
		
	};

	while(!dbintr->isDone())
	{
		/* 
		 * eh_pgm_id       integer,
		 * caf             integer,
		 * daf             integer,
		 * ptrsize         integer,
		 * cie_program     text,
		 * fde_program     text
		 */


		const auto eh_pgm_id=atoi(dbintr->getResultColumn("eh_pgm_id").c_str());
		const auto caf=atoi(dbintr->getResultColumn("caf").c_str());
		const auto daf=atoi(dbintr->getResultColumn("daf").c_str());
		const auto rr=atoi(dbintr->getResultColumn("return_register").c_str());
		const auto ptrsize=atoi(dbintr->getResultColumn("ptrsize").c_str());
		const auto& encoded_cie_program = dbintr->getResultColumn("cie_program");
		const auto& encoded_fde_program = dbintr->getResultColumn("fde_program");

		auto new_ehpgm=new EhProgram_t(eh_pgm_id, caf, daf, rr, ptrsize, {}, {});
		decode_pgm(encoded_cie_program, new_ehpgm->GetCIEProgram());
		decode_pgm(encoded_fde_program, new_ehpgm->GetFDEProgram());

		idMap[eh_pgm_id]=new_ehpgm;
		eh_pgms.insert(new_ehpgm);
		dbintr->moveToNextRow();
	}

	return idMap;
}

std::map<db_id_t,AddressID_t*> FileIR_t::ReadAddrsFromDB  
	(
	) 
{
	std::map<db_id_t,AddressID_t*> idMap;

	std::string q= "select * from " + fileptr->address_table_name + " ; ";


	dbintr->issueQuery(q);

	while(!dbintr->isDone())
	{
//   address_id            integer PRIMARY KEY,
//  file_id               integer REFERENCES file_info,
//  vaddress_offset       bigint,
//  doip_id               integer DEFAULT -1


		db_id_t aid=atoi(dbintr->getResultColumn("address_id").c_str());
		db_id_t file_id=atoi(dbintr->getResultColumn("file_id").c_str());
		virtual_offset_t vaddr=strtovo(dbintr->getResultColumn("vaddress_offset"));

		// handle later?
		//db_id_t doipid=atoi(dbintr->getResultColumn("doip_id").c_str());

		AddressID_t *newaddr=new AddressID_t(aid,file_id,vaddr);

//std::cout<<"Found address "<<aid<<"."<<std::endl;

		idMap[aid]=newaddr;

		addrs.insert(newaddr);

		dbintr->moveToNextRow();
	}

	return idMap;
}


static string encoded_to_raw_string(string encoded)
{
	int len = encoded.length();
	std::string raw;
	for(int i=0; i< len; i+=2)
	{
	    string byte = encoded.substr(i,2);
	    char chr = (char) (int)strtol(byte.c_str(), nullptr, 16);
	    raw.push_back(chr);
	}
	return raw;
}

std::map<db_id_t,Instruction_t*> FileIR_t::ReadInsnsFromDB 
	(      
        const std::map<db_id_t,Function_t*> &funcMap,
        const std::map<db_id_t,AddressID_t*> &addrMap,
        const std::map<db_id_t,EhProgram_t*> &ehpgmMap,
        const std::map<db_id_t,EhCallSite_t*> &ehcsMap,
	std::map<db_id_t,Instruction_t*> &addressToInstructionMap,
	std::map<Instruction_t*, db_id_t> &unresolvedICFS
        ) 
{
	std::map<db_id_t,Instruction_t*> idMap;
	std::map<db_id_t,db_id_t> fallthroughs;
	std::map<db_id_t,db_id_t> targets;

	std::string q= "select * from " + fileptr->instruction_table_name + " ; ";


	dbintr->issueQuery(q);

	while(!dbintr->isDone())
	{

//  address_id                integer REFERENCES #PROGNAME#_address,
//  parent_function_id        integer,
//  orig_address_id           integer REFERENCES #PROGNAME#_address,
//  fallthrough_address_id    integer,
//  target_address_id         integer,
//  icfs_id                   integer,
//  data                      bytea,
//  callback                  text,
//  comment                   text,
//  doip_id                   integer DEFAULT -1


		db_id_t instruction_id=atoi(dbintr->getResultColumn("instruction_id").c_str());
		db_id_t aid=atoi(dbintr->getResultColumn("address_id").c_str());
		db_id_t parent_func_id=atoi(dbintr->getResultColumn("parent_function_id").c_str());
		db_id_t orig_address_id=atoi(dbintr->getResultColumn("orig_address_id").c_str());
		db_id_t fallthrough_address_id=atoi(dbintr->getResultColumn("fallthrough_address_id").c_str());
		db_id_t targ_address_id=atoi(dbintr->getResultColumn("target_address_id").c_str());
		db_id_t icfs_id=atoi(dbintr->getResultColumn("icfs_id").c_str());
		db_id_t eh_pgm_id=atoi(dbintr->getResultColumn("ehpgm_id").c_str());
		db_id_t eh_cs_id=atoi(dbintr->getResultColumn("ehcss_id").c_str());
		std::string encoded_data=(dbintr->getResultColumn("data"));
		std::string data=encoded_to_raw_string(encoded_data);
		std::string callback=(dbintr->getResultColumn("callback"));
		std::string comment=(dbintr->getResultColumn("comment"));
		db_id_t indirect_branch_target_address_id = atoi(dbintr->getResultColumn("ind_target_address_id").c_str());
		db_id_t doipid=atoi(dbintr->getResultColumn("doip_id").c_str());

		std::string isIndStr=(dbintr->getResultColumn("ind_target_address_id"));

                auto indTarg=(AddressID_t*)NULL;
		if (indirect_branch_target_address_id != NOT_IN_DATABASE) 
			indTarg = addrMap.at(indirect_branch_target_address_id);

		auto parent_func=(Function_t*)NULL;
		if(parent_func_id!= NOT_IN_DATABASE) parent_func=funcMap.at(parent_func_id);

		Instruction_t *newinsn=new Instruction_t(instruction_id,
			addrMap.at(aid),
			parent_func,
			orig_address_id,
			data, callback, comment, indTarg, doipid);

		if(eh_pgm_id != NOT_IN_DATABASE) newinsn->setEhProgram(ehpgmMap.at(eh_pgm_id));
		if(eh_cs_id != NOT_IN_DATABASE) newinsn->setEhCallSite(ehcsMap.at(eh_cs_id));
	
		if(parent_func)
		{
			parent_func->GetInstructions().insert(newinsn);
			newinsn->setFunction(funcMap.at(parent_func_id));
		}

//std::cout<<"Found address "<<aid<<"."<<std::endl;

		idMap[instruction_id]=newinsn;
		fallthroughs[instruction_id]=fallthrough_address_id;
		targets[instruction_id]=targ_address_id;

		addressToInstructionMap[aid] = newinsn;
		insns.insert(newinsn);

		if (icfs_id == NOT_IN_DATABASE)
		{
			newinsn->setIBTargets(NULL);
		}
		else
		{
			// keep track of instructions for which we have not yet
			// resolved the ICFS
			unresolvedICFS[newinsn] = icfs_id;
		}

		dbintr->moveToNextRow();
	}

	for(std::map<db_id_t,Instruction_t*>::const_iterator i=idMap.begin(); i!=idMap.end(); ++i)
	{
		Instruction_t *instr=(*i).second;
		db_id_t fallthroughid=fallthroughs[instr->getBaseID()];
		if(idMap[fallthroughid])	
			instr->setFallthrough(idMap[fallthroughid]);
		db_id_t targetid=targets[instr->getBaseID()];
		if(idMap[targetid])	
			instr->setTarget(idMap[targetid]);
	}


	return idMap;
}

void FileIR_t::ReadRelocsFromDB
	(
		std::map<db_id_t,BaseObj_t*> 	&objMap
	)
{
	std::string q= "select * from " + fileptr->relocs_table_name + " ; ";
	dbintr->issueQuery(q);

	while(!dbintr->isDone())
	{
                db_id_t reloc_id=atoi(dbintr->getResultColumn("reloc_id").c_str());
                int reloc_offset=atoi(dbintr->getResultColumn("reloc_offset").c_str());
                std::string reloc_type=(dbintr->getResultColumn("reloc_type"));
                db_id_t instruction_id=atoi(dbintr->getResultColumn("instruction_id").c_str());

		// handle later?
                //db_id_t doipid=atoi(dbintr->getResultColumn("doip_id").c_str());
                db_id_t wrt_id=atoi(dbintr->getResultColumn("wrt_id").c_str());
                uint32_t addend=atoi(dbintr->getResultColumn("addend").c_str());


		BaseObj_t* wrt_obj=objMap[wrt_id];	 // might be null.
		Relocation_t *reloc=new Relocation_t(reloc_id,reloc_offset,reloc_type,wrt_obj,addend);

		assert(objMap[instruction_id]!=NULL);

		objMap[instruction_id]->GetRelocations().insert(reloc);
		relocs.insert(reloc);

		dbintr->moveToNextRow();
	}

}


void FileIR_t::writeToDB(ostream *verbose_logging)
{
//     	const auto WriteIRDB_start = clock();

	const auto pqIntr=dynamic_cast<pqxxDB_t*>(dbintr);
	assert(pqIntr);

	//Resolve (assemble) any instructions in the registry.
	assembleRegistry();

	/* assign each item a unique ID */
	setBaseIDS();

	CleanupICFS(verbose_logging);

	db_id_t j=-1;

	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->instruction_table_name 	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->icfs_table_name 		+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->icfs_map_table_name 	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->function_table_name    	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->address_table_name     	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->relocs_table_name     	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->types_table_name     	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->scoop_table_name     	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->scoop_table_name+"_part2"+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->ehpgm_table_name     	+ string(" cascade;"));
	dbintr->issueQuery(string("TRUNCATE TABLE ")+ fileptr->ehcss_table_name     	+ string(" cascade;"));

	/* and now that everything has an ID, let's write to the DB */

// write out the types
	string q=string("");
	for(auto t=types.begin(); t!=types.end(); ++t)
	{
		auto real_t=dynamic_cast<Type_t*>(*t);
		assert(real_t);
		q+=real_t->WriteToDB(fileptr,j); // @TODO: wtf is j?
		if(q.size()>1024*1024)
		{
			dbintr->issueQuery(q);
			q=string("");
		}
	}
	dbintr->issueQuery(q);
	

// write out functions 
	auto withHeader=true;
	q=string("");
	for(auto f=funcs.begin(); f!=funcs.end(); ++f)
	{
		auto real_f=dynamic_cast<Function_t*>(*f);
		assert(real_f);
		q+=real_f->WriteToDB(fileptr,j);
		if(q.size()>1024*1024)
		{
			dbintr->issueQuery(q);
			q=string("");
		}
	}
	dbintr->issueQuery(q);


// write out addresses
	pqxx::tablewriter W_addrs(pqIntr->getTransaction(),fileptr->address_table_name);
	for(const auto &a : addrs)
	{
		auto real_a=dynamic_cast<AddressID_t*>(a);
		assert(real_a);
		W_addrs << real_a->WriteToDB(fileptr,j,withHeader);
	}
	W_addrs.complete();

// write out instructions

	pqxx::tablewriter W(pqIntr->getTransaction(),fileptr->instruction_table_name);
	for(auto i=insns.begin(); i!=insns.end(); ++i)
	{	
		auto insnp=dynamic_cast<Instruction_t*>(*i);
		//DISASM disasm;
		//Disassemble(insnp,disasm);
		const auto p_disasm=DecodedInstruction_t::factory(insnp);
		const auto& disasm=*p_disasm;

		if(insnp->getOriginalAddressID() == NOT_IN_DATABASE)
		{

			// if(insnp->getFallthrough()==NULL && disasm.Instruction.BranchType!=RetType && disasm.Instruction.BranchType!=JmpType )
			if(insnp->getFallthrough()==NULL && !disasm.isReturn() && !disasm.isUnconditionalBranch())
			{
				// instructions that fall through are required to either specify a fallthrough that's
				// in the IRDB, or have an associated "old" instruction.  
				// without these bits of information, the new instruction can't possibly execute correctly.
				// and we won't have the information necessary to emit spri.

				*verbose_logging << "NULL fallthrough: offending instruction:" << ((Instruction_t*)insnp)->getDisassembly() << " comment: " << ((Instruction_t*)insnp)->getComment() << endl;
				assert(0);
				abort();
			}
			//if(insnp->getTarget()==NULL && disasm.Instruction.BranchType!=0 && 
			//	disasm.Instruction.BranchType!=RetType &&
			//	// not an indirect branch
			//	((disasm.Instruction.BranchType!=JmpType && disasm.Instruction.BranchType!=CallType) ||
			//	 disasm.Argument1.ArgType&CONSTANT_TYPE))

			if(insnp->getTarget()==NULL && disasm.isBranch() && !disasm.isReturn() &&
				// not an indirect branch
				( (!disasm.isUnconditionalBranch() && !disasm.isCall()) || disasm.getOperand(0)->isConstant())
			  )
			{
				// direct branches are required to either specify a target that's
				// in the IRDB, or have an associated "old" instruction.  
				// without these bits of information, the new instruction can't possibly execute correctly.
				// and we won't have the information necessary to emit spri.
				*verbose_logging << "Call must have a target; offending instruction:" << ((Instruction_t*)insnp)->getDisassembly() << " comment: " << ((Instruction_t*)insnp)->getComment() << endl;
				assert(0);
				abort();
			}
		}

		const auto &insn_values=insnp->WriteToDB(fileptr,j);
		W << insn_values;

	}
	W.complete();


// icfs 
	for (auto it = GetAllICFS().begin(); it != GetAllICFS().end(); ++it)
	{
		ICFS_t* icfs = dynamic_cast<ICFS_t*>(*it);
		assert(icfs);
		string q = icfs->WriteToDB(fileptr);
		dbintr->issueQuery(q);
	}

// scoops
	for(auto it=scoops.begin(); it!=scoops.end(); ++it)
	{
		DataScoop_t* scoop = dynamic_cast<DataScoop_t*>(*it);
		assert(scoop);
		string q = scoop->WriteToDB(fileptr,j);
		dbintr->issueQuery(q);
	}

// ehpgms 
	pqxx::tablewriter W_eh(pqIntr->getTransaction(),fileptr->ehpgm_table_name);
	for(const auto& i : eh_pgms)
	{
		W_eh << dynamic_cast<EhProgram_t*>(i)->WriteToDB(fileptr);
	}
	W_eh.complete();

// eh css
	for(const auto& i : eh_css)
	{
		string q = dynamic_cast<EhCallSite_t*>(i)->WriteToDB(fileptr);
		dbintr->issueQuery(q);
	}


// all relocs
	pqxx::tablewriter W_reloc(pqIntr->getTransaction(),fileptr->relocs_table_name);

// eh css relocs
	for(const auto& i : eh_css)
	{
		const auto &relocs=i->getRelocations();
		for(auto& reloc : relocs)
			W_reloc << dynamic_cast<Relocation_t*>(reloc)->WriteToDB(fileptr,dynamic_cast<BaseObj_t*>(i));
	}

// eh pgms relocs
	for(const auto& i : eh_pgms)
	{
		const auto &relocs=i->getRelocations();
		for(auto& reloc : relocs)
			W_reloc << dynamic_cast<Relocation_t*>(reloc)->WriteToDB(fileptr,dynamic_cast<BaseObj_t*>(i));
	}
// scoops relocs
	for(const auto& i : scoops)
	{
		const auto &relocs=i->getRelocations();
		for(auto& reloc : relocs)
			W_reloc << dynamic_cast<Relocation_t*>(reloc)->WriteToDB(fileptr,dynamic_cast<BaseObj_t*>(i));
	}
// write out instruction's relocs
	for(const auto& i : insns)
	{
		const auto &relocs=i->getRelocations();
		for(auto& reloc : relocs)
			W_reloc << dynamic_cast<Relocation_t*>(reloc)->WriteToDB(fileptr,dynamic_cast<BaseObj_t*>(i));
	}
	W_reloc.complete();

}


db_id_t FileIR_t::getMaxBaseID() const
{
#define MAX(a,b) (((a)>(b)) ? (a) : (b))

	/* find the highest database ID */
	db_id_t j=0;
	for(auto i=funcs.begin(); i!=funcs.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=addrs.begin(); i!=addrs.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=insns.begin(); i!=insns.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=relocs.begin(); i!=relocs.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=types.begin(); i!=types.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=scoops.begin(); i!=scoops.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=icfs_set.begin(); i!=icfs_set.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=eh_pgms.begin(); i!=eh_pgms.end(); ++i)
		j=MAX(j,(*i)->getBaseID());
	for(auto i=eh_css.begin(); i!=eh_css.end(); ++i)
		j=MAX(j,(*i)->getBaseID());

	return j+1;	 // easy to off-by-one this so we do it for a user just in case.
}

void FileIR_t::setBaseIDS()
{
	auto j=getMaxBaseID();
	/* increment past the max ID so we don't duplicate */
	j++;
	/* for anything that's not yet in the DB, assign an ID to it */
	for(auto i=funcs.begin(); i!=funcs.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=addrs.begin(); i!=addrs.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=insns.begin(); i!=insns.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=relocs.begin(); i!=relocs.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=types.begin(); i!=types.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=scoops.begin(); i!=scoops.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=icfs_set.begin(); i!=icfs_set.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=eh_pgms.begin(); i!=eh_pgms.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
	for(auto i=eh_css.begin(); i!=eh_css.end(); ++i)
		if((*i)->getBaseID()==NOT_IN_DATABASE)
			(*i)->setBaseID(j++);
}

const  IRDB_SDK::ArchitectureDescription_t* IRDB_SDK::FileIR_t::getArchitecture() 
{ 
		return libIRDB::FileIR_t::archdesc; 
}

uint32_t IRDB_SDK::FileIR_t::getArchitectureBitWidth()
{
	return libIRDB::FileIR_t::archdesc->getBitWidth();
}

void FileIR_t::setArchitecture(const int width, const ADMachineType_t mt) 
{
	if(archdesc==NULL)
		archdesc=new ArchitectureDescription_t;
	archdesc->setBitWidth(width); 
	archdesc->setMachineType(mt); 

	archdesc->setFileBase(0);	// maybe not rght for PE files?
}	

void FileIR_t::setArchitecture()
{

	/* the first 16 bytes of an ELF file define the magic number and ELF Class. */
    	// unsigned char e_ident[16];
	union 
	{
		Elf32_Ehdr ehdr32;
		Elf64_Ehdr ehdr64;
	} hdr_union;

	auto myinter=BaseObj_t::GetInterface();
	auto *mypqxxintr=dynamic_cast<pqxxDB_t*>(myinter);

	const auto elfoid=getFile()->getELFOID();
        pqxx::largeobjectaccess loa(mypqxxintr->getTransaction(), elfoid, std::ios::in);


        loa.cread((char*)&hdr_union, sizeof(hdr_union));

	const auto e_ident=hdr_union.ehdr32.e_ident;

	libIRDB::FileIR_t::archdesc=new ArchitectureDescription_t;

	auto is_elf=false;
	auto is_pe=false;

	if((e_ident[EI_MAG0]==ELFMAG0) && 
	   (e_ident[EI_MAG1]==ELFMAG1) && 
	   (e_ident[EI_MAG2]==ELFMAG2) && 
	   (e_ident[EI_MAG3]==ELFMAG3))
	{
		is_elf=true;
	}
	else if((e_ident[EI_MAG0]=='M') &&
	   (e_ident[EI_MAG1]=='Z'))
	{
		is_pe=true;
	}
	else
	{
		cerr << "File format magic number wrong:  is this an ELF, PE or CGC file? [" << e_ident[EI_MAG0] << " " << e_ident[EI_MAG1] << "]" << endl;
		exit(-1);
	}


	if (is_pe) 
	{

		// now we need to read the image base from the exe file

		//DOS .EXE header
		struct irdb_dos_header
		{
			uint16_t e_magic;                     // Magic number
			uint16_t e_cblp;                      // Bytes on last page of file
			uint16_t e_cp;                        // Pages in file
			uint16_t e_crlc;                      // Relocations
			uint16_t e_cparhdr;                   // Size of header in paragraphs
			uint16_t e_minalloc;                  // Minimum extra paragraphs needed
			uint16_t e_maxalloc;                  // Maximum extra paragraphs needed
			uint16_t e_ss;                        // Initial (relative) SS value
			uint16_t e_sp;                        // Initial SP value
			uint16_t e_csum;                      // Checksum
			uint16_t e_ip;                        // Initial IP value
			uint16_t e_cs;                        // Initial (relative) CS value
			uint16_t e_lfarlc;                    // File address of relocation table
			uint16_t e_ovno;                      // Overlay number
			uint16_t e_res[4];                    // Reserved words
			uint16_t e_oemid;                     // OEM identifier (for e_oeminfo)
			uint16_t e_oeminfo;                   // OEM information; e_oemid specific
			uint16_t e_res2[10];                  // Reserved words
			int32_t  e_lfanew;                    // File address of new exe header
		};

		struct irdb_image_pe_headers64
		{
			uint32_t Signature;
			uint16_t Machine;
			uint16_t NumberOfSections;

			uint32_t TimeDateStamp;
			uint32_t PointerToSymbolTable;

			uint32_t NumberOfSymbols;
			uint16_t SizeOfOptionalHeader;
			uint16_t Characteristics;

			uint16_t Magic;
			uint8_t  MajorLinkerVersion;
			uint8_t  MinorLinkerVersion;
			uint32_t SizeOfCode;

			uint32_t SizeOfInitializedData;
			uint32_t SizeOfUninitializedData;

			uint32_t AddressOfEntryPoint;
			uint32_t BaseOfCode;

			union
			{
				struct 
				{
					uint32_t BaseOfData;

					// ImageBase is a uint32_t for 32-bit code.
					uint32_t ImageBase;
				} pe32;
				struct 
				{
					// ImageBase is a uint64_t for 64-bit code and BaseOfData is elided.
					uint64_t ImageBase;
				} pe32plus;
			} mach_dep;

		};



		// declare and init a dos header.
                struct irdb_dos_header idh;
                memset(&idh,0,sizeof(idh));
                struct irdb_image_pe_headers64 pe_and_opt_headers;
                memset(&pe_and_opt_headers,0,sizeof(pe_and_opt_headers));

                loa.seek(0, ios::beg);
                loa.cread((char*)&idh, sizeof(idh));

                loa.seek(idh.e_lfanew, ios::beg);
                loa.cread((char*)&pe_and_opt_headers, sizeof(pe_and_opt_headers));

                assert(pe_and_opt_headers.Signature ==0x4550 /* "PE" means pe file */);

		auto image_base = uint64_t{0};
		if(pe_and_opt_headers.Magic == 0x20b)
		{
			// record image base
			image_base = pe_and_opt_headers.mach_dep.pe32plus.ImageBase;

			// set machine/file types
			libIRDB::FileIR_t::archdesc->setFileType(IRDB_SDK::adftPE);
			libIRDB::FileIR_t::archdesc->setBitWidth(64);
			libIRDB::FileIR_t::archdesc->setMachineType(IRDB_SDK::admtX86_64);
		}
		else if(pe_and_opt_headers.Magic == 0x10b)
		{
			// record image base
			image_base = pe_and_opt_headers.mach_dep.pe32.ImageBase;

			// set machine/file types
			libIRDB::FileIR_t::archdesc->setFileType(IRDB_SDK::adftPE);
			libIRDB::FileIR_t::archdesc->setBitWidth(32);
			libIRDB::FileIR_t::archdesc->setMachineType(IRDB_SDK::admtI386);
		}
		else
			throw invalid_argument("Cannot map Magic number ("+to_string(pe_and_opt_headers.Magic)+") into machine type");

                if(getenv("IRDB_VERBOSE"))
                        cout<<"Determined PE32/PE32+ file has image base: "<<hex<< image_base <<endl;

                archdesc->setFileBase(image_base);
	}
	else if(is_elf)
	{
		const auto elf_big_endian  = 
			e_ident[5] == 2 ? true  : 
			e_ident[5] == 1 ? false : 
			throw invalid_argument("Cannot detect endianness");

		const auto bits = 
			e_ident[4] == ELFCLASS32 ? 32 :
			e_ident[4] == ELFCLASS64 ? 64 :
			throw std::invalid_argument("Unknown ELF class");

		const auto e_type = elf_big_endian ? be16toh(hdr_union.ehdr32.e_type) : le16toh(hdr_union.ehdr32.e_type) ; 

		const auto ft = 
			e_type == ET_DYN  ? IRDB_SDK::adftELFSO  :
			e_type == ET_EXEC ? IRDB_SDK::adftELFEXE :
			throw std::invalid_argument("Unknown file type");

		const auto e_machine32 = elf_big_endian ? be16toh(hdr_union.ehdr32.e_machine) : le16toh(hdr_union.ehdr32.e_machine) ; 
		const auto e_machine64 = elf_big_endian ? be16toh(hdr_union.ehdr64.e_machine) : le16toh(hdr_union.ehdr64.e_machine) ;
		const auto mt = 
			e_machine32 == EM_MIPS    ? IRDB_SDK::admtMips32  : 
			e_machine32 == EM_386     ? IRDB_SDK::admtI386    : 
			e_machine32 == EM_ARM     ? IRDB_SDK::admtArm32   : 
			e_machine64 == EM_AARCH64 ? IRDB_SDK::admtAarch64 : 
			e_machine64 == EM_X86_64  ? IRDB_SDK::admtX86_64  : 
			throw std::invalid_argument("Arch not supported.");

		libIRDB::FileIR_t::archdesc->setFileType(ft);
		libIRDB::FileIR_t::archdesc->setMachineType(mt);
		libIRDB::FileIR_t::archdesc->setBitWidth(bits);
	}
}

ArchitectureDescription_t* FileIR_t::archdesc=NULL;

std::map<db_id_t, Type_t*> FileIR_t::ReadTypesFromDB (TypeSet_t& types)
{
	std::map<db_id_t, Type_t*> tMap;

	// 3 pass algorithm. Must be done in this exact order as:
	//      aggregrate type depend on basic types
	//      function type depends on both aggregate and basic types
	//
	// pass 1: retrieve all basic types
	// pass 2: retrieve all aggregate types
	// pass 3: retrieve all function types
	//

	// pass 1, get all the basic types first
//	std::string q= "select * from " + fileptr->types_table_name + " WHERE ref_type_id = -1 AND ref_type_id2 = -1 AND pos = -1 order by type; ";

	std::string q= "select * from " + fileptr->types_table_name + " WHERE ref_type_id = -1 order by type; ";

//	cout << "pass1: query: " << q;

	dbintr->issueQuery(q);

	while(!dbintr->isDone())
	{
//  type_id            integer,     
//  type               integer DEFAULT 0,    -- possible types (0: UNKNOWN)
//  name               text DEFAULT '',      -- string representation of the type
//  ref_type_id        integer DEFAULT -1,   -- for aggregate types and func
//  pos                integer DEFAULT -1,   -- for aggregate types, position in aggregate
//  ref_type_id2       integer DEFAULT -1,   -- for func types
//  doip_id            integer DEFAULT -1    -- the DOIP

		db_id_t tid=atoi(dbintr->getResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->getResultColumn("type").c_str());
		std::string name=dbintr->getResultColumn("name");
		BasicType_t *t = NULL;	

//		cout << "fileir::ReadFromDB(): pass1: " << name << endl;
		switch(type) 
		{
			case IRDB_SDK::itUnknown:
			case IRDB_SDK::itVoid:
			case IRDB_SDK::itNumeric:
			case IRDB_SDK::itVariadic:
			case IRDB_SDK::itInt:
			case IRDB_SDK::itChar:
			case IRDB_SDK::itFloat:
			case IRDB_SDK::itDouble:
				t = new BasicType_t(tid, type, name);	
				types.insert(t);
				tMap[tid] = t;
				break;

			case IRDB_SDK::itPointer:
				// do nothing for pointers
				break;

			default:
				cerr << "ReadTypesFromDB(): ERROR: pass 1 should only see basic types or pointer"  << endl;
				assert(0); 
				break;						
		}

		dbintr->moveToNextRow();
	} // end pass1

	char query[2048];

	// pass2 get pointers
	sprintf(query,"select * from %s WHERE type = %d;", fileptr->types_table_name.c_str(), IRDB_SDK::itPointer);
	dbintr->issueQuery(query);

	while(!dbintr->isDone())
	{
		db_id_t tid=atoi(dbintr->getResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->getResultColumn("type").c_str());
		std::string name=dbintr->getResultColumn("name");
		db_id_t ref1=atoi(dbintr->getResultColumn("ref_type_id").c_str());
//		cout << "fileir::ReadFromDB(): pass2 (pointers): " << name << endl;
		switch(type) 
		{
			case IRDB_SDK::itPointer:
			{
//						cout << "   pointer type: ref1: " << ref1 << endl;
				Type_t *referentType = NULL;
				if (ref1 >= 0) 
				{
					assert(tMap.count(ref1) > 0);
					referentType = tMap[ref1];
				}
				PointerType_t *ptr = new PointerType_t(tid, referentType, name);	
				types.insert(ptr);
				tMap[tid] = ptr;
			}
			break;

			default:
				cerr << "ReadTypesFromDB(): ERROR: pass3: unhandled type id = "  << type << endl;
				assert(0); // not yet handled
				break;						

		}

		dbintr->moveToNextRow();
	} // end pass3

	// pass3 get all aggregates
	sprintf(query,"select * from %s WHERE type = %d order by type_id, pos;", fileptr->types_table_name.c_str(), IRDB_SDK::itAggregate);
	dbintr->issueQuery(query);

	while(!dbintr->isDone())
	{
		db_id_t tid=atoi(dbintr->getResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->getResultColumn("type").c_str());
		std::string name=dbintr->getResultColumn("name");
		db_id_t ref1=atoi(dbintr->getResultColumn("ref_type_id").c_str());
		int pos=atoi(dbintr->getResultColumn("pos").c_str());
		AggregateType_t *agg = NULL;	
//		cout << "fileir::ReadFromDB(): pass3 (aggregates): " << name << endl;
		switch(type) 
		{
			case IRDB_SDK::itAggregate:
			{
				if (tMap.count(tid) == 0)  // new aggregate
				{	
//		cout << "fileir::ReadFromDB(): pass3: new aggregate type: typeid: " << tid << " name: " << name << endl;
					agg = new AggregateType_t(tid, name);	
					types.insert(agg);
					tMap[tid] = agg;
				}
				else
					agg = dynamic_cast<AggregateType_t*>(tMap[tid]);

				assert(agg);
				// ref1 has the id of a basic type, look it up
				Type_t *ref = tMap[ref1];
				assert(ref);
				agg->addAggregatedType(ref, pos);
			}
			break;

			default:
				cerr << "ReadTypesFromDB(): ERROR: pass2: unhandled type id = "  << type << endl;
				assert(0); // not yet handled
				break;						

		}

		dbintr->moveToNextRow();
	} // end pass3

	// pass4 get all functions
	sprintf(query,"select * from %s WHERE type = %d;", fileptr->types_table_name.c_str(), IRDB_SDK::itFunc);
	dbintr->issueQuery(query);

	while(!dbintr->isDone())
	{
		db_id_t tid=atoi(dbintr->getResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->getResultColumn("type").c_str());
		std::string name=dbintr->getResultColumn("name");
		db_id_t ref1=atoi(dbintr->getResultColumn("ref_type_id").c_str());
		db_id_t ref2=atoi(dbintr->getResultColumn("ref_type_id2").c_str());
		FuncType_t *fn = NULL;	

		switch(type) 
		{
			case IRDB_SDK::itFunc:
			{
				assert(tMap.count(tid) == 0);

				fn = new FuncType_t(tid, name);	
				types.insert(fn);
				tMap[tid] = fn;
				assert(tMap[ref1]); // return type
				assert(tMap[ref2]); // argument type (which is an aggregate)
				fn->setReturnType(tMap[ref1]);
				AggregateType_t *args = dynamic_cast<AggregateType_t*>(tMap[ref2]);
				assert(args);
				fn->setArgumentsType(args);
				break;
			}
			default:
				cerr << "ReadTypesFromDB(): ERROR: pass4: unhandled type id = "  << type << endl;
				assert(0); // not yet handled
				break;						

		}

		dbintr->moveToNextRow();
	} // end pass4

	return tMap;
}

void FileIR_t::ReadAllICFSFromDB(std::map<db_id_t,Instruction_t*> &addr2instMap,
		std::map<Instruction_t*, db_id_t> &unresolvedICFS)
{
	std::map<db_id_t, ICFS_t*> icfsMap;

	// retrieve all sets
	std::string q= "select * from " + fileptr->icfs_table_name + " ; ";
	dbintr->issueQuery(q);

	while(!dbintr->isDone())
	{
		db_id_t icfs_id = atoi(dbintr->getResultColumn("icfs_id").c_str());
		string statusString=dbintr->getResultColumn("icfs_status"); 

		ICFS_t* icfs = new ICFS_t(icfs_id, statusString);		
		GetAllICFS().insert(icfs);

		icfsMap[icfs_id] = icfs;
		dbintr->moveToNextRow();
	}

	ICFSSet_t all_icfs = GetAllICFS();

	// for each set, populate its members
	for (auto it = all_icfs.begin(); it != all_icfs.end(); ++it)
	{
		char query2[2048];
		auto icfs = dynamic_cast<ICFS_t*>(*it);
		assert(icfs);
		int icfsid = icfs->getBaseID();
		sprintf(query2,"select * from %s WHERE icfs_id = %d;", fileptr->icfs_map_table_name.c_str(), icfsid);
		dbintr->issueQuery(query2);
		while(!dbintr->isDone())
		{
			db_id_t address_id = atoi(dbintr->getResultColumn("address_id").c_str());
			Instruction_t* instruction = addr2instMap[address_id];
			if (instruction) 
			{
				icfs->insert(instruction);
			}

			// @todo: handle cross-file addresses
			//        these are allowed by the DB schema but we don't yet handle them
			// if we encounter an unresolved address, we should mark the ICFS
			//      as unresolved

			dbintr->moveToNextRow();
		}					
	}

	// backpatch all unresolved instruction -> ICFS
	std::map<Instruction_t*, db_id_t>::iterator uit;
	for (auto uit = unresolvedICFS.begin(); uit != unresolvedICFS.end(); ++uit)
	{
		auto unresolved = dynamic_cast<Instruction_t*>(uit->first);
		db_id_t icfs_id = uit->second;

		assert(unresolved);

		ICFS_t *icfs = icfsMap[icfs_id];
		assert(icfs);

		unresolved->setIBTargets(icfs);
	}
}

void FileIR_t::GarbageCollectICFS(ostream* verbose_logging)
{
	auto used_icfs= ICFSSet_t();
	// get the IBTarget of each instruction into used_icfs
	transform(     ALLOF(insns), inserter(used_icfs, begin(used_icfs)), 
	               [](const IRDB_SDK::Instruction_t* insn) -> IRDB_SDK::ICFS_t* { return insn->getIBTargets(); } 
	         );
	// we likely inserted null into the set, which we just will remove as a special ase.
	used_icfs.erase(nullptr);

	// update the list to include only the used ones.
	icfs_set=used_icfs;
}

void FileIR_t::DedupICFS(ostream *verbose_logging)
{
	std::set<InstructionSet_t> unique_icfs;

	auto& all_icfs=this->GetAllICFS();

	// detect duplicate icfs
	ICFSSet_t duplicates;
	for(auto it=all_icfs.begin(); it!=all_icfs.end(); ++it)
	{
		auto p=dynamic_cast<ICFS_t*>(*it);
		assert(p);
		auto ret = unique_icfs.insert( *p );
		if (!ret.second) {
			duplicates.insert(p);
		}
	}

	if (duplicates.size() > 0)
	{
		*verbose_logging << "FileIR_t::DedupICFS(): WARNING: " << dec << duplicates.size() << " duplicate ICFS out of " << all_icfs.size() << " total ICFS";
		*verbose_logging << ". De-duplicating before committing to IRDB" << endl;
	}

	// remove duplicate icfs
	for(auto it=duplicates.begin(); it!=duplicates.end(); ++it)
	{
		auto icfs=*it;
		all_icfs.erase(icfs);
	}

	// build duplicate icfs map
	std::map<IRDB_SDK::ICFS_t*, IRDB_SDK::ICFS_t*> duplicate_map;
	for(auto it=duplicates.begin(); it!=duplicates.end(); ++it)
	{
		auto icfs = *it;
		for(auto it=all_icfs.begin(); it!=all_icfs.end(); ++it)
		{
			auto t = *it;

			assert(t);
			if (*icfs == *t)
			{
				duplicate_map[icfs] = t;
				*verbose_logging << "FileIR_t::DedupICFS(): remap: icfs id " << icfs->getBaseID() << " --> icsf id " << t->getBaseID() << endl;
				break;
			}
		}
	}

	// reassign ibtargets 
	for(auto it=this->GetInstructions().begin();
		it!=this->GetInstructions().end();
   		++it)
	{
		auto instr=*it;
		if(instr->getIBTargets() && duplicate_map[instr->getIBTargets()])
		{ 
			instr->setIBTargets(duplicate_map[instr->getIBTargets()]);
		}
	}
}

void FileIR_t::CleanupICFS(ostream *verbose_logging)
{
	GarbageCollectICFS(verbose_logging);
	DedupICFS(verbose_logging);
}

std::map<db_id_t,DataScoop_t*> FileIR_t::ReadScoopsFromDB
        (
                std::map<db_id_t,AddressID_t*> &addrMap,
                std::map<db_id_t,Type_t*> &typeMap
        )
{
/*
  scoop_id           SERIAL PRIMARY KEY,        -- key
  name               text DEFAULT '',           -- string representation of the type
  type_id            integer,                   -- the type of the data, as an index into the table table.
  start_address_id   integer,                   -- address id for start.
  end_address_id     integer,                   -- address id for end
  permissions        integer                    -- in umask format (bitmask for rwx)

 */

	std::map<db_id_t,DataScoop_t*> scoopMap;

	//std::map<db_id_t,string> bonus_contents;
	//
	// read part 2 of the scoops.
        //std::string q= "select * from " + fileptr->scoop_table_name + "_part2 ; ";
        //dbintr->issueQuery(q);
        //while(!dbintr->isDone())
	//{
        //        db_id_t sid=atoi(dbintr->getResultColumn("scoop_id").c_str());
	//	bonus_contents[sid]=dbintr->getResultColumn("data");
	//	dbintr->moveToNextRow();
	//}



	// read part 1 of the scoops, and merge in the part 2s
	// scoop_id           SERIAL PRIMARY KEY,        -- key
	// name               text DEFAULT '',           -- string representation of the type
	// type_id            integer,                   -- the type of the data, as an index into the table table.
	// start_address_id   integer,                   -- address id for start.
	// end_address_id     integer,                   -- address id for end
	// permissions        integer,                   -- in umask format (bitmask for rwx)
	// relro              bit,                       -- is this scoop a relro scoop (i.e., is r/w until relocs are done).
	// data               bytea                      -- the actual bytes of the scoop

        string q= "select scoop_id,name,type_id,start_address_id,end_address_id,permissions,relro from " + fileptr->scoop_table_name + " ; ";
        dbintr->issueQuery(q);
        while(!dbintr->isDone())
        {

                db_id_t sid=atoi(dbintr->getResultColumn("scoop_id").c_str());
                std::string name=dbintr->getResultColumn("name");
                db_id_t type_id=atoi(dbintr->getResultColumn("type_id").c_str());
		Type_t *type=typeMap[type_id];
                db_id_t start_id=atoi(dbintr->getResultColumn("start_address_id").c_str());
		AddressID_t* start_addr=addrMap[start_id];
                db_id_t end_id=atoi(dbintr->getResultColumn("end_address_id").c_str());
		AddressID_t* end_addr=addrMap[end_id];
                int permissions=atoi(dbintr->getResultColumn("permissions").c_str());
                bool is_relro=atoi(dbintr->getResultColumn("relro").c_str()) != 0 ;

		DataScoop_t* newscoop=new DataScoop_t(sid,name,start_addr,end_addr,type,permissions,is_relro,"");
		assert(newscoop);
		GetDataScoops().insert(newscoop);
		dbintr->moveToNextRow();

		scoopMap[sid]=newscoop;
	}

	for(auto it=getDataScoops().begin(); it!=getDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=dynamic_cast<DataScoop_t*>(*it);

        	q= "select length(data) from " + fileptr->scoop_table_name + " where scoop_id='"+to_string(scoop->getBaseID())+"'; ";
        	dbintr->issueQuery(q);
		if(!dbintr->isDone())
		{
			int data_len=atoi(dbintr->getResultColumn("length").c_str());
			for(int i=0;i<data_len;i+=SCOOP_CHUNK_SIZE)
			{
				string start_pos=to_string(i);
				string len_to_get=to_string(SCOOP_CHUNK_SIZE);
				string field="substr(data,"+start_pos+","+len_to_get+")";
				q= "select "+field+" from " + fileptr->scoop_table_name + " where scoop_id='"+to_string(scoop->getBaseID())+"'; ";
				dbintr->issueQuery(q);

				scoop->getContents()+=dbintr->getResultColumn("substr");

			}
		}


		// read part 2 from db
        	q= "select length(data) from " + fileptr->scoop_table_name + "_part2 where scoop_id='"+to_string(scoop->getBaseID())+"'; ";
        	dbintr->issueQuery(q);
		if(!dbintr->isDone())
		{
			int part2_len=atoi(dbintr->getResultColumn("length").c_str());
			for(int i=0;i<part2_len; i+=SCOOP_CHUNK_SIZE)
			{
				string start_pos=to_string(i);
				string len_to_get=to_string(SCOOP_CHUNK_SIZE);
				string field="substr(data,"+start_pos+","+len_to_get+")";
				q= "select "+field+" from " + fileptr->scoop_table_name + "_part2 where scoop_id='"+to_string(scoop->getBaseID())+"'; ";
				dbintr->issueQuery(q);

				scoop->getContents()+=dbintr->getResultColumn("substr");

			}
		}
	}
	for(auto s : getDataScoops())
	{
		auto scoop=dynamic_cast<DataScoop_t*>(s);
		assert(scoop->getContents().size() == scoop->getSize());
	}


	return scoopMap;
}


// Lookup a scoop by address
IRDB_SDK::DataScoop_t* FileIR_t::findScoop(const IRDB_SDK::VirtualOffset_t &addr) const
{
/*
	for(auto it=scoops.begin(); it!=scoops.end(); ++it)
	{
		auto s=dynamic_cast<DataScoop_t*>(*it);
		// we're doing <= in both comparisons here intentionally.
		// scoop addresses are the start/end address are inclusive
		// so that start+size-1 == end.
		if( s->getStart()->getVirtualOffset() <= addr && addr <= s->getEnd()->getVirtualOffset() )
			return *it;
	}
*/
	const auto found = find_if(ALLOF(scoops), [addr](IRDB_SDK::DataScoop_t* s) {
		return s->getStart()->getVirtualOffset() <= addr && addr <= s->getEnd()->getVirtualOffset();
	});
	return found==scoops.end() ? NULL : *found;
}


void FileIR_t::splitScoop(
		IRDB_SDK::DataScoop_t *p_tosplit, 
		const IRDB_SDK::VirtualOffset_t &addr, 
		size_t size, 
		IRDB_SDK::DataScoop_t* &p_before,
		IRDB_SDK::DataScoop_t* &p_containing, 
		IRDB_SDK::DataScoop_t* &p_after,
		IRDB_SDK::DatabaseID_t *max_id_ptr	 /* in and out param */
	)
{
	auto tosplit    = dynamic_cast<DataScoop_t*>(p_tosplit);
	auto before     = dynamic_cast<DataScoop_t*>(p_before);
	auto containing = dynamic_cast<DataScoop_t*>(p_containing);
	auto after      = dynamic_cast<DataScoop_t*>(p_after);
 
	// init output params.
	before=containing=after=NULL;


	if(tosplit->getStart()->getVirtualOffset() == addr &&
		tosplit->getEnd()->getVirtualOffset() == addr+size-1)
	{
		// no split necessary
		p_containing=tosplit;
		return;
	}

	auto calcd_max_id=db_id_t(0);
	if(max_id_ptr==NULL)
		calcd_max_id = getMaxBaseID();

	auto &max_id = max_id_ptr == NULL ? calcd_max_id : *max_id_ptr ;
	
	if(max_id==0)
		max_id=calcd_max_id;

	const auto multiple=(unsigned)1000;

	// round to nearest multiple
	const auto rounded_max = ((max_id + (multiple/2)) / multiple) * multiple;

	max_id=rounded_max + multiple; // and skip forward so we're passed the highest.

	const bool needs_before = addr!=tosplit->getStart()->getVirtualOffset();
	const bool needs_after = addr+size-1 != tosplit->getEnd()->getVirtualOffset();


	if(needs_before)
	{
		// setup before
		// const AddressID_t* before_start=NULL;
	
		const auto before_start=new AddressID_t;
		before_start->setBaseID(max_id++);
		before_start->setFileID(tosplit->getStart()->getFileID());
		before_start->setVirtualOffset(tosplit->getStart()->getVirtualOffset());

		// const AddressID_t* before_end=new AddressID_t;
		const auto before_end=new AddressID_t;
		before_end->setBaseID(max_id++);
		before_end->setFileID(tosplit->getStart()->getFileID());
		before_end->setVirtualOffset(addr-1);

		before=new DataScoop_t;
		before->setBaseID(max_id++);
		before->setName(tosplit->getName()+"3");
		before->setStart(before_start);
		before->setEnd(before_end);
		before->setRawPerms(tosplit->getRawPerms());
		before->getContents().resize(before_end->getVirtualOffset() - before_start->getVirtualOffset()+1);

		// copy bytes
		for(virtual_offset_t i=before_start->getVirtualOffset() ; i <= before_end->getVirtualOffset(); i++)
			before->getContents()[i-before_start->getVirtualOffset()] = tosplit->getContents()[i-tosplit->getStart()->getVirtualOffset()];
	

		GetAddresses().insert(before_start);
		GetAddresses().insert(before_end);
		GetDataScoops().insert(before);
	}

	// setup containing
	// AddressID_t* containing_start=new AddressID_t;
	const auto containing_start=new AddressID_t;
	containing_start->setBaseID(max_id++);
	containing_start->setFileID(tosplit->getStart()->getFileID());
	containing_start->setVirtualOffset(addr);

	//AddressID_t* containing_end=new AddressID_t;
	const auto containing_end=new AddressID_t;
	containing_end->setBaseID(max_id++);
	containing_end->setFileID(tosplit->getStart()->getFileID());
	containing_end->setVirtualOffset(addr+size-1);

	containing=new DataScoop_t;
	containing->setBaseID(max_id++);
	containing->setName(tosplit->getName()+"3");
	containing->setStart(containing_start);
	containing->setEnd(containing_end);
	containing->setRawPerms(tosplit->getRawPerms());
	containing->getContents().resize(containing_end->getVirtualOffset() - containing_start->getVirtualOffset()+1);
	// copy bytes
	for(virtual_offset_t i=containing_start->getVirtualOffset() ; i <= containing_end->getVirtualOffset(); i++)
		containing->getContents()[i-containing_start->getVirtualOffset()] = tosplit->getContents()[i-tosplit->getStart()->getVirtualOffset()];

	GetAddresses().insert(containing_start);
	GetAddresses().insert(containing_end);
	GetDataScoops().insert(containing);
		

	if(needs_after)
	{
		// setup after
		// AddressID_t* after_start=new AddressID_t;
		const auto after_start=new AddressID_t;
		after_start->setBaseID(max_id++);
		after_start->setFileID(tosplit->getStart()->getFileID());
		after_start->setVirtualOffset(addr+size);

		// AddressID_t* after_end=new AddressID_t;
		const auto after_end=new AddressID_t;
		after_end->setBaseID(max_id++);
		after_end->setFileID(tosplit->getStart()->getFileID());
		after_end->setVirtualOffset(tosplit->getEnd()->getVirtualOffset());

		after=new DataScoop_t;
		after->setBaseID(max_id++);
		after->setName(tosplit->getName()+"3");
		after->setStart(after_start);
		after->setEnd(after_end);
		after->setRawPerms(tosplit->getRawPerms());
		after->getContents().resize(after_end->getVirtualOffset() - after_start->getVirtualOffset()+1);
		// copy bytes
		for(virtual_offset_t i=after_start->getVirtualOffset() ; i <= after_end->getVirtualOffset(); i++)
			after->getContents()[i-after_start->getVirtualOffset()] = tosplit->getContents()[i-tosplit->getStart()->getVirtualOffset()];

		GetAddresses().insert(after_start);
		GetAddresses().insert(after_end);
		GetDataScoops().insert(after);
	}


	// since tosplit is going away, we need to move every relocation from the 
	// tosplit scoop into one of the others.  Iterate through each and 
	// adjust the reloc's offset accordingly before inserting into the correct scoop's reloc set.
	while(!tosplit->GetRelocations().empty())
	{
		auto reloc=dynamic_cast<Relocation_t*>(*(tosplit->getRelocations().begin()));
		tosplit->GetRelocations().erase(tosplit->getRelocations().begin());

		virtual_offset_t reloc_start=reloc->getOffset()+tosplit->getStart()->getVirtualOffset();

		if(reloc_start < containing->getStart()->getVirtualOffset() )
		{
			before->GetRelocations().insert(reloc);
		}
		else if(reloc_start > containing->getEnd()->getVirtualOffset() )
		{
			reloc->setOffset(reloc_start-after->getStart()->getVirtualOffset());
			after->GetRelocations().insert(reloc);
		}
		else
		{
			reloc->setOffset(reloc_start-containing->getStart()->getVirtualOffset());
			containing->GetRelocations().insert(reloc);
		}
	
	}

	/* look at each relocation in the IR */
	for(auto & r : GetRelocations())
	{
		if(r->getWRT()==tosplit)
		{
			const auto &addend=r->getAddend();
			const auto containing_start_offset=(containing -> getStart()->getVirtualOffset() - 
				tosplit->getStart()->getVirtualOffset());
			const auto containing_end_offset=containing_start_offset+containing->getSize();
			if(needs_before && addend<before->getSize())
			{
				r->setWRT(before);
			}
			else if( addend < containing_end_offset)
			{
				r->setWRT(containing);
				r->setAddend(addend-containing_start_offset);
			}
			else 		
			{
				assert(needs_after);
				const auto after_start_offset=(after -> getStart()->getVirtualOffset() - 
					tosplit->getStart()->getVirtualOffset());
				r->setWRT(after);
				r->setAddend(addend-after_start_offset);
			}
		}
	}
	

	GetAddresses().erase(tosplit->getStart());
	GetAddresses().erase(tosplit->getEnd());
	GetDataScoops().erase(tosplit);

	delete tosplit->getStart();
	delete tosplit->getEnd();
	delete tosplit;

	// set output parameters before returning.
	p_before    = before;
	p_containing= containing;
	p_after     = after;

	return;
}


IRDB_SDK::EhCallSite_t* FileIR_t::addEhCallSite(IRDB_SDK::Instruction_t* for_insn, const uint64_t enc, IRDB_SDK::Instruction_t* lp) 
{
	auto new_ehcs = new libIRDB::EhCallSite_t(BaseObj_t::NOT_IN_DATABASE, enc, lp);
	GetAllEhCallSites().insert(new_ehcs);
	if(for_insn)
        	for_insn->setEhCallSite(new_ehcs);
	return new_ehcs;

}

IRDB_SDK::Relocation_t* FileIR_t::addNewRelocation(
	IRDB_SDK::BaseObj_t* p_from_obj,
	int32_t p_offset,
	string p_type,
	IRDB_SDK::BaseObj_t* p_wrt_obj,
	int32_t p_addend)
{
	const auto from_obj=dynamic_cast<libIRDB::BaseObj_t*>(p_from_obj);
	auto newreloc=new libIRDB::Relocation_t(BaseObj_t::NOT_IN_DATABASE, p_offset, p_type, p_wrt_obj, p_addend);
	GetRelocations().insert(newreloc);
	if(from_obj)
		from_obj->GetRelocations().insert(newreloc);
	
	return newreloc;
}

EhProgram_t* FileIR_t::addEhProgram(
	IRDB_SDK::Instruction_t* insn,
	const uint64_t caf,
	const int64_t daf,
	const uint8_t rr,
	const uint8_t p_ptrsize,
	const EhProgramListing_t& p_cie_program,
	const EhProgramListing_t& p_fde_program)
{
	auto newehpgm=new EhProgram_t(BaseObj_t::NOT_IN_DATABASE, caf,daf,rr,p_ptrsize, p_cie_program, p_fde_program);
	assert(newehpgm);
	GetAllEhPrograms().insert(newehpgm);
	if(insn)
		insn->setEhProgram(newehpgm);
	return newehpgm;
}



void FileIR_t::removeScoop(IRDB_SDK::DataScoop_t* s)
{
	auto remove_reloc=[&](IRDB_SDK::Relocation_t* r) -> void
        {
                GetRelocations().erase(r);
                delete r;
        };

        auto remove_address=[&](IRDB_SDK::AddressID_t* a) -> void
        {
                GetAddresses().erase(a);
                for(auto &r : a->getRelocations()) remove_reloc(r);
                for(auto &r : getRelocations()) assert(r->getWRT() != a);
                delete a;
        };

        auto remove_scoop=[&] (IRDB_SDK::DataScoop_t* s) -> void
        {
                if(s==NULL)
                        return;
                GetDataScoops().erase(s);
                remove_address(s->getStart());
                remove_address(s->getEnd());
                for(auto &r : s->getRelocations()) remove_reloc(r);
                for(auto &r : GetRelocations()) assert(r->getWRT() != s);
                delete s;
        };

	remove_scoop(s);
}

IRDB_SDK::AddressID_t* FileIR_t::addNewAddress(const IRDB_SDK::DatabaseID_t& myfileID, const IRDB_SDK::VirtualOffset_t& voff) 
{
	auto newaddr = new libIRDB::AddressID_t(BaseObj_t::NOT_IN_DATABASE, myfileID, voff);
	GetAddresses().insert(newaddr);
	return newaddr;
}


IRDB_SDK::ICFS_t* FileIR_t::addNewICFS(
	IRDB_SDK::Instruction_t* insn,
	const IRDB_SDK::InstructionSet_t& targets,
	const IRDB_SDK::ICFSAnalysisStatus_t& status)
{
	auto newicfs=new libIRDB::ICFS_t(BaseObj_t::NOT_IN_DATABASE, status);
	newicfs->setTargets(targets);
	if(insn)
		insn->setIBTargets(newicfs);
	GetAllICFS().insert(newicfs);
	return newicfs;
}

IRDB_SDK::Instruction_t* FileIR_t::addNewInstruction(
	IRDB_SDK::AddressID_t* addr,
	IRDB_SDK::Function_t* func,
	const string& bits,
	const string& comment,
	IRDB_SDK::AddressID_t* indTarg)
{
	auto irdb_func    = dynamic_cast<libIRDB::Function_t* >(func);
	auto irdb_addr    = dynamic_cast<libIRDB::AddressID_t*>(addr);
	auto irdb_indTarg = dynamic_cast<libIRDB::AddressID_t*>(indTarg);

	if(irdb_addr==nullptr)
	{
		irdb_addr=dynamic_cast<libIRDB::AddressID_t*>(addNewAddress(getFile()->getBaseID(), 0));
	}

	auto newinsn=new libIRDB::Instruction_t(BaseObj_t::NOT_IN_DATABASE, irdb_addr, irdb_func, BaseObj_t::NOT_IN_DATABASE, bits, "", comment, irdb_indTarg, BaseObj_t::NOT_IN_DATABASE);
	
	GetInstructions().insert(newinsn);

	if(irdb_func)
		irdb_func->GetInstructions().insert(newinsn);
	return newinsn;
}

IRDB_SDK::DataScoop_t* FileIR_t::addNewDataScoop(
	const string& p_name,
	IRDB_SDK::AddressID_t* p_start,
	IRDB_SDK::AddressID_t* p_end,
	IRDB_SDK::Type_t* p_type,
	uint8_t p_permissions,
	bool p_is_relro,
	const string& p_contents,
	IRDB_SDK::DatabaseID_t id)
{
	auto irdb_start    = dynamic_cast<libIRDB::AddressID_t*>(p_start);
	auto irdb_end      = dynamic_cast<libIRDB::AddressID_t*>(p_end);
	auto irdb_type     = dynamic_cast<libIRDB::Type_t*>     (p_type);

	auto newscoop=new libIRDB::DataScoop_t(id, p_name,irdb_start,irdb_end,irdb_type,p_permissions, p_is_relro, p_contents);
	GetDataScoops().insert(newscoop);
	return newscoop;
}

IRDB_SDK::EhProgram_t* FileIR_t::copyEhProgram(const IRDB_SDK::EhProgram_t& orig)
{
	const auto ehpgm=dynamic_cast<const libIRDB::EhProgram_t*>(&orig);
	assert(ehpgm);
	auto new_eh_pgm=new libIRDB::EhProgram_t(*ehpgm);
        GetAllEhPrograms().insert(new_eh_pgm);
	return new_eh_pgm;
}



void FileIR_t::removeInstruction(IRDB_SDK::Instruction_t* toRemove)
{
	auto func = toRemove->getFunction();
	if(func)
	{
		auto func_insns = func->getInstructions();
		func_insns.erase(toRemove);
		func->setInstructions(func_insns);
	}
	insns.erase(toRemove);

}
