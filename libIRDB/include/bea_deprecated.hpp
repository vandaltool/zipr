#ifndef bea_deprecated_h
#define bea_deprecated_h


#include <libIRDB-core.hpp>
#include <beaengine/BeaEngine.h>


static inline int Disassemble(const libIRDB::Instruction_t* const insn, DISASM &disasm)
{
	assert(insn);
        memset(&disasm, 0, sizeof(DISASM));
        disasm.Options = NasmSyntax + PrefixedNumeral;
        disasm.Archi = libIRDB::FileIR_t::GetArchitectureBitWidth();
        disasm.EIP = (UIntPtr) insn->GetDataBits().c_str();
        disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();
        int instr_len = Disasm(&disasm);
        return instr_len;  
} 

static inline bool SetsStackPointer(const ARGTYPE* arg)
{
        if((arg->AccessMode & WRITE ) == 0)
                return false;
        int access_type=arg->ArgType;

        if(access_type==REGISTER_TYPE + GENERAL_REG +REG4)
                return true;
        return false;
        
}

static inline bool SetsStackPointer(const DISASM* disasm)
{
        if(strstr(disasm->Instruction.Mnemonic, "push")!=NULL)
                return true;
        if(strstr(disasm->Instruction.Mnemonic, "pop")!=NULL)
                return true;
        if(strstr(disasm->Instruction.Mnemonic, "call")!=NULL)
                return true;
        if(disasm->Instruction.ImplicitModifiedRegs==REGISTER_TYPE+GENERAL_REG+REG4)
                return true;

        if(SetsStackPointer(&disasm->Argument1)) return true;
        if(SetsStackPointer(&disasm->Argument2)) return true;
        if(SetsStackPointer(&disasm->Argument3)) return true;

        return false;

}

#endif
