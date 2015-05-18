#include "utils.hpp"
#include "rigrandom_instr.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>
#include <sstream>

using namespace std;
using namespace libIRDB;

static Instruction_t* addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm)
{
	Instruction_t* newinstr;
	if (p_instr)
		newinstr = allocateNewInstruction(firp,p_instr->GetAddress()->GetFileID(), p_instr->GetFunction());
	else   
		newinstr = allocateNewInstruction(firp,BaseObj_t::NOT_IN_DATABASE, NULL);

	firp->RegisterAssembly(newinstr, p_asm);

	if (p_instr)
	{
		newinstr->SetFallthrough(p_instr->GetFallthrough());
		p_instr->SetFallthrough(newinstr);
	}

	return newinstr;
}

Instruction_t* RigRandom_Instrument::insertRandom(Instruction_t* after) 
{
/*
   d:	85 d2                	test   %ecx,%ecx
J1:	7e 11                	jle    L4
  11:	b8 00 00 00 00       	mov    $0x0,%eax
L3:	88 04 01             	mov    [%ebx+%eax*1], 0x41 (or passed-in value)
  19:	83 c0 01             	add    $0x1,%eax
  1c:	39 d0                	cmp    %ecx,%eax
J2:	75 f6                	jne    L3
  20:	eb 05                	jmp    L2
L4:	ba 00 00 00 00       	mov    $0x0,%ecx
L2:	85 db                	test   %edx,%edx
J3:	74 02                	je     L1
  2b:	89 13                	mov    %ecx,(%edx)
L1:	b8 00 00 00 00       	mov    $0x0,%eax
*/

	Instruction_t *J1=NULL, *J2=NULL, *J3=NULL, *L1=NULL, *L2=NULL, *L3=NULL, *L4=NULL;

	after=insertAssemblyAfter(firp, after, "test ecx, ecx");
	J1=after=insertAssemblyAfter(firp, after, "jle 0x0");
	after=insertAssemblyAfter(firp, after, "mov eax, 0");

	// user-selected random sequence
	stringstream ss;
	ss << "mov [ebx+eax], byte 0x" << std::hex << (int) random_start << std::dec;
	L3=after=insertAssemblyAfter(firp, after, ss.str().c_str());

	after=insertAssemblyAfter(firp, after, "add eax, 1");
	after=insertAssemblyAfter(firp, after, "cmp eax, ecx");
	J2=after=insertAssemblyAfter(firp, after, "jne 0x0");

	L4=after=insertAssemblyAfter(firp, after, "mov ecx, 0");
	L2=after=insertAssemblyAfter(firp, after, "test edx, edx");
	J3=after=insertAssemblyAfter(firp, after, "je 0x0");
	after=insertAssemblyAfter(firp, after, "mov [edx], ecx");
	L1=after=insertAssemblyAfter(firp, after, "mov eax, 0");

	J1->SetTarget(L4);
	J2->SetTarget(L3);
	J2->SetFallthrough(L2);
	J3->SetTarget(L1);


	return after;
}

bool RigRandom_Instrument::add_rr_instrumentation(libIRDB::Instruction_t* insn)
{

	assert(insn);
	cout<<"Adding CGC->Elf instrumentation for "<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;

	Instruction_t* tmp=insn;
	Instruction_t* randomjmp=NULL, *randominsn=NULL;
	Instruction_t* old=insn;
	Instruction_t* failinsn=NULL;

	old=insertAssemblyBefore(firp,tmp,"cmp eax, 7"); // terminate
	randominsn=tmp;
	randomjmp=tmp=insertAssemblyAfter(firp,tmp,"jne 0");
		tmp=insertRandom(tmp);
		tmp->SetFallthrough(old);
	failinsn=tmp=addNewAssembly(firp,NULL,"mov eax, 13"); // fail
	failinsn->SetFallthrough(old);

	randomjmp->SetTarget(failinsn);
	
	// nop
	string bits;  
	bits.resize(1); 
	bits[0]=0x90;
	old->SetDataBits(bits);
}

bool RigRandom_Instrument::needs_rr_instrumentation(libIRDB::Instruction_t* insn)
{
	// instrument int instructions 
	DISASM d;
	insn->Disassemble(d);
	return strstr(d.CompleteInstr,"int")!=0;
}

bool RigRandom_Instrument::instrument_ints()
{
	bool success=true;

	// only instrument syscall to random
	bool eax_7=false;
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it)
	{
		Instruction_t* insn=*it;
		DISASM d;
		insn->Disassemble(d);
		if (strstr(d.Instruction.Mnemonic,"mov") != 0 && 
		    strstr(d.Argument1.ArgMnemonic, "eax") != 0 &&
		    strstr(d.Argument2.ArgMnemonic, "00000007") != 0)
			eax_7 = true;
		if (eax_7 && strstr(d.CompleteInstr,"int") != 0)
		{
			success = success && add_rr_instrumentation(insn);
			eax_7 = false;
		}
			
	}

	return success;
}



bool RigRandom_Instrument::execute()
{
	bool success=true;

	success = success && instrument_ints();

	return success;
}
