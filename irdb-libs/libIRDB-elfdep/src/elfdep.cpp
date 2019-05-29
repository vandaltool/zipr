/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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


#include <irdb-core>
#include <irdb-transform>
#include <irdb-elfdep>
#include <stdlib.h>
#include <memory>
#include <math.h>
#include <elf.h>
#include <libElfDep.hpp>
#include <iterator>
#include <algorithm>

using namespace libIRDB;
using namespace std;

// use some of IRDB_SDK, without committing to all of it.
// it's too confusing to use libIRDB namespace as well as the IRDB_SDK namespace;
using IRDB_SDK::VirtualOffset_t;
using IRDB_SDK::Relocation_t;
using IRDB_SDK::Instruction_t;
using IRDB_SDK::DataScoop_t;

// defines
#define REV_ALLOF(a) rbegin(a), rend(a)
#define ALLOF(a) begin(a), end(a)

// static helpers

// use this to determine whether a scoop has a given name.
static struct ScoopFinder : binary_function<const IRDB_SDK::DataScoop_t*,const string,bool>
{
	// declare a simple scoop finder function that finds scoops by name
	bool operator()(const IRDB_SDK::DataScoop_t* scoop, const string& name) const
	{
		return (scoop->getName() == name);
	};
} finder;

static IRDB_SDK::DataScoop_t* find_scoop(IRDB_SDK::FileIR_t *firp,const string &name)
{
	auto it=find_if(firp->getDataScoops().begin(), firp->getDataScoops().end(), bind2nd(finder, name)) ;
	if( it != firp->getDataScoops().end() )
		return *it;
	return NULL;
};

static unsigned int add_to_scoop(const string &str, IRDB_SDK::DataScoop_t* scoop) 
{
	// assert that this scoop is unpinned.  may need to enable --step move_globals --step-option move_globals:--cfi
	assert(scoop->getStart()->getVirtualOffset()==0);
	int len=str.length();
	scoop->setContents(scoop->getContents()+str);
	VirtualOffset_t oldend=scoop->getEnd()->getVirtualOffset();
	VirtualOffset_t newend=oldend+len;
	scoop->getEnd()->setVirtualOffset(newend);
	return oldend+1;
};

template<int ptrsize>
static void insert_into_scoop_at(const string &str, IRDB_SDK::DataScoop_t* scoop, IRDB_SDK::FileIR_t* firp, const unsigned int at) 
{
	// assert that this scoop is unpinned.  may need to enable --step move_globals --step-option move_globals:--cfi
	assert(scoop->getStart()->getVirtualOffset()==0);
	int len=str.length();
	string new_scoop_contents=scoop->getContents();
	new_scoop_contents.insert(at,str);
	scoop->setContents(new_scoop_contents);

	VirtualOffset_t oldend=scoop->getEnd()->getVirtualOffset();
	VirtualOffset_t newend=oldend+len;
	scoop->getEnd()->setVirtualOffset(newend);

	// update each reloc to point to the new location.
	for(auto reloc : scoop->getRelocations())
	{
		if((unsigned int)reloc->getOffset()>=at)
			reloc->setOffset(reloc->getOffset()+str.size());
		
	};

	// for each scoop
	for(auto scoop_to_update : firp->getDataScoops())
	{
		// for each relocation for that scoop
		for (auto reloc : scoop_to_update->getRelocations())
		{
			// if it's a reloc that's wrt scoop
			auto wrt=dynamic_cast<IRDB_SDK::DataScoop_t*>(reloc->getWRT());
			if(wrt==scoop)
			{
				// then we need to update the scoop
				if(reloc->getType()=="dataptr_to_scoop")
				{
					string contents=scoop_to_update->getContents();
					// subtract the stringsize from the (implicitly stored) addend
					// taking pointer size into account.
					switch(ptrsize)
					{
						case 4:
						{
							unsigned int val=*((unsigned int*)&contents.c_str()[reloc->getOffset()]); 
							if(val>=at)
								val +=str.size();
							contents.replace(reloc->getOffset(), ptrsize, (const char*)&val, ptrsize);
							break;
						
						}
						case 8:
						{
							unsigned long long val=*((long long*)&contents.c_str()[reloc->getOffset()]); 
							if(val>=at)
								val +=str.size();
							contents.replace(reloc->getOffset(), ptrsize, (const char*)&val, ptrsize);
							break;

						}
						default: 
							assert(0);
					}
					scoop_to_update->setContents(contents);
				}
			}	

		};
		
	};
};

template<int ptrsize>
static void prefix_scoop(const string &str, IRDB_SDK::DataScoop_t* scoop, IRDB_SDK::FileIR_t* firp) 
{
	insert_into_scoop_at<ptrsize>(str,scoop,firp,0);
};


// Public interfaces for ElfDependencies_t



// constructors
ElfDependencies_t::ElfDependencies_t(IRDB_SDK::FileIR_t* firp)
	: Transform_t(firp)
{
	typedef ElfDependencies_t::ElfDependenciesImpl_t<Elf64_Sym, Elf64_Rela, Elf64_Dyn, R_X86_64_GLOB_DAT, 32, 8> ElfDependencies64_t;
	typedef ElfDependencies_t::ElfDependenciesImpl_t<Elf32_Sym, Elf32_Rel, Elf32_Dyn, R_386_GLOB_DAT, 8, 4> ElfDependencies32_t;

	if(firp->getArchitectureBitWidth()==32)
		transformer.reset(new ElfDependencies32_t(firp));
	else if(firp->getArchitectureBitWidth()==64)
		transformer.reset(new ElfDependencies64_t(firp));
	else
		assert(0);
}


template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::ElfDependenciesImpl_t(IRDB_SDK::FileIR_t* firp)
	: ElfDependencies_t::ElfDependenciesBase_t(firp)
{
}


template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
pair<IRDB_SDK::DataScoop_t*,int> ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::appendGotEntry(const string &name)
{
	auto got_scoop=add_got_entry(name);
	return {got_scoop,0};
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
IRDB_SDK::Instruction_t* ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::appendPltEntry(const string &name)
{

	static int labelcounter=0;

	stringstream labelstream;
	labelstream << "L_pltentry_" << labelcounter++;

	auto got_scoop=add_got_entry(name);

	auto newinsn=addNewAssembly(labelstream.str()+": jmp [rel "+labelstream.str()+"]");
	
	/*
	auto newreloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE, 0, "pcrel", got_scoop);

	dynamic_cast<libIRDB::Instruction_t*>(newinsn)->GetRelocations().insert(newreloc);
	dynamic_cast<libIRDB::FileIR_t*>(getFileIR())->GetRelocations().insert(newreloc);
	*/
	auto newreloc=getFileIR()->addNewRelocation(newinsn, 0,"pcrel", got_scoop);
	(void)newreloc; // just give to IR.
	newinsn->getAddress()->setFileID(getFileIR()->getFile()->getBaseID());

	return newinsn;
}





// please  keep this if 0, as we likely want to add plt/got entries in a library later, but
// we need a use case to test this code -- it was copied from CFI.

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
IRDB_SDK::Instruction_t* ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::find_runtime_resolve(IRDB_SDK::DataScoop_t* gotplt_scoop)
{
	const auto firp=getFileIR();
	// find any data_to_insn_ptr reloc for the gotplt scoop
	auto it=find_if(gotplt_scoop->getRelocations().begin(), gotplt_scoop->getRelocations().end(), [](IRDB_SDK::Relocation_t* reloc)
	{
		return reloc->getType()=="data_to_insn_ptr";
	});
	// there _should_ be one.
	assert(it!=gotplt_scoop->getRelocations().end());

	Relocation_t* reloc=*it;
	auto wrt=dynamic_cast<Instruction_t*>(reloc->getWRT());
	assert(wrt);	// should be a WRT
	assert(wrt->getDisassembly().find("push ") != string::npos);	// should be push K insn
	return wrt->getFallthrough();	// jump to the jump, or not.. doesn't matter.  zopt will fix
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
IRDB_SDK::DataScoop_t* ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::add_got_entry(const std::string& name)
{
	const auto firp=getFileIR(); // dynamic_cast<libIRDB::FileIR_t*>(getFileIR());
	// find relevant scoops
	auto dynamic_scoop=find_scoop(firp,".dynamic");
	// auto gotplt_scoop=find_scoop(firp,".got.plt");
	//auto got_scoop=find_scoop(firp,".got");
	auto dynstr_scoop=find_scoop(firp,".dynstr");
	auto dynsym_scoop=find_scoop(firp,".dynsym");
	auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");
	auto relscoop=relaplt_scoop!=NULL ?  relaplt_scoop : relplt_scoop;
	auto gnu_version_scoop=find_scoop(firp,".gnu.version");


	// if there is versioning info in this binary, assert we unpinned it.
	if(gnu_version_scoop)
		assert(gnu_version_scoop->getStart()->getVirtualOffset()==0);

	if (!relscoop) 
		throw std::logic_error("Cannot find rela.plt or rel.plt. Did you remember to use move_globals with --elf_tables?");

	// add 0-init'd pointer to table
	auto new_got_entry_str=string(ptrsize,0);	 // zero-init a pointer-sized string


	// create a new, unpinned, rw+relro scoop that's an empty pointer.
	const auto start_addr=firp->addNewAddress(firp->getFile()->getBaseID(), 0);
	const auto end_addr  =firp->addNewAddress(firp->getFile()->getBaseID(), ptrsize-1);
	auto external_func_addr_scoop=firp->addNewDataScoop(name,start_addr,end_addr,NULL,6,true,new_got_entry_str);

	// add string to string table 
	auto dl_str_pos=add_to_scoop(name+'\0', dynstr_scoop);

	// add symbol to dlsym
	T_Elf_Sym dl_sym;
	memset(&dl_sym,0,sizeof(T_Elf_Sym));
	dl_sym.st_name=dl_str_pos;
	dl_sym.st_info=((STB_GLOBAL<<4)| (STT_OBJECT));
	string dl_sym_str((const char*)&dl_sym, sizeof(T_Elf_Sym));
	unsigned int dl_pos=add_to_scoop(dl_sym_str,dynsym_scoop);

	// if there is versioning info in this binary, update it.
	if(gnu_version_scoop)
	{
		// update the gnu.version section so that the new symbol has a version.
		const auto new_version_str=string("\0\0", 2);	 // \0\0 means *local*, as in, don't index the gnu.verneeded array.
		add_to_scoop(new_version_str,gnu_version_scoop);
	}

	

	// find the rela count.  can't insert before that.
	int rela_count=0;
	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->getSize(); i+=sizeof(T_Elf_Dyn))
	{
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->getContents().c_str()[i];
		if(dyn_entry.d_tag==DT_RELACOUNT)	 // diff than rela size.
		{
			// add to the size
			rela_count=dyn_entry.d_un.d_val;
			break;
		}
	}

	// create the new reloc 
	T_Elf_Rela dl_rel;
	memset(&dl_rel,0,sizeof(dl_rel));
	auto r_info_tmp=(decltype(dl_rel.r_info))dl_pos;
	dl_rel.r_info= ((r_info_tmp/sizeof(T_Elf_Sym)) <<rela_shift ) | reloc_type;

	string dl_rel_str((const char*)&dl_rel, sizeof(dl_rel));

// need to fixup relocs
	unsigned int at=rela_count*sizeof(T_Elf_Rela);
	insert_into_scoop_at<ptrsize>(dl_rel_str, relscoop, firp, at);

	/*
	auto dl_reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE,  at+((uintptr_t)&dl_rel.r_offset -(uintptr_t)&dl_rel), "dataptr_to_scoop", external_func_addr_scoop);
	dynamic_cast<libIRDB::DataScoop_t*>(relscoop)->GetRelocations().insert(dl_reloc);
	firp->GetRelocations().insert(dl_reloc);
	*/
	auto dl_reloc=firp->addNewRelocation(relscoop,at+((uintptr_t)&dl_rel.r_offset -(uintptr_t)&dl_rel), "dataptr_to_scoop", external_func_addr_scoop);
	(void)dl_reloc; // just give to IR;

	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->getSize(); i+=sizeof(T_Elf_Dyn))
	{
		// cast the index'd c_str to an Elf_Dyn pointer and deref it to assign to a 
		// reference structure.  That way editing the structure directly edits the string.
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->getContents().c_str()[i];
		if(dyn_entry.d_tag==DT_RELASZ)
			// add to the size
			dyn_entry.d_un.d_val+=sizeof(T_Elf_Rela);

		// we insert the zest_cfi_dispatch symbol after the relative relocs.
		// but we need to adjust the start if there are no relative relocs.
		if(at == 0  && dyn_entry.d_tag==DT_RELA)
			// subtract from the start.
			dyn_entry.d_un.d_val-=sizeof(T_Elf_Rela);

	}
	return external_func_addr_scoop;
}

#if 0

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
bool ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::add_got_entries()
{
	const auto firp=getFileIR();

	// find all the necessary scoops;
	auto dynamic_scoop=find_scoop(firp,".dynamic");
	auto gotplt_scoop=find_scoop(firp,".got.plt");
	auto got_scoop=find_scoop(firp,".got");
	auto dynstr_scoop=find_scoop(firp,".dynstr");
	auto dynsym_scoop=find_scoop(firp,".dynsym");
	auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");
	auto relscoop=relaplt_scoop!=NULL ?  relaplt_scoop : relplt_scoop;

	// Instruction_t* to_dl_runtime_resolve=find_runtime_resolve<T_Elf_Sym,T_Elf_Rela, T_Elf_Dyn, rela_shift, reloc_type, ptrsize>(gotplt_scoop);
	Instruction_t* to_dl_runtime_resolve=find_runtime_resolve(gotplt_scoop);


	// add necessary GOT entries.
	// add_got_entry<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>("zest_cfi_dispatch");
	add_got_entry("zest_cfi_dispatch");


	// also add a zest cfi "function" that's exported so dlsym can find it.
	auto zestcfi_str_pos=add_to_scoop(string("zestcfi")+'\0', dynstr_scoop);

	// add zestcfi symbol to binary
	T_Elf_Sym zestcfi_sym;
	memset(&zestcfi_sym,0,sizeof(T_Elf_Sym));
	zestcfi_sym.st_name=zestcfi_str_pos;
	zestcfi_sym.st_size=1234;
	zestcfi_sym.st_info=((STB_GLOBAL<<4)| (STT_FUNC));
	string zestcfi_sym_str((const char*)&zestcfi_sym, sizeof(T_Elf_Sym));
	unsigned int zestcfi_pos=add_to_scoop(zestcfi_sym_str,dynsym_scoop);

	// add "function" for zestcfi"
	// for now, return that the target is allowed.  the nonce plugin will have to have a slow path for this later.
	assert(firp->GetArchitectureBitWidth()==64); // fixme for 32-bit, should jmp to ecx.
	auto zestcfi_function_entry=addNewAssembly(firp,NULL,"jmp r11");

	// this jump can target any IBT in the module.
	ICFS_t *newicfs=new ICFS_t;
	for_each(firp->GetInstructions().begin(), firp->GetInstructions().end(), [&](Instruction_t* insn)
	{
		if(insn->GetIndirectBranchTargetAddress() != NULL )
			newicfs->insert(insn);
	});
	zestcfi_function_entry->SetIBTargets(newicfs);
	firp->GetAllICFS().insert(newicfs);
	firp->AssembleRegistry();
	

	// add a relocation so that the zest_cfi "function"  gets pointed to by the symbol
	Relocation_t* zestcfi_reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE,  zestcfi_pos+((uintptr_t)&zestcfi_sym.st_value - (uintptr_t)&zestcfi_sym), "data_to_insn_ptr", zestcfi_function_entry);
	dynsym_scoop->GetRelocations().insert(zestcfi_reloc);
	firp->GetRelocations().insert(zestcfi_reloc);


	// update strtabsz after got/etc entries are added.
	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->GetSize(); i+=sizeof(T_Elf_Dyn))
	{
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->GetContents().c_str()[i];
		if(dyn_entry.d_tag==DT_STRSZ)
		{
			dyn_entry.d_un.d_val=dynstr_scoop->GetContents().size();
		}
	}


	return true;
}
#endif

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
void ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::appendLibraryDepedencies(const string &libraryName)
{
	const auto firp=getFileIR(); // dynamic_cast<FileIR_t*>(getFileIR());

	auto dynamic_scoop=find_scoop(firp,".dynamic");
	auto dynstr_scoop=find_scoop(firp,".dynstr");

	// not dynamic executable w/o a .dynamic section.
	if(!dynamic_scoop)
		throw std::logic_error("Cannot change libraries in statically linked program");

	// may need to enable --step move_globals --step-option move_globals:--cfi
	if(dynamic_scoop->getStart()->getVirtualOffset()!=0)
	{
		cerr<<"Cannot find relocation-scoop pair:  Did you enable '--step move_globals --step-option move_globals:--cfi' ? "<<endl;
		exit(1);
	}

	const auto libld_str_pos=add_to_scoop(libraryName+'\0', dynstr_scoop);

	// a new dt_needed entry for libdl.so
	auto new_dynamic_entry=T_Elf_Dyn ({});
	new_dynamic_entry.d_tag=DT_NEEDED;
	new_dynamic_entry.d_un.d_val=libld_str_pos;
	const auto new_dynamic_entry_str=string((const char*)&new_dynamic_entry, sizeof(T_Elf_Dyn));

	// a null terminator
	const auto null_dynamic_entry=T_Elf_Dyn ({});
	const auto null_dynamic_entry_str=string((const char*)&null_dynamic_entry, sizeof(T_Elf_Dyn));

	// declare an entry for the .dynamic section and add it.
	auto index=0;
	while(1)
	{
		// assert we don't run off the end.
		assert((index+1)*sizeof(T_Elf_Dyn) <= dynamic_scoop->getContents().size());

		const auto dyn_ptr=(T_Elf_Dyn*) & dynamic_scoop->getContents().c_str()[index*sizeof(T_Elf_Dyn)];
	
		if(memcmp(dyn_ptr,&null_dynamic_entry,sizeof(T_Elf_Dyn)) == 0 )
		{
			cout<<"Inserting new DT_NEEDED at index "<<dec<<index<<endl;
			// found a null terminator entry.
			for(auto i=0U; i<sizeof(T_Elf_Dyn); i++)
			{
				// copy new_dynamic_entry ontop of null entry.
				auto str=dynamic_scoop->getContents();
				str[index*sizeof(T_Elf_Dyn) + i ] = ((char*)&new_dynamic_entry)[i];
				dynamic_scoop->setContents(str);
			}

			// check if there's room for the new null entry
			if((index+2)*sizeof(T_Elf_Dyn) <= dynamic_scoop->getContents().size())
			{
				/* yes */
				const auto next_entry=(T_Elf_Dyn*)&dynamic_scoop->getContents().c_str()[(index+1)*sizeof(T_Elf_Dyn)];
				// assert it's actually null 
				assert(memcmp(next_entry,&null_dynamic_entry,sizeof(T_Elf_Dyn)) == 0 );
			}
			else
			{
				// add to the scoop 
				add_to_scoop(null_dynamic_entry_str,dynamic_scoop);
			}
			break;
		}

		index++;
	}

	return;

}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
void ElfDependencies_t::ElfDependenciesImpl_t<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>::prependLibraryDepedencies(const string &libraryName)
{
	const auto firp=getFileIR();

	auto dynamic_scoop=find_scoop(firp,".dynamic");
	auto dynstr_scoop=find_scoop(firp,".dynstr");

	// not dynamic executable w/o a .dynamic section.
	if(!dynamic_scoop || !dynstr_scoop)
		throw std::logic_error("Cannot change libraries in statically linked program");

	// may need to enable --step move_globals --step-option move_globals:--cfi
	if(dynamic_scoop->getStart()->getVirtualOffset()!=0)
	{
		cerr<<"Cannot find relocation-scoop pair:  Did you enable '--step move_globals --step-option move_globals:--cfi' ? "<<endl;
		exit(1);
	}

	const auto libld_str_pos=add_to_scoop(libraryName+'\0', dynstr_scoop);

	// a new dt_needed entry for libdl.so
	auto new_dynamic_entry=T_Elf_Dyn ({});
	new_dynamic_entry.d_tag=DT_NEEDED;
	new_dynamic_entry.d_un.d_val=libld_str_pos;
	const auto new_dynamic_entry_str=string((const char*)&new_dynamic_entry, sizeof(T_Elf_Dyn));

	prefix_scoop<ptrsize>(new_dynamic_entry_str,  dynamic_scoop, firp) ;
}

unique_ptr<IRDB_SDK::ElfDependencies_t> IRDB_SDK::ElfDependencies_t::factory(IRDB_SDK::FileIR_t* firp)
{
	return unique_ptr<IRDB_SDK::ElfDependencies_t>(new libIRDB::ElfDependencies_t(firp));
}

