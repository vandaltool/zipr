#include "Rewrite_Utility.hpp"
using namespace std;
using namespace libIRDB;

map<string, set<Instruction_t*> > inserted_instr; //used to undo inserted instructions
map<string, set<AddressID_t*> > inserted_addr; //used to undo inserted addresses

void setExitCode(FileIR_t* virp, Instruction_t* exit_code);

//For all insertBefore functions:
//The "first" instruction will have its contents replaced and a duplicate of "first" will be in the follow of first. 
//This duplicate is returned since the user already has a pointer to first.
//To insert before an instruction is the same as modifying the original instruction, and inserting after it
//a copy of the original instruction 
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
    Instruction_t* next = copyInstruction(virp,first);
	virp->ChangeRegistryKey(first,next);
    setInstructionAssembly(virp,first,assembly,next,target);

    return next;
}

Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly)
{
    return insertAssemblyBefore(virp,first,assembly,NULL);
}

Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits)
{
    return insertDataBitsBefore(virp,first,dataBits,NULL);
}

Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
    Instruction_t* next = copyInstruction(virp,first);
    setInstructionDataBits(virp,first,dataBits,next,target);

    return next;    
}

Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
    Instruction_t *new_instr = allocateNewInstruction(virp,first);
    setInstructionAssembly(virp,new_instr,assembly,first->GetFallthrough(), target);
    first->SetFallthrough(new_instr);
    return new_instr;
}

Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly)
{
    return insertAssemblyAfter(virp,first,assembly,NULL);

}

Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
    Instruction_t *new_instr = allocateNewInstruction(virp,first);
    setInstructionDataBits(virp,new_instr,dataBits,first->GetFallthrough(), target);
    first->SetFallthrough(new_instr);

    return new_instr;
}

Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits)
{
    return insertDataBitsAfter(virp,first,dataBits,NULL);
}

//Does not insert into any variant
Instruction_t* copyInstruction(Instruction_t* instr)
{
    Instruction_t* cpy = new Instruction_t();

    copyInstruction(instr,cpy);

    return cpy;
}

Instruction_t* copyInstruction(FileIR_t* virp, Instruction_t* instr)
{
    Instruction_t* cpy = allocateNewInstruction(virp,instr);

    cpy->SetDataBits(instr->GetDataBits());
    cpy->SetComment(instr->GetComment());
    cpy->SetCallback(instr->GetCallback());
    cpy->SetFallthrough(instr->GetFallthrough());
    cpy->SetTarget(instr->GetTarget());

	//In case the fallthrough is null, generate spri has to have a 
	//place to jump, which is determined by the original address. 
	cpy->SetOriginalAddressID(instr->GetOriginalAddressID());

    return cpy;
}

void copyInstruction(Instruction_t* src, Instruction_t* dest)
{
    dest->SetDataBits(src->GetDataBits());
    dest->SetComment(src->GetComment());
    dest->SetCallback(src->GetCallback());
    dest->SetFallthrough(src->GetFallthrough());
    dest->SetTarget(src->GetTarget());
}

Instruction_t* allocateNewInstruction(FileIR_t* virp, db_id_t p_fileID,Function_t* func)
{
	Instruction_t *instr = new Instruction_t();
	AddressID_t *a =new AddressID_t();

	a->SetFileID(p_fileID);

	instr->SetFunction(func);
	instr->SetAddress(a);

	virp->GetInstructions().insert(instr);
	virp->GetAddresses().insert(a);

	string name = "1_null_func_dummy_1";

	if(func != NULL)
		name = func->GetName();

	inserted_instr[name].insert(instr);
	inserted_addr[name].insert(a);
	
	return instr;
}

Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr)
{
    Function_t *func = template_instr->GetFunction();
    db_id_t fileID = template_instr->GetAddress()->GetFileID();
    return allocateNewInstruction(virp, fileID, func);
}

void setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, string p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
    if (p_instr == NULL) return;
    
    ///TODO: what if bad assembly?
	virp->RegisterAssembly(p_instr,p_assembly);

//    p_instr->Assemble(p_assembly);
    p_instr->SetComment(p_instr->getDisassembly());
    p_instr->SetFallthrough(p_fallThrough); 
    p_instr->SetTarget(p_target); 
    
    virp->GetAddresses().insert(p_instr->GetAddress());
    virp->GetInstructions().insert(p_instr);
}

void setInstructionDataBits(FileIR_t* virp, Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
    if (p_instr == NULL) return;
    
    p_instr->SetDataBits(p_dataBits);
    p_instr->SetComment(p_instr->getDisassembly());
    p_instr->SetFallthrough(p_fallThrough); 
    p_instr->SetTarget(p_target); 
    
    virp->GetAddresses().insert(p_instr->GetAddress());
    virp->GetInstructions().insert(p_instr);
}

// jns - jump not signed
string getJnsDataBits()
{
    string dataBits;
    dataBits.resize(2);
    dataBits[0] = 0x79;
    dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later
    return dataBits;
}

// jz - jump zero
string  getJzDataBits()
{
    string dataBits;
    dataBits.resize(2);
    dataBits[0] = 0x74;
    dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

    return dataBits;
}

// jnz - jump not zero
string getJnzDataBits()
{
    string dataBits;
    dataBits.resize(2);
    dataBits[0] = 0x75;
    dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

    return dataBits;    
}

Instruction_t* getHandlerCode(FileIR_t* virp, Instruction_t* fallthrough, mitigation_policy policy)
{
    Instruction_t *handler_code = allocateNewInstruction(virp,fallthrough);
/*
    setInstructionAssembly(virp,exit_code,"mov eax, 1",exit_code,NULL);
    Instruction_t* mov_ebx = insertAssemblyAfter(virp,exit_code,"mov ebx, 666");
    insertAssemblyAfter(virp,mov_ebx,"int 0x80");
*/
    setInstructionAssembly(virp,handler_code,"pusha",NULL,NULL);
    Instruction_t* pushf = insertAssemblyAfter(virp,handler_code,"pushf",NULL);
    stringstream ss;
    ss<<"push dword 0x";
    ss<<hex<<policy;
    Instruction_t *policy_push = insertAssemblyAfter(virp,pushf,ss.str(),NULL);
    ss.str("");
    ss<<"push dword 0x";
    ss<<hex<<fallthrough->GetAddress()->GetVirtualOffset();
    Instruction_t *addr_push = insertAssemblyAfter(virp,policy_push,ss.str(),NULL);
    //I am not planning on returning, but pass the address at which the overflow was detected.
    Instruction_t *ret_push = insertAssemblyAfter(virp,addr_push,ss.str(),NULL);
    Instruction_t *callback = insertAssemblyAfter(virp,ret_push,"nop",NULL);

    callback->SetCallback("buffer_overflow_detector");
    

    Instruction_t *popf = insertAssemblyAfter(virp,callback,"popf",NULL);
    Instruction_t *popa = insertAssemblyAfter(virp,popf,"popa",NULL);
    popa->SetFallthrough(fallthrough);
    
    return handler_code;
}

Instruction_t* insertCanaryCheckBefore(FileIR_t* virp,Instruction_t *first, unsigned int canary_val, int esp_offset, Instruction_t *fail_code)
{
    stringstream ss;

    ss<<"cmp dword [esp";

    if(esp_offset <0)
    {
	ss<<"-";
	esp_offset = esp_offset*-1;
    }
    else
	ss<<"+";

    ss<<"0x"<<hex<<esp_offset<<"], 0x"<<hex<<canary_val;

    Instruction_t* cpy = copyInstruction(virp,first);
    setInstructionAssembly(virp,first,ss.str(),cpy,NULL);
    insertDataBitsAfter(virp,first,getJnzDataBits(),fail_code);
    first->SetComment("Canary Check: "+first->GetComment());

    return cpy;
}
