
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
#include <cstdlib>
#include <map>
#include <fstream>
#include <elf.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <iomanip>
#include <bea_deprecated.hpp>



using namespace libIRDB;
using namespace std;


#define SCOOP_CHUNK_SIZE (10*1024*1024)  /* 10 mb  */


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
		func->SetEntryPoint(insnMap.at(func_entry_id));
//		cout<<"Function named "<<func->GetName()<< " getting entry point set to "<<insnMap[func_entry_id]->GetComment()<<"."<<endl;
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
		ehcs->SetLandingPad(insn);
	}
}

static virtual_offset_t strtovo(std::string s)
{
        return strtoint<virtual_offset_t>(s);
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
	{
		ReadFromDB();
		SetArchitecture();
	}

}

FileIR_t::~FileIR_t()
{
	for(std::set<Function_t*>::const_iterator i=funcs.begin(); i!=funcs.end(); ++i)
	{
		delete *i;
	}


	for(std::set<Instruction_t*>::const_iterator i=insns.begin(); i!=insns.end(); ++i)
	{
		delete *i;
	}

	for(std::set<AddressID_t*>::const_iterator i=addrs.begin(); i!=addrs.end(); ++i)
	{
		delete *i;
	}

	for(std::set<Relocation_t*>::const_iterator i=relocs.begin(); i!=relocs.end(); ++i)
	{
		delete *i;
	}

	for(std::set<Type_t*>::const_iterator i=types.begin(); i!=types.end(); ++i)
	{
		delete *i;
	}

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


void  FileIR_t::ChangeRegistryKey(Instruction_t *orig, Instruction_t *updated)
{
	if(assembly_registry.find(orig) != assembly_registry.end())
	{
		assembly_registry[updated] = assembly_registry[orig];

		assembly_registry.erase(orig);
	}
}

void FileIR_t::AssembleRegistry()
{
	if(assembly_registry.size() == 0)
		return;

	string assemblyFile = "tmp.asm";
	string binaryOutputFile = "tmp.bin";

	string command = "rm -f " + assemblyFile + " " + binaryOutputFile;
	int rt = system(command.c_str());
	
	int actual_exit = -1;
	int actual_signal = -1;

	if (WIFEXITED(rt)) actual_exit = WEXITSTATUS(rt);
    else actual_signal = WTERMSIG(rt);

	assert(actual_exit == 0);
	
	ofstream asmFile;
	asmFile.open(assemblyFile.c_str());
	if(!asmFile.is_open())
		assert(false);

	asmFile<<"BITS "<<std::dec<<GetArchitectureBitWidth()<<endl; 

	for(registry_type::iterator it = assembly_registry.begin();
		it != assembly_registry.end();
		it++
		)
	{
		asmFile<<it->second<<endl;
	}
	asmFile.close();

	command = string("nasm ") + assemblyFile + string(" -o ") + binaryOutputFile;
	rt = system(command.c_str());

	actual_exit = -1;
	actual_signal = -1;

    	if (WIFEXITED(rt)) 
		actual_exit = WEXITSTATUS(rt);
    	else 
		actual_signal = WTERMSIG(rt);

	assert(actual_exit == 0);
	
	
	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));
	disasm.Archi=GetArchitectureBitWidth();

	ifstream binreader;
	unsigned int filesize;
	binreader.open(binaryOutputFile.c_str(),ifstream::in|ifstream::binary);

	assert(binreader.is_open());

	binreader.seekg(0,ios::end);
	filesize = binreader.tellg();
	binreader.seekg(0,ios::beg);

	unsigned char *binary_stream = new unsigned char[filesize];

	binreader.read((char*)binary_stream,filesize);
	binreader.close();

	unsigned int instr_index = 0;
	//for(unsigned int index=0; index < filesize; instr_index++)
	unsigned int index = 0;
	registry_type::iterator reg_val =  assembly_registry.begin();

	while(index < filesize)
	{
		//the number of registered instructions should not be exceeded
		assert(reg_val != assembly_registry.end());
		Instruction_t *instr = reg_val->first;


		disasm.EIP =  (UIntPtr)&binary_stream[index];
		int instr_len = Disasm(&disasm);
		string rawBits;
		rawBits.resize(instr_len);
		for(int i=0;i<instr_len;i++,index++)
		{
			rawBits[i] = binary_stream[index];
		}

		instr->SetDataBits(rawBits);
//		cerr << "doing instruction:" << ((Instruction_t*)instr)->getDisassembly() << " comment: " << ((Instruction_t*)instr)->GetComment() << endl;
		reg_val++;
	}

	assert(reg_val == assembly_registry.end());

	delete [] binary_stream;
	assembly_registry.clear();

}

void FileIR_t::RegisterAssembly(Instruction_t *instr, string assembly)
{
	assembly_registry[instr] = assembly;
}

void FileIR_t::UnregisterAssembly(Instruction_t *instr)
{
	assembly_registry.erase(instr);
}

std::string FileIR_t::LookupAssembly(Instruction_t *instr)
{
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

	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
// function_id | file_id | name | stack_frame_size | out_args_region_size | use_frame_pointer | is_safe | doip_id

		db_id_t fid=atoi(dbintr->GetResultColumn("function_id").c_str());
		db_id_t entry_point_id=atoi(dbintr->GetResultColumn("entry_point_id").c_str());
		std::string name=dbintr->GetResultColumn("name");
		int sfsize=atoi(dbintr->GetResultColumn("stack_frame_size").c_str());
		int oasize=atoi(dbintr->GetResultColumn("out_args_region_size").c_str());
		db_id_t function_type_id=atoi(dbintr->GetResultColumn("type_id").c_str());
// postgresql encoding of boolean can be 'true', '1', 'T', 'y'
		bool useFP=false;
		bool isSafe=false;
		string useFPString=dbintr->GetResultColumn("use_frame_pointer"); 
		string isSafeString=dbintr->GetResultColumn("is_safe"); 
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

		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		FuncType_t* fnType = NULL;
		if (typesMap.count(function_type_id) > 0)
			fnType = dynamic_cast<FuncType_t*>(typesMap[function_type_id]);
		Function_t *newfunc=new Function_t(fid,name,sfsize,oasize,useFP,isSafe,fnType, NULL); 
		entry_points[newfunc]=entry_point_id;
		
//std::cout<<"Found function "<<name<<"."<<std::endl;

		idMap[fid]=newfunc;

		funcs.insert(newfunc);

		dbintr->MoveToNextRow();
	}

	return idMap;
}


std::map<db_id_t,EhCallSite_t*> FileIR_t::ReadEhCallSitesFromDB
	(
		map<EhCallSite_t*,db_id_t> &unresolvedEhCssLandingPads // output arg.
	)
{
	auto ehcsMap=std::map<db_id_t,EhCallSite_t*>();

	std::string q= "select * from " + fileptr->GetEhCallSiteTableName() + " ; ";

	for(dbintr->IssueQuery(q); !dbintr->IsDone(); dbintr->MoveToNextRow())
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


		const auto eh_cs_id=atoi(dbintr->GetResultColumn("ehcs_id").c_str());
		const auto tt_encoding=atoi(dbintr->GetResultColumn("tt_encoding").c_str());
		const auto &ttov=string(dbintr->GetResultColumn("ttov").c_str());
		const auto lp_insn_id=atoi(dbintr->GetResultColumn("lp_insn_id").c_str());

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
	dbintr->IssueQuery(q);

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

	while(!dbintr->IsDone())
	{
		/* 
		 * eh_pgm_id       integer,
		 * caf             integer,
		 * daf             integer,
		 * ptrsize         integer,
		 * cie_program     text,
		 * fde_program     text
		 */


		const auto eh_pgm_id=atoi(dbintr->GetResultColumn("eh_pgm_id").c_str());
		const auto caf=atoi(dbintr->GetResultColumn("caf").c_str());
		const auto daf=atoi(dbintr->GetResultColumn("daf").c_str());
		const auto rr=atoi(dbintr->GetResultColumn("return_register").c_str());
		const auto ptrsize=atoi(dbintr->GetResultColumn("ptrsize").c_str());
		const auto& encoded_cie_program = dbintr->GetResultColumn("cie_program");
		const auto& encoded_fde_program = dbintr->GetResultColumn("fde_program");

		auto new_ehpgm=new EhProgram_t(eh_pgm_id, caf, daf, rr, ptrsize);
		decode_pgm(encoded_cie_program, new_ehpgm->GetCIEProgram());
		decode_pgm(encoded_fde_program, new_ehpgm->GetFDEProgram());

		idMap[eh_pgm_id]=new_ehpgm;
		eh_pgms.insert(new_ehpgm);
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
//  vaddress_offset       bigint,
//  doip_id               integer DEFAULT -1


		db_id_t aid=atoi(dbintr->GetResultColumn("address_id").c_str());
		db_id_t file_id=atoi(dbintr->GetResultColumn("file_id").c_str());
		virtual_offset_t vaddr=strtovo(dbintr->GetResultColumn("vaddress_offset"));
		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		AddressID_t *newaddr=new AddressID_t(aid,file_id,vaddr);

//std::cout<<"Found address "<<aid<<"."<<std::endl;

		idMap[aid]=newaddr;

		addrs.insert(newaddr);

		dbintr->MoveToNextRow();
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


	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
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


		db_id_t instruction_id=atoi(dbintr->GetResultColumn("instruction_id").c_str());
		db_id_t aid=atoi(dbintr->GetResultColumn("address_id").c_str());
		db_id_t parent_func_id=atoi(dbintr->GetResultColumn("parent_function_id").c_str());
		db_id_t orig_address_id=atoi(dbintr->GetResultColumn("orig_address_id").c_str());
		db_id_t fallthrough_address_id=atoi(dbintr->GetResultColumn("fallthrough_address_id").c_str());
		db_id_t targ_address_id=atoi(dbintr->GetResultColumn("target_address_id").c_str());
		db_id_t icfs_id=atoi(dbintr->GetResultColumn("icfs_id").c_str());
		db_id_t eh_pgm_id=atoi(dbintr->GetResultColumn("ehpgm_id").c_str());
		db_id_t eh_cs_id=atoi(dbintr->GetResultColumn("ehcss_id").c_str());
		std::string encoded_data=(dbintr->GetResultColumn("data"));
		std::string data=encoded_to_raw_string(encoded_data);
		std::string callback=(dbintr->GetResultColumn("callback"));
		std::string comment=(dbintr->GetResultColumn("comment"));
		db_id_t indirect_branch_target_address_id = atoi(dbintr->GetResultColumn("ind_target_address_id").c_str());
		db_id_t doipid=atoi(dbintr->GetResultColumn("doip_id").c_str());

		std::string isIndStr=(dbintr->GetResultColumn("ind_target_address_id"));

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

		if(eh_pgm_id != NOT_IN_DATABASE) newinsn->SetEhProgram(ehpgmMap.at(eh_pgm_id));
		if(eh_cs_id != NOT_IN_DATABASE) newinsn->SetEhCallSite(ehcsMap.at(eh_cs_id));
	
		if(parent_func)
		{
			parent_func->GetInstructions().insert(newinsn);
			newinsn->SetFunction(funcMap.at(parent_func_id));
		}

//std::cout<<"Found address "<<aid<<"."<<std::endl;

		idMap[instruction_id]=newinsn;
		fallthroughs[instruction_id]=fallthrough_address_id;
		targets[instruction_id]=targ_address_id;

		addressToInstructionMap[aid] = newinsn;
		insns.insert(newinsn);

		if (icfs_id == NOT_IN_DATABASE)
		{
			newinsn->SetIBTargets(NULL);
		}
		else
		{
			// keep track of instructions for which we have not yet
			// resolved the ICFS
			unresolvedICFS[newinsn] = icfs_id;
		}

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
		std::map<db_id_t,BaseObj_t*> 	&objMap
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
                db_id_t wrt_id=atoi(dbintr->GetResultColumn("wrt_id").c_str());
                uint32_t addend=atoi(dbintr->GetResultColumn("addend").c_str());


		BaseObj_t* wrt_obj=objMap[wrt_id];	 // might be null.
		Relocation_t *reloc=new Relocation_t(reloc_id,reloc_offset,reloc_type,wrt_obj,addend);

		assert(objMap[instruction_id]!=NULL);

		objMap[instruction_id]->GetRelocations().insert(reloc);
		relocs.insert(reloc);

		dbintr->MoveToNextRow();
	}

}


void FileIR_t::WriteToDB()
{
    	const auto WriteIRDB_start = clock();

	const auto pqIntr=dynamic_cast<pqxxDB_t*>(dbintr);
	assert(pqIntr);

	//Resolve (assemble) any instructions in the registry.
	AssembleRegistry();

	/* assign each item a unique ID */
	SetBaseIDS();

	CleanupICFS();

	db_id_t j=-1;

	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->instruction_table_name 	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->icfs_table_name 		+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->icfs_map_table_name 	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->function_table_name    	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->address_table_name     	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->relocs_table_name     	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->types_table_name     	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->scoop_table_name     	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->scoop_table_name+"_part2"+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->ehpgm_table_name     	+ string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->ehcss_table_name     	+ string(" cascade;"));

	/* and now that everything has an ID, let's write to the DB */

// write out the types
	string q=string("");
	for(std::set<Type_t*>::const_iterator t=types.begin(); t!=types.end(); ++t)
	{
		q+=(*t)->WriteToDB(fileptr,j); // @TODO: wtf is j?
		if(q.size()>1024*1024)
		{
			dbintr->IssueQuery(q);
			q=string("");
		}
	}
	dbintr->IssueQuery(q);


// write out functions 
	auto withHeader=true;
	q=string("");
	for(std::set<Function_t*>::const_iterator f=funcs.begin(); f!=funcs.end(); ++f)
	{
		q+=(*f)->WriteToDB(fileptr,j);
		if(q.size()>1024*1024)
		{
			dbintr->IssueQuery(q);
			q=string("");
		}
	}
	dbintr->IssueQuery(q);


// write out addresses
	pqxx::tablewriter W_addrs(pqIntr->GetTransaction(),fileptr->address_table_name);
	for(const auto &a : addrs)
	{
		W_addrs << a->WriteToDB(fileptr,j,withHeader);
	}
	W_addrs.complete();


// write out instructions

	pqxx::tablewriter W(pqIntr->GetTransaction(),fileptr->instruction_table_name);
	for(std::set<Instruction_t*>::const_iterator i=insns.begin(); i!=insns.end(); ++i)
	{	
		Instruction_t const * const insnp=*i;
		DISASM disasm;
		Disassemble(insnp,disasm);

		if(insnp->GetOriginalAddressID() == NOT_IN_DATABASE)
		{

			if(insnp->GetFallthrough()==NULL && 
				disasm.Instruction.BranchType!=RetType && disasm.Instruction.BranchType!=JmpType )
			{
				// instructions that fall through are required to either specify a fallthrough that's
				// in the IRDB, or have an associated "old" instruction.  
				// without these bits of information, the new instruction can't possibly execute correctly.
				// and we won't have the information necessary to emit spri.
				cerr << "NULL fallthrough: offending instruction:" << ((Instruction_t*)insnp)->getDisassembly() << " comment: " << ((Instruction_t*)insnp)->GetComment() << endl;
				assert(0);
				abort();
			}
			if(insnp->GetTarget()==NULL && disasm.Instruction.BranchType!=0 && 
				disasm.Instruction.BranchType!=RetType &&
				// not an indirect branch
				((disasm.Instruction.BranchType!=JmpType && disasm.Instruction.BranchType!=CallType) ||
				 disasm.Argument1.ArgType&CONSTANT_TYPE))
			{
				// direct branches are required to either specify a target that's
				// in the IRDB, or have an associated "old" instruction.  
				// without these bits of information, the new instruction can't possibly execute correctly.
				// and we won't have the information necessary to emit spri.
				cerr << "Call must have a target; offending instruction:" << ((Instruction_t*)insnp)->getDisassembly() << " comment: " << ((Instruction_t*)insnp)->GetComment() << endl;
				assert(0);
				abort();
			}
		}


		const auto &insn_values=(*i)->WriteToDB(fileptr,j);
		W << insn_values;

	}
	W.complete();



// icfs 
	for (ICFSSet_t::iterator it = GetAllICFS().begin(); it != GetAllICFS().end(); ++it)
	{
		ICFS_t* icfs = *it;
		assert(icfs);
		string q = icfs->WriteToDB(fileptr);
		dbintr->IssueQuery(q);
	}

// scoops
	for(DataScoopSet_t::const_iterator it=scoops.begin(); it!=scoops.end(); ++it)
	{
		DataScoop_t* scoop = *it;
		assert(scoop);
		string q = scoop->WriteToDB(fileptr,j);
		dbintr->IssueQuery(q);
	}

// ehpgms 
	pqxx::tablewriter W_eh(pqIntr->GetTransaction(),fileptr->ehpgm_table_name);
	for(const auto& i : eh_pgms)
	{
		W_eh << i->WriteToDB(fileptr);
	}
	W_eh.complete();

// eh css
	for(const auto& i : eh_css)
	{
		string q = i->WriteToDB(fileptr);
		dbintr->IssueQuery(q);
	}


// all relocs
	pqxx::tablewriter W_reloc(pqIntr->GetTransaction(),fileptr->relocs_table_name);

// eh css relocs
	for(const auto& i : eh_css)
	{
		for(auto& reloc : i->GetRelocations())
			W_reloc << reloc->WriteToDB(fileptr,i);
	}

// eh pgms relocs
	for(const auto& i : eh_pgms)
	{
		string r="";
		for(auto& reloc : i->GetRelocations())
			W_reloc << reloc->WriteToDB(fileptr,i);
	}
// scoops relocs
	for(const auto& i : scoops)
	{
		for(auto& reloc : i->GetRelocations())
			W_reloc << reloc->WriteToDB(fileptr,i);
	}
// write out instruction's relocs
	for(const auto& i : insns)
	{
		for(auto& reloc : i->GetRelocations())
			W_reloc << reloc->WriteToDB(fileptr,i);
	}
	W_reloc.complete();

	const auto WriteIRDB_end = clock();
	const auto read_time = (double)(ReadIRDB_end-ReadIRDB_start)/ CLOCKS_PER_SEC;
	const auto write_time = (double)(WriteIRDB_end-WriteIRDB_start)/ CLOCKS_PER_SEC;
	const auto wall_time = (double)(WriteIRDB_end-ReadIRDB_start)/ CLOCKS_PER_SEC;
	const auto transform_time=wall_time - read_time - write_time; 

	std::cout << std::dec; 
    	std::cout << std::fixed << std::setprecision(2) << "#ATTRIBUTE ReadIRDB_WallClock=" << read_time <<endl;
    	std::cout << std::fixed << std::setprecision(2) << "#ATTRIBUTE WriteIRDB_WallClock=" << write_time << endl;
    	std::cout << std::fixed << std::setprecision(2) << "#ATTRIBUTE TotalIRDB_WallClock=" << wall_time << endl;
    	std::cout << std::fixed << std::setprecision(2) << "#ATTRIBUTE TransformIRDB_WallClock=" << transform_time << endl;

}


db_id_t FileIR_t::GetMaxBaseID()
{
#define MAX(a,b) (((a)>(b)) ? (a) : (b))

	/* find the highest database ID */
	db_id_t j=0;
	for(auto i=funcs.begin(); i!=funcs.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=addrs.begin(); i!=addrs.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=insns.begin(); i!=insns.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=relocs.begin(); i!=relocs.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=types.begin(); i!=types.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=scoops.begin(); i!=scoops.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=icfs_set.begin(); i!=icfs_set.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=eh_pgms.begin(); i!=eh_pgms.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());
	for(auto i=eh_css.begin(); i!=eh_css.end(); ++i)
		j=MAX(j,(*i)->GetBaseID());

	return j+1;	 // easy to off-by-one this so we do it for a user just in case.
}

void FileIR_t::SetBaseIDS()
{
	auto j=GetMaxBaseID();
	/* increment past the max ID so we don't duplicate */
	j++;
	/* for anything that's not yet in the DB, assign an ID to it */
	for(auto i=funcs.begin(); i!=funcs.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=addrs.begin(); i!=addrs.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=insns.begin(); i!=insns.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=relocs.begin(); i!=relocs.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=types.begin(); i!=types.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=scoops.begin(); i!=scoops.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=icfs_set.begin(); i!=icfs_set.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=eh_pgms.begin(); i!=eh_pgms.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
	for(auto i=eh_css.begin(); i!=eh_css.end(); ++i)
		if((*i)->GetBaseID()==NOT_IN_DATABASE)
			(*i)->SetBaseID(j++);
}

int FileIR_t::GetArchitectureBitWidth()
{
	return archdesc->GetBitWidth();
}

void FileIR_t::SetArchitecture()
{

	/* the first 16 bytes of an ELF file define the magic number and ELF Class. */
    	unsigned char e_ident[16];

	DBinterface_t* myinter=BaseObj_t::GetInterface();
	pqxxDB_t *mypqxxintr=dynamic_cast<pqxxDB_t*>(myinter);

	int elfoid=GetFile()->GetELFOID();
        pqxx::largeobjectaccess loa(mypqxxintr->GetTransaction(), elfoid, PGSTD::ios::in);


        loa.cread((char*)&e_ident, sizeof(e_ident));

	archdesc=new ArchitectureDescription_t;

	if((e_ident[EI_MAG0]==ELFMAG0) && 
	   (e_ident[EI_MAG1]==ELFMAG1) && 
	   (e_ident[EI_MAG2]==ELFMAG2) && 
	   (e_ident[EI_MAG3]==ELFMAG3))
	{
		archdesc->SetFileType(AD_ELF);
	}
	else if((e_ident[EI_MAG0]==ELFMAG0) && 
	   (e_ident[EI_MAG1]=='C') && 
	   (e_ident[EI_MAG2]=='G') && 
	   (e_ident[EI_MAG3]=='C'))
	{
		archdesc->SetFileType(AD_ELF);
	}
	else if((e_ident[EI_MAG0]=='M') &&
	   (e_ident[EI_MAG1]=='Z'))
	{
		archdesc->SetFileType(AD_PE);
	}
	else
	{
		cerr << "File format magic number wrong:  is this an ELF, PE or CGC file? [" << e_ident[EI_MAG0] << " " << e_ident[EI_MAG1] << "]" << endl;
		exit(-1);
	}


	if (archdesc->GetFileType() == AD_PE)
	{
		// just assume 64 bit for Windows, o/w could also extract from file
		archdesc->SetBitWidth(64);
	}
	else
	{
		switch(e_ident[4])
		{
			case ELFCLASS32:
				archdesc->SetBitWidth(32);
				break;
			case ELFCLASS64:
				archdesc->SetBitWidth(64);
				break;
			case ELFCLASSNONE:
			default:
				cerr << "Unknown ELF class " <<endl;
				exit(-1);
		}
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

	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
//  type_id            integer,     
//  type               integer DEFAULT 0,    -- possible types (0: UNKNOWN)
//  name               text DEFAULT '',      -- string representation of the type
//  ref_type_id        integer DEFAULT -1,   -- for aggregate types and func
//  pos                integer DEFAULT -1,   -- for aggregate types, position in aggregate
//  ref_type_id2       integer DEFAULT -1,   -- for func types
//  doip_id            integer DEFAULT -1    -- the DOIP

		db_id_t tid=atoi(dbintr->GetResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->GetResultColumn("type").c_str());
		std::string name=dbintr->GetResultColumn("name");
		BasicType_t *t = NULL;	

//		cout << "fileir::ReadFromDB(): pass1: " << name << endl;
		switch(type) {
				case T_UNKNOWN:
				case T_VOID:
				case T_NUMERIC:
				case T_VARIADIC:
				case T_INT:
				case T_CHAR:
				case T_FLOAT:
				case T_DOUBLE:
					t = new BasicType_t(tid, type, name);	
					types.insert(t);
					tMap[tid] = t;
					break;

				case T_POINTER:
					// do nothing for pointers
					break;

				default:
					cerr << "ReadTypesFromDB(): ERROR: pass 1 should only see basic types or pointer"  << endl;
					assert(0); 
					break;						
		}

		dbintr->MoveToNextRow();
	} // end pass1

	char query[2048];

	// pass2 get pointers
	sprintf(query,"select * from %s WHERE type = %d;", fileptr->types_table_name.c_str(), T_POINTER);
	dbintr->IssueQuery(query);

	while(!dbintr->IsDone())
	{
		db_id_t tid=atoi(dbintr->GetResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->GetResultColumn("type").c_str());
		std::string name=dbintr->GetResultColumn("name");
		db_id_t ref1=atoi(dbintr->GetResultColumn("ref_type_id").c_str());
//		cout << "fileir::ReadFromDB(): pass2 (pointers): " << name << endl;
		switch(type) {
				case T_POINTER:
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

		dbintr->MoveToNextRow();
	} // end pass3

	// pass3 get all aggregates
	sprintf(query,"select * from %s WHERE type = %d order by type_id, pos;", fileptr->types_table_name.c_str(), T_AGGREGATE);
	dbintr->IssueQuery(query);

	while(!dbintr->IsDone())
	{
		db_id_t tid=atoi(dbintr->GetResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->GetResultColumn("type").c_str());
		std::string name=dbintr->GetResultColumn("name");
		db_id_t ref1=atoi(dbintr->GetResultColumn("ref_type_id").c_str());
		int pos=atoi(dbintr->GetResultColumn("pos").c_str());
		AggregateType_t *agg = NULL;	
//		cout << "fileir::ReadFromDB(): pass3 (aggregates): " << name << endl;
		switch(type) {
				case T_AGGREGATE:
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
						agg->AddAggregatedType(ref, pos);
					}
					break;

				default:
					cerr << "ReadTypesFromDB(): ERROR: pass2: unhandled type id = "  << type << endl;
					assert(0); // not yet handled
					break;						

		}

		dbintr->MoveToNextRow();
	} // end pass3

	// pass4 get all functions
	sprintf(query,"select * from %s WHERE type = %d;", fileptr->types_table_name.c_str(), T_FUNC);
	dbintr->IssueQuery(query);

	while(!dbintr->IsDone())
	{
		db_id_t tid=atoi(dbintr->GetResultColumn("type_id").c_str());
		IRDB_Type type=(IRDB_Type)atoi(dbintr->GetResultColumn("type").c_str());
		std::string name=dbintr->GetResultColumn("name");
		db_id_t ref1=atoi(dbintr->GetResultColumn("ref_type_id").c_str());
		db_id_t ref2=atoi(dbintr->GetResultColumn("ref_type_id2").c_str());
		FuncType_t *fn = NULL;	

		switch(type) {
				case T_FUNC:
					{
					assert(tMap.count(tid) == 0);

					fn = new FuncType_t(tid, name);	
					types.insert(fn);
					tMap[tid] = fn;
					assert(tMap[ref1]); // return type
					assert(tMap[ref2]); // argument type (which is an aggregate)
					fn->SetReturnType(tMap[ref1]);
					AggregateType_t *args = dynamic_cast<AggregateType_t*>(tMap[ref2]);
					assert(args);
					fn->SetArgumentsType(args);
					}
					break;

				default:
					cerr << "ReadTypesFromDB(): ERROR: pass4: unhandled type id = "  << type << endl;
					assert(0); // not yet handled
					break;						

		}

		dbintr->MoveToNextRow();
	} // end pass4

	return tMap;
}

void FileIR_t::ReadAllICFSFromDB(std::map<db_id_t,Instruction_t*> &addr2instMap,
		std::map<Instruction_t*, db_id_t> &unresolvedICFS)
{
	std::map<db_id_t, ICFS_t*> icfsMap;

	// retrieve all sets
	std::string q= "select * from " + fileptr->icfs_table_name + " ; ";
	dbintr->IssueQuery(q);

	while(!dbintr->IsDone())
	{
		db_id_t icfs_id = atoi(dbintr->GetResultColumn("icfs_id").c_str());
		string statusString=dbintr->GetResultColumn("icfs_status"); 

		ICFS_t* icfs = new ICFS_t(icfs_id, statusString);		
		GetAllICFS().insert(icfs);

		icfsMap[icfs_id] = icfs;
		dbintr->MoveToNextRow();
	}

	ICFSSet_t all_icfs = GetAllICFS();

	// for each set, populate its members
	for (ICFSSet_t::iterator it = all_icfs.begin(); it != all_icfs.end(); ++it)
	{
		char query2[2048];
		ICFS_t *icfs = *it;
		assert(icfs);
		int icfsid = icfs->GetBaseID();
		sprintf(query2,"select * from %s WHERE icfs_id = %d;", fileptr->icfs_map_table_name.c_str(), icfsid);
		dbintr->IssueQuery(query2);
		while(!dbintr->IsDone())
		{
			db_id_t address_id = atoi(dbintr->GetResultColumn("address_id").c_str());
			Instruction_t* instruction = addr2instMap[address_id];
			if (instruction) 
			{
				icfs->insert(instruction);
			}

			// @todo: handle cross-file addresses
			//        these are allowed by the DB schema but we don't yet handle them
			// if we encounter an unresolved address, we should mark the ICFS
			//      as unresolved

			dbintr->MoveToNextRow();
		}					
	}

	// backpatch all unresolved instruction -> ICFS
	std::map<Instruction_t*, db_id_t>::iterator uit;
	for (std::map<Instruction_t*, db_id_t>::iterator uit = unresolvedICFS.begin(); uit != unresolvedICFS.end(); ++uit)
	{
		Instruction_t* unresolved = uit->first;
		db_id_t icfs_id = uit->second;

		assert(unresolved);

		ICFS_t *icfs = icfsMap[icfs_id];
		assert(icfs);

		unresolved->SetIBTargets(icfs);
	}
}

void FileIR_t::GarbageCollectICFS()
{
	std::set<ICFS_t*> used_icfs;

	for(set<Instruction_t*>::const_iterator it=this->GetInstructions().begin();
		it!=this->GetInstructions().end();
   		++it)
	{
		Instruction_t* instr=*it;
		if(instr && instr->GetIBTargets())
		{ 
			used_icfs.insert(instr->GetIBTargets());
		}
	}

	int unused_icfs = this->GetAllICFS().size() - used_icfs.size();
	if (unused_icfs > 0)
	{
		cerr << "FileIR_t::GarbageCollectICFS(): WARNING: " << dec << unused_icfs << " unused ICFS found. ";
		cerr << "Deleting before committing to IRDB" << endl;
	}

	ICFSSet_t to_erase;
	for(ICFSSet_t::const_iterator it=this->GetAllICFS().begin();
		it != this->GetAllICFS().end();
		++it)
	{
		ICFS_t* icfs = *it;
		if (used_icfs.count(icfs) == 0)
		{
			to_erase.insert(icfs);
		}
	}

	for(ICFSSet_t::const_iterator it=to_erase.begin();
		it != to_erase.end();
		++it)
	{
		ICFS_t* icfs = *it;
		this->GetAllICFS().erase(icfs);
	}

}

void FileIR_t::DedupICFS()
{
	std::set<ICFS_t> unique_icfs;

	ICFSSet_t& all_icfs=this->GetAllICFS();

	// detect duplicate icfs
	ICFSSet_t duplicates;
	std::pair<std::set<ICFS_t>::iterator,bool> ret;
	for(ICFSSet_t::iterator it=all_icfs.begin(); it!=all_icfs.end(); ++it)
	{
		ICFS_t* p=*it;
		assert(p);
		ret = unique_icfs.insert( *p );
		if (!ret.second) {
			duplicates.insert(p);
		}
	}

	if (duplicates.size() > 0)
	{
		cerr << "FileIR_t::DedupICFS(): WARNING: " << dec << duplicates.size() << " duplicate ICFS out of " << all_icfs.size() << " total ICFS";
		cerr << ". De-duplicating before committing to IRDB" << endl;
	}

	// remove duplicate icfs
	for(ICFSSet_t::const_iterator it=duplicates.begin(); it!=duplicates.end(); ++it)
	{
		ICFS_t* icfs = *it;
		all_icfs.erase(icfs);
	}

	// build duplicate icfs map
	std::map<ICFS_t*, ICFS_t*> duplicate_map;
	for(ICFSSet_t::const_iterator it=duplicates.begin(); it!=duplicates.end(); ++it)
	{
		ICFS_t* icfs = *it;
		for(ICFSSet_t::iterator it=all_icfs.begin(); it!=all_icfs.end(); ++it)
		{
			ICFS_t* t = *it;

			assert(t);
			if (*icfs == *t)
			{
				duplicate_map[icfs] = t;
				cerr << "FileIR_t::DedupICFS(): remap: icfs id " << icfs->GetBaseID() << " --> icsf id " << t->GetBaseID() << endl;
				break;
			}
		}
	}

	// reassign ibtargets 
	for(set<Instruction_t*>::const_iterator it=this->GetInstructions().begin();
		it!=this->GetInstructions().end();
   		++it)
	{
		Instruction_t* instr=*it;
		if(instr->GetIBTargets() && duplicate_map[instr->GetIBTargets()])
		{ 
			instr->SetIBTargets(duplicate_map[instr->GetIBTargets()]);
		}
	}
}

void FileIR_t::CleanupICFS()
{
	GarbageCollectICFS();
	DedupICFS();
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
        //dbintr->IssueQuery(q);
        //while(!dbintr->IsDone())
	//{
        //        db_id_t sid=atoi(dbintr->GetResultColumn("scoop_id").c_str());
	//	bonus_contents[sid]=dbintr->GetResultColumn("data");
	//	dbintr->MoveToNextRow();
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
        dbintr->IssueQuery(q);
        while(!dbintr->IsDone())
        {

                db_id_t sid=atoi(dbintr->GetResultColumn("scoop_id").c_str());
                std::string name=dbintr->GetResultColumn("name");
                db_id_t type_id=atoi(dbintr->GetResultColumn("type_id").c_str());
		Type_t *type=typeMap[type_id];
                db_id_t start_id=atoi(dbintr->GetResultColumn("start_address_id").c_str());
		AddressID_t* start_addr=addrMap[start_id];
                db_id_t end_id=atoi(dbintr->GetResultColumn("end_address_id").c_str());
		AddressID_t* end_addr=addrMap[end_id];
                int permissions=atoi(dbintr->GetResultColumn("permissions").c_str());
                bool is_relro=atoi(dbintr->GetResultColumn("relro").c_str()) != 0 ;

		DataScoop_t* newscoop=new DataScoop_t(sid,name,start_addr,end_addr,type,permissions,is_relro,"");
		assert(newscoop);
		GetDataScoops().insert(newscoop);
		dbintr->MoveToNextRow();

		scoopMap[sid]=newscoop;
	}

	for(DataScoopSet_t::iterator it=GetDataScoops().begin(); it!=GetDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

        	q= "select length(data) from " + fileptr->scoop_table_name + " where scoop_id='"+to_string(scoop->GetBaseID())+"'; ";
        	dbintr->IssueQuery(q);
		if(!dbintr->IsDone())
		{
			int data_len=atoi(dbintr->GetResultColumn("length").c_str());
			for(int i=0;i<data_len;i+=SCOOP_CHUNK_SIZE)
			{
				string start_pos=to_string(i);
				string len_to_get=to_string(SCOOP_CHUNK_SIZE);
				string field="substr(data,"+start_pos+","+len_to_get+")";
				q= "select "+field+" from " + fileptr->scoop_table_name + " where scoop_id='"+to_string(scoop->GetBaseID())+"'; ";
				dbintr->IssueQuery(q);

				scoop->GetContents()+=dbintr->GetResultColumn("substr");

			}
		}


		// read part 2 from db
        	q= "select length(data) from " + fileptr->scoop_table_name + "_part2 where scoop_id='"+to_string(scoop->GetBaseID())+"'; ";
        	dbintr->IssueQuery(q);
		if(!dbintr->IsDone())
		{
			int part2_len=atoi(dbintr->GetResultColumn("length").c_str());
			for(int i=0;i<part2_len; i+=SCOOP_CHUNK_SIZE)
			{
				string start_pos=to_string(i);
				string len_to_get=to_string(SCOOP_CHUNK_SIZE);
				string field="substr(data,"+start_pos+","+len_to_get+")";
				q= "select "+field+" from " + fileptr->scoop_table_name + "_part2 where scoop_id='"+to_string(scoop->GetBaseID())+"'; ";
				dbintr->IssueQuery(q);

				scoop->GetContents()+=dbintr->GetResultColumn("substr");

			}
		}
	}
	for(	DataScoopSet_t::const_iterator it=GetDataScoops().begin();
		it!=GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* scoop=*it;
		assert(scoop->GetContents().size() == scoop->GetSize());
	}


	return scoopMap;
}


// Lookup a scoop by address
DataScoop_t* FileIR_t::FindScoop(const libIRDB::virtual_offset_t &addr)
{
	for(DataScoopSet_t::iterator it=scoops.begin(); it!=scoops.end(); ++it)
	{
		// we're doing <= in both comparisons here intentionally.
		// scoop addresses are the start/end address are inclusive
		// so that start+size-1 == end.
		if( (*it)->GetStart()->GetVirtualOffset() <= addr && addr <= (*it)->GetEnd()->GetVirtualOffset() )
			return *it;
	}
	return NULL;
}


void FileIR_t::SplitScoop(
		DataScoop_t *tosplit, 
		const libIRDB::virtual_offset_t &addr, 
		size_t size, 
		DataScoop_t* &before,
		DataScoop_t* &containing, 
		DataScoop_t* &after,
		db_id_t *max_id_ptr	 /* in and out param */
	)
{

	// init output params.
	before=containing=after=NULL;


	if(tosplit->GetStart()->GetVirtualOffset() == addr &&
		tosplit->GetEnd()->GetVirtualOffset() == addr+size-1)
	{
		// no split necessary
		containing=tosplit;
		return;
	}

	auto calcd_max_id=db_id_t(0);
	if(max_id_ptr==NULL)
		calcd_max_id = GetMaxBaseID();

	auto &max_id = max_id_ptr == NULL ? calcd_max_id : *max_id_ptr ;
	
	if(max_id==0)
		max_id=calcd_max_id;

	const auto multiple=(unsigned)1000;

	// round to nearest multiple
	const auto rounded_max = ((max_id + (multiple/2)) / multiple) * multiple;

	max_id=rounded_max + multiple; // and skip forward so we're passed the highest.

	const bool needs_before = addr!=tosplit->GetStart()->GetVirtualOffset();
	const bool needs_after = addr+size-1 != tosplit->GetEnd()->GetVirtualOffset();


	if(needs_before)
	{
		// setup before
		// const AddressID_t* before_start=NULL;
	
		const auto before_start=new AddressID_t;
		before_start->SetBaseID(max_id++);
		before_start->SetFileID(tosplit->GetStart()->GetFileID());
		before_start->SetVirtualOffset(tosplit->GetStart()->GetVirtualOffset());

		// const AddressID_t* before_end=new AddressID_t;
		const auto before_end=new AddressID_t;
		before_end->SetBaseID(max_id++);
		before_end->SetFileID(tosplit->GetStart()->GetFileID());
		before_end->SetVirtualOffset(addr-1);

		before=new DataScoop_t;
		before->SetBaseID(max_id++);
		before->SetName(tosplit->GetName()+"3");
		before->SetStart(before_start);
		before->SetEnd(before_end);
		before->setRawPerms(tosplit->getRawPerms());
		before->GetContents().resize(before_end->GetVirtualOffset() - before_start->GetVirtualOffset()+1);

		// copy bytes
		for(virtual_offset_t i=before_start->GetVirtualOffset() ; i <= before_end->GetVirtualOffset(); i++)
			before->GetContents()[i-before_start->GetVirtualOffset()] = tosplit->GetContents()[i-tosplit->GetStart()->GetVirtualOffset()];
	

		GetAddresses().insert(before_start);
		GetAddresses().insert(before_end);
		GetDataScoops().insert(before);
	}

	// setup containing
	// AddressID_t* containing_start=new AddressID_t;
	const auto containing_start=new AddressID_t;
	containing_start->SetBaseID(max_id++);
	containing_start->SetFileID(tosplit->GetStart()->GetFileID());
	containing_start->SetVirtualOffset(addr);

	//AddressID_t* containing_end=new AddressID_t;
	const auto containing_end=new AddressID_t;
	containing_end->SetBaseID(max_id++);
	containing_end->SetFileID(tosplit->GetStart()->GetFileID());
	containing_end->SetVirtualOffset(addr+size-1);

	containing=new DataScoop_t;
	containing->SetBaseID(max_id++);
	containing->SetName(tosplit->GetName()+"3");
	containing->SetStart(containing_start);
	containing->SetEnd(containing_end);
	containing->setRawPerms(tosplit->getRawPerms());
	containing->GetContents().resize(containing_end->GetVirtualOffset() - containing_start->GetVirtualOffset()+1);
	// copy bytes
	for(virtual_offset_t i=containing_start->GetVirtualOffset() ; i <= containing_end->GetVirtualOffset(); i++)
		containing->GetContents()[i-containing_start->GetVirtualOffset()] = tosplit->GetContents()[i-tosplit->GetStart()->GetVirtualOffset()];

	GetAddresses().insert(containing_start);
	GetAddresses().insert(containing_end);
	GetDataScoops().insert(containing);
		

	if(needs_after)
	{
		// setup after
		// AddressID_t* after_start=new AddressID_t;
		const auto after_start=new AddressID_t;
		after_start->SetBaseID(max_id++);
		after_start->SetFileID(tosplit->GetStart()->GetFileID());
		after_start->SetVirtualOffset(addr+size);

		// AddressID_t* after_end=new AddressID_t;
		const auto after_end=new AddressID_t;
		after_end->SetBaseID(max_id++);
		after_end->SetFileID(tosplit->GetStart()->GetFileID());
		after_end->SetVirtualOffset(tosplit->GetEnd()->GetVirtualOffset());

		after=new DataScoop_t;
		after->SetBaseID(max_id++);
		after->SetName(tosplit->GetName()+"3");
		after->SetStart(after_start);
		after->SetEnd(after_end);
		after->setRawPerms(tosplit->getRawPerms());
		after->GetContents().resize(after_end->GetVirtualOffset() - after_start->GetVirtualOffset()+1);
		// copy bytes
		for(virtual_offset_t i=after_start->GetVirtualOffset() ; i <= after_end->GetVirtualOffset(); i++)
			after->GetContents()[i-after_start->GetVirtualOffset()] = tosplit->GetContents()[i-tosplit->GetStart()->GetVirtualOffset()];

		GetAddresses().insert(after_start);
		GetAddresses().insert(after_end);
		GetDataScoops().insert(after);
	}


	// since tosplit is going away, we need to move every relocation from the 
	// tosplit scoop into one of the others.  Iterate through each and 
	// adjust the reloc's offset accordingly before inserting into the correct scoop's reloc set.
	while(!tosplit->GetRelocations().empty())
	{
		Relocation_t* reloc=*(tosplit->GetRelocations().begin());
		tosplit->GetRelocations().erase(tosplit->GetRelocations().begin());

		virtual_offset_t reloc_start=reloc->GetOffset()+tosplit->GetStart()->GetVirtualOffset();

		if(reloc_start < containing->GetStart()->GetVirtualOffset() )
		{
			before->GetRelocations().insert(reloc);
		}
		else if(reloc_start > containing->GetEnd()->GetVirtualOffset() )
		{
			reloc->SetOffset(reloc_start-after->GetStart()->GetVirtualOffset());
			after->GetRelocations().insert(reloc);
		}
		else
		{
			reloc->SetOffset(reloc_start-containing->GetStart()->GetVirtualOffset());
			containing->GetRelocations().insert(reloc);
		}
	
	}

	/* look at each relocation in the IR */
	for(auto & r : GetRelocations())
	{
		if(r->GetWRT()==tosplit)
		{
			const auto &addend=r->GetAddend();
			const auto containing_start_offset=(containing -> GetStart()->GetVirtualOffset() - 
				tosplit->GetStart()->GetVirtualOffset());
			const auto containing_end_offset=containing_start_offset+containing->GetSize();
			if(needs_before && addend<before->GetSize())
			{
				r->SetWRT(before);
			}
			else if( addend < containing_end_offset)
			{
				r->SetWRT(containing);
				r->SetAddend(addend-containing_start_offset);
			}
			else 		
			{
				assert(needs_after);
				const auto after_start_offset=(after -> GetStart()->GetVirtualOffset() - 
					tosplit->GetStart()->GetVirtualOffset());
				r->SetWRT(after);
				r->SetAddend(addend-after_start_offset);
			}
		}
	}
	

	GetAddresses().erase(tosplit->GetStart());
	GetAddresses().erase(tosplit->GetEnd());
	GetDataScoops().erase(tosplit);

	delete tosplit->GetStart();
	delete tosplit->GetEnd();
	delete tosplit;

	return;
}

