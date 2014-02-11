
#include <all.hpp>
#include <utils.hpp>
#include <cstdlib>
#include <map>
#include <fstream>
#include <elf.h>
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

		assert(func_entry_id==-1 || insnMap[func_entry_id]);
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
	//Resolve (assemble) any instructions in the registry.
	AssembleRegistry();

	/* assign each item a unique ID */
	SetBaseIDS();

	db_id_t j=-1;

	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->instruction_table_name + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->function_table_name    + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->address_table_name     + string(" cascade;"));
	dbintr->IssueQuery(string("TRUNCATE TABLE ")+ fileptr->relocs_table_name     + string(" cascade;"));

	/* and now that everything has an ID, let's write to the DB */
	bool withHeader;

	withHeader = true;
	string q=string("");
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

	withHeader = true;
	q=string("");
	for(std::set<AddressID_t*>::const_iterator a=addrs.begin(); a!=addrs.end(); ++a)
	{
		q+=(*a)->WriteToDB(fileptr,j,withHeader);
		withHeader = false;
		if(q.size()>1024*1024)
		{
			q+=";";
			dbintr->IssueQuery(q);
			q=string("");
			withHeader = true;
		}
	}
	dbintr->IssueQuery(q);

	withHeader = true;
	q=string("");
	for(std::set<Instruction_t*>::const_iterator i=insns.begin(); i!=insns.end(); ++i)
	{	
		Instruction_t const * const insnp=*i;
		DISASM disasm;
		insnp->Disassemble(disasm);

		// we have a few new requirements for instructions that doesn't correspond to original program insns.
		if(insnp->GetOriginalAddressID() == NOT_IN_DATABASE)
		{

			if(insnp->GetFallthrough()==NULL && 
				disasm.Instruction.BranchType!=RetType && disasm.Instruction.BranchType!=JmpType )
			{
				// instructions that fall through are required to either specify a fallthrough that's
				// in the IRDB, or have an associated "old" instruction.  
				// without these bits of information, the new instruction can't possibly execute correctly.
				// and we won't have the information necessary to emit spri.
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
				assert(0);
				abort();
			}
		}

		q+=(*i)->WriteToDB(fileptr,j,withHeader);
		withHeader = false;
		if(q.size()>1024*1024)
		{
			q+=";";
			dbintr->IssueQuery(q);
			q=string("");
			withHeader = true;
		}

		string r="";
		std::set<Relocation_t*> relocs = (*i)->GetRelocations();
		for(set<Relocation_t*>::iterator it=relocs.begin(); it!=relocs.end(); ++it)
		{
			Relocation_t* reloc=*it;
			r+=reloc->WriteToDB(fileptr,*i);
		}
		dbintr->IssueQuery(r);
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

	if((e_ident[EI_MAG0]!=ELFMAG0) || 
	   (e_ident[EI_MAG1]!=ELFMAG1) || 
	   (e_ident[EI_MAG2]!=ELFMAG2) || 
	   (e_ident[EI_MAG3]!=ELFMAG3))
	{
		cerr << "ELF magic number wrong:  is this an ELF file? " <<endl;
		exit(-1);
	}

	archdesc=new ArchitectureDescription_t;

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

ArchitectureDescription_t* FileIR_t::archdesc=NULL;
