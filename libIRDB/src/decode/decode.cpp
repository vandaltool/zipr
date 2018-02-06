
#include <libIRDB-decode.hpp>

#include <bea_deprecated.hpp>

using namespace libIRDB;
using namespace std;

// static functions
static inline bool my_SetsStackPointer(const ARGTYPE* arg)
{
        if((arg->AccessMode & WRITE ) == 0)
                return false;
        int access_type=arg->ArgType;

        if(access_type==REGISTER_TYPE + GENERAL_REG +REG4)
                return true;
        return false;

}

// constructors, destructors, operators.

DecodedInstruction_t::DecodedInstruction_t(const Instruction_t* i)
{
	disasm_data=static_cast<void*>(new DISASM({}));
	DISASM* d=(DISASM*)disasm_data;
	disasm_length=::Disassemble(i,*d);
}

DecodedInstruction_t::DecodedInstruction_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
	disasm_data=static_cast<void*>(new DISASM({}));
	const auto endptr=data+max_len;
	Disassemble(start_addr, data, max_len);
}

DecodedInstruction_t::DecodedInstruction_t(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
	disasm_data=static_cast<void*>(new DISASM({}));
	const auto length=(char*)endptr-(char*)data;
	Disassemble(start_addr,data,length);
}

DecodedInstruction_t::DecodedInstruction_t(const DecodedInstruction_t& copy)
{
	DISASM* d=static_cast<DISASM*>(copy.disasm_data);
	disasm_data=static_cast<void*>(new DISASM(*d));
	disasm_length=copy.disasm_length;
}

DecodedInstruction_t::~DecodedInstruction_t()
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	delete d;
	disasm_data=NULL;
}

DecodedInstruction_t& DecodedInstruction_t::operator=(const DecodedInstruction_t& copy)
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	delete d;
	disasm_data=NULL;

	d=static_cast<DISASM*>(copy.disasm_data);
	disasm_data=static_cast<void*>(new DISASM(*d));
	disasm_length=copy.disasm_length;
	return *this;
}

// private methods

void DecodedInstruction_t::Disassemble(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	memset(d, 0, sizeof(DISASM));
	d->Options = NasmSyntax + PrefixedNumeral;
	d->Archi = FileIR_t::GetArchitectureBitWidth();
	d->EIP = (UIntPtr)data;
	d->SecurityBlock=max_len;
	d->VirtualAddr = start_addr;
	disasm_length=Disasm(d);

}

// public methods

string DecodedInstruction_t::getDisassembly() const
{
	assert(valid());
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return string(d->CompleteInstr);
}

bool DecodedInstruction_t::valid() const
{
	return disasm_length>=0;
}

uint32_t DecodedInstruction_t::length() const
{
	assert(valid());
	return disasm_length;
}


bool DecodedInstruction_t::isBranch() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return d->Instruction.BranchType!=0;
}

bool DecodedInstruction_t::isCall() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return d->Instruction.BranchType==CallType;
}

bool DecodedInstruction_t::isUnconditionalBranch() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return d->Instruction.BranchType==JmpType;
}

bool DecodedInstruction_t::isConditionalBranch() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	assert(0);
}

bool DecodedInstruction_t::isReturn() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return d->Instruction.BranchType==RetType;
}


bool DecodedInstruction_t::hasOperand(const int op_num) const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	switch(op_num+1)
	{
		case 1:
			return d->Argument1.ArgType!=NO_ARGUMENT;
		case 2:
			return d->Argument2.ArgType!=NO_ARGUMENT;
		case 3:
			return d->Argument3.ArgType!=NO_ARGUMENT;
		case 4:
			return d->Argument4.ArgType!=NO_ARGUMENT;
		default:
			return false;
	}
}


// 0-based.  first operand is numbered 0.
DecodedOperand_t DecodedInstruction_t::getOperand(const int op_num) const
{
	if(!hasOperand(op_num))
		throw std::out_of_range("op_num");

	DISASM* d=static_cast<DISASM*>(disasm_data);
	switch(op_num+1)
	{
		case 1:
			return DecodedOperand_t(&d->Argument1);
		case 2:
			return DecodedOperand_t(&d->Argument2);
		case 3:
			return DecodedOperand_t(&d->Argument3);
		case 4:
			return DecodedOperand_t(&d->Argument4);
	}
}

vector<DecodedOperand_t> DecodedInstruction_t::getOperands() const
{
	auto ret_val=vector<DecodedOperand_t>();
	for(auto i=0;i<4;i++)
	{
		if(hasOperand(i))
			ret_val.push_back(getOperand(i));
		else
			break;
	}
	return ret_val;
}


string DecodedInstruction_t::getMnemonic() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	const auto mnemonic=string(d->Instruction.Mnemonic);
	return mnemonic.substr(0,mnemonic.size()-1);
}

virtual_offset_t DecodedInstruction_t::getImmediate() const
{
	// find and return an immediate operand from this instruction
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return d->Instruction.Immediat;
}


virtual_offset_t DecodedInstruction_t::getAddress() const
{
	// return anything that's explicitly an address, like a jmp/call target 
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return d->Instruction.AddrValue;
}


bool DecodedInstruction_t::setsStackPointer() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);

        if(strstr(d->Instruction.Mnemonic, "push")!=NULL)
                return true;
        if(strstr(d->Instruction.Mnemonic, "pop")!=NULL)
                return true;
        if(strstr(d->Instruction.Mnemonic, "call")!=NULL)
                return true;
        if(d->Instruction.ImplicitModifiedRegs==REGISTER_TYPE+GENERAL_REG+REG4)
                return true;

        if(my_SetsStackPointer(&d->Argument1)) return true;
        if(my_SetsStackPointer(&d->Argument2)) return true;
        if(my_SetsStackPointer(&d->Argument3)) return true;
        if(my_SetsStackPointer(&d->Argument4)) return true;

        return false;
}

uint32_t DecodedInstruction_t::getPrefixCount() const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	return d->Prefix.Number;
}

virtual_offset_t DecodedInstruction_t::getMemoryDisplacementOffset(const DecodedOperand_t& t) const
{
	DISASM* d=static_cast<DISASM*>(disasm_data);
	ARGTYPE* the_arg=static_cast<ARGTYPE*>(t.arg_data);
	return the_arg->Memory.DisplacementAddr-d->EIP;
}

