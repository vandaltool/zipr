
#include "edt.hpp"
#include <libElfDep.hpp>
#include <algorithm> 

using namespace IRDB_SDK;
using namespace std;
using namespace ElfDep_Tester;


ElfDep_Tester_t::ElfDep_Tester_t(FileIR_t* firp)
	: Transform (firp)
{
}


#define ALLOF(a) begin(a),end(a)

	
int ElfDep_Tester_t::execute()
{
	// insert the PLT and GOT entries needed
	auto ed=libIRDB::ElfDependencies_t(getFileIR());
	(void)ed.appendLibraryDepedencies("libelf_dep_test.so");
	auto edpcb=ed.appendPltEntry("elf_dep_test_callback");
	auto edvar=ed.appendGotEntry("elf_dep_test_var");
	auto edvar_scoop=edvar.first;
	auto edvar_offset=edvar.second;


	// find the insertion point.
	const auto &all_funcs=getFileIR()->getFunctions();
	const auto main_func_it=find_if(ALLOF(all_funcs), [&](const Function_t* f) { return f->getName()=="main";});
	assert(main_func_it!=all_funcs.end());		
	const auto main_func=*main_func_it;
	auto insert_loc=main_func->getEntryPoint();


	// insert the instrumentation
	auto tmp=insert_loc;
    	(void)insertAssemblyBefore(tmp," push rdi") ;
	tmp=  insertAssemblyAfter(tmp," push rsi ") ;
	tmp=  insertAssemblyAfter(tmp," push rdx") ;
	tmp=  insertAssemblyAfter(tmp," push rcx ") ;
	tmp=  insertAssemblyAfter(tmp," push r8 ") ;
	tmp=  insertAssemblyAfter(tmp," push r9 ") ;
	tmp=  insertAssemblyAfter(tmp," call 0 ", edpcb) ;
	tmp=  insertAssemblyAfter(tmp," L1: mov rcx, [rel L1]");
	auto got_insn=tmp;
	tmp=  insertAssemblyAfter(tmp," inc dword [rcx]");
	tmp=  insertAssemblyAfter(tmp," call 0", edpcb);
	tmp=  insertAssemblyAfter(tmp," pop r9");
	tmp=  insertAssemblyAfter(tmp," pop r8");
	tmp=  insertAssemblyAfter(tmp," pop rcx");
	tmp=  insertAssemblyAfter(tmp," pop rdx");
	tmp=  insertAssemblyAfter(tmp," pop rsi");
	tmp=  insertAssemblyAfter(tmp," pop rdi");


	// map the load to point at the GOT entry.
	auto got_reloc=getFileIR()->addNewRelocation(got_insn, edvar_offset, "pcrel", edvar_scoop);
	(void)got_reloc; // just give to IR

	return 0;
}
	


