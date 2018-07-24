
#include "edt.hpp"
#include <libElfDep.hpp>
#include "Rewrite_Utility.hpp"
#include <algorithm> 

using namespace libTransform;
using namespace libIRDB;
using namespace std;
using namespace IRDBUtility;
using namespace ElfDep_Tester;


ElfDep_Tester_t::ElfDep_Tester_t(FileIR_t* firp)
	: Transform (NULL, firp, NULL)
{
}


#define ALLOF(a) begin(a),end(a)

	
int ElfDep_Tester_t::execute()
{
	// insert the PLT and GOT entries needed
	auto ed=ElfDependencies_t(getFileIR());
	(void)ed.appendLibraryDepedencies("libelf_dep_test.so");
	auto edpcb=ed.appendPltEntry("elf_dep_test_callback");
	auto edvar=ed.appendGotEntry("elf_dep_test_var");
	auto edvar_scoop=edvar.first;
	auto edvar_offset=edvar.second;


	// find the insertion point.
	const auto &all_funcs=getFileIR()->GetFunctions();
	const auto main_func_it=find_if(ALLOF(all_funcs), [&](const Function_t* f) { return f->GetName()=="main";});
	assert(main_func_it!=all_funcs.end());		
	const auto main_func=*main_func_it;
	auto insert_loc=main_func->GetEntryPoint();


	// insert the instrumentation
	auto tmp=insert_loc;
    	(void)insertAssemblyBefore(getFileIR(),tmp," push rdi") ;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," push rsi ") ;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," push rdx") ;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," push rcx ") ;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," push r8 ") ;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," push r9 ") ;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," call 0 ", edpcb) ;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," L1: mov rcx, [rel L1]");
	auto got_insn=tmp;
	tmp=  insertAssemblyAfter(getFileIR(), tmp," inc dword [rcx]");
	tmp=  insertAssemblyAfter(getFileIR(), tmp," call 0", edpcb);
	tmp=  insertAssemblyAfter(getFileIR(), tmp," pop r9");
	tmp=  insertAssemblyAfter(getFileIR(), tmp," pop r8");
	tmp=  insertAssemblyAfter(getFileIR(), tmp," pop rcx");
	tmp=  insertAssemblyAfter(getFileIR(), tmp," pop rdx");
	tmp=  insertAssemblyAfter(getFileIR(), tmp," pop rsi");
	tmp=  insertAssemblyAfter(getFileIR(), tmp," pop rdi");


	// map the load to point at the GOT entry.
	auto got_reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE, edvar_offset, "pcrel", edvar_scoop);
	getFileIR()->GetRelocations().insert(got_reloc);
	got_insn->GetRelocations().insert(got_reloc);


	return 0;
}
	


