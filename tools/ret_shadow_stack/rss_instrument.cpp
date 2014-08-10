
#include "rss_instrument.hpp"
#include "MEDS_SafeFuncAnnotation.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>



using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;


static void create_tls_reloc(FileIR_t* firp, Instruction_t* insn)
{
        Relocation_t* reloc=new Relocation_t;
	reloc->SetOffset(0);
	reloc->SetType("tls_ss_start");
        insn->GetRelocations().insert(reloc);
        firp->GetRelocations().insert(reloc);
}


static 	bool add_rss_push(FileIR_t* firp, Instruction_t* insn)
{

	if(getenv("RSS_VERBOSE")!=NULL)
	{
		cout<<"Adding push instrumentation at 0x"<<std::hex<<insn->GetAddress()->GetVirtualOffset()<<endl;
	}

	/* this moves insn to a new instructiona fter insn, and then overwrites insn */
	insertAssemblyBefore(firp,insn,"push rax");

	/* now we insert after that insn */
	Instruction_t* tmp=insertAssemblyAfter(firp,insn,"push rcx");
	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [rsp+16]");	// load return address 
	tmp=insertAssemblyAfter(firp,tmp,"mov rax, [fs:0x12345678] ");
	create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"mov [rax], rcx");
	tmp=insertAssemblyAfter(firp,tmp,"lea rax, [rax+8]");
	tmp=insertAssemblyAfter(firp,tmp,"mov [fs:0x12345678], rax ");
	create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"pop rcx");
	tmp=insertAssemblyAfter(firp,tmp,"pop rax");
	

	return true;

	
}


static 	bool add_rss_pop(FileIR_t* firp, Instruction_t* insn)
{
	Instruction_t *jmp_insn=NULL, *ret_to_app=NULL, *tmp=NULL;

	if(getenv("RSS_VERBOSE")!=NULL)
	{
		cout<<"Adding push instrumentation at 0x"<<std::hex<<insn->GetAddress()->GetVirtualOffset()<<endl;
	}

	/* this moves insn to a new instructiona fter insn, and then overwrites insn */
	insertAssemblyBefore(firp,insn,"push rcx");

	/* now we insert after that insn */
	tmp=insertAssemblyAfter(firp,insn,"pushfq");
	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [fs:0x12345678] "); create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"lea rcx, [rcx-8]");
	tmp=insertAssemblyAfter(firp,tmp,"mov [fs:0x12345678], rcx "); create_tls_reloc(firp,tmp);
	tmp=insertAssemblyAfter(firp,tmp,"mov rcx, [rcx]");
	tmp=insertAssemblyAfter(firp,tmp,"sub rcx, [rsp+16]");
	jmp_insn=tmp=insertDataBitsAfter(firp,tmp,getJecxzDataBits()); // jecxz L1
	tmp=insertAssemblyAfter(firp,tmp,"hlt");
/*L1*/	ret_to_app=
	tmp=insertAssemblyAfter(firp,tmp,"popfq");
	tmp=insertAssemblyAfter(firp,tmp,"pop rcx");

	/* link jump instruction to restore code */
	jmp_insn->SetTarget(ret_to_app);
	
	return true;
}

static bool is_exit_instruction(Instruction_t *insn)
{
	DISASM d;
	insn->Disassemble(d);
	if(strstr(d.CompleteInstr,"ret")!=0)
		return true;
	return false;	
}

static bool add_rss_instrumentation(FileIR_t* firp, Function_t* func)
{
	bool success=true;
	if(func->GetEntryPoint()==NULL)
		return false;

	success&=add_rss_push(firp, func->GetEntryPoint());

	for(
		set<Instruction_t*>::iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		if(is_exit_instruction(insn))
			success&=add_rss_pop(firp, insn);
	}

	return success;
}


static bool is_safe_func(Function_t* func, MEDS_AnnotationParser* meds_ap)
{
	assert(meds_ap);
	if(!func->GetEntryPoint())
		return false;

	/* grab vo from IRDB */
	virtual_offset_t irdb_vo = func->GetEntryPoint()->GetAddress()->GetVirtualOffset();

	/* no original address for this function?  already instrumented and getting data from MEDS is a bad idea? */
	if (irdb_vo == 0) 
		return false;

	VirtualOffset vo(irdb_vo);

	std::pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret;

	/* find it in the annotations */
	ret = meds_ap->getAnnotations().equal_range(vo);
	MEDS_SafeFuncAnnotation annotation;
	MEDS_SafeFuncAnnotation* p_annotation;

	/* for each annotation for this instruction */
	for (MEDS_Annotations_t::iterator it = ret.first; it != ret.second; ++it)
	{
			/* is this annotation a funcSafe annotation? */
                        p_annotation=dynamic_cast<MEDS_SafeFuncAnnotation*>(&it->second);
                        if(p_annotation==NULL)
                                continue;
			annotation = *p_annotation;

			/* bad annotation? */
			if(!annotation.isValid())
				continue;

			/* that marks the function safe? */
			if(annotation.isSafe())
				return true;
	}

	/* couldn't find the func marked as safe */
	return false;
		
}

bool RSS_Instrument::execute()
{

	bool success=false;

	for(set<Function_t*>::iterator it=firp->GetFunctions().begin(); 
		it!=firp->GetFunctions().end(); 
		++it
	   )
	{
		Function_t* func=*it;
		if(!is_safe_func(func,meds_ap))
		{
			cout<<"Function "<<func->GetName()<<" is not safe!\n";
			success|=add_rss_instrumentation(firp,func);
		}
		else
		{
			cout<<"Function "<<func->GetName()<<" is safe!\n";
		}
		
	}

	/* return an exit code */
	if(success)
		return 0; /* success? */
	
	return 1;
}


