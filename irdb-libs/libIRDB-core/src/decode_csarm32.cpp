
#include <libIRDB-core.hpp>
#include <decode_base.hpp>
#include <decode_csarm.hpp>
#include <operand_base.hpp>
#include <operand_csarm.hpp>


#include <capstone.h>
#include <arm.h>
#include <string>
#include <functional>
#include <set>
#include <algorithm>
#include <stdexcept>

using namespace libIRDB;
using namespace std;

#define ALLOF(a) begin(a),end(a)

static bool isPartOfGroup(const cs_insn* the_insn, const arm_insn_group the_grp) 
{
	const auto grp_it=find(ALLOF(the_insn->detail->groups), the_grp);
	return grp_it!=end(the_insn->detail->groups);
}

static bool isArm32Jmp(cs_insn* the_insn) 
{
	return isPartOfGroup(the_insn,ARM_GRP_JUMP);
}


template<class type>
static inline type arm32InsnToImmedHelper(cs_insn* the_insn, csh handle)
{
        const auto count = cs_op_count(handle, the_insn, ARM_OP_IMM);
	const auto arm = &(the_insn->detail->arm);

        if (count==0) 
		return 0;	 /* no immediate is the same as an immediate of 0, i guess? */
        else if (count==1) 
	{
		const auto index = cs_op_index(handle, the_insn, ARM_OP_IMM, 1);
		return arm->operands[index].imm;
	}
	else
		throw std::logic_error(string("Called ")+__FUNCTION__+" with number of immedaites not equal 1");
}



// shared code 
// constructors, destructors, operators.

void DecodedInstructionCapstoneARM32_t::Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len)
{
	using namespace std::placeholders;
	auto insn=cs_malloc(cs_handle->getHandle());
	auto address=(uint64_t)start_addr;
	auto size=(size_t)max_len;
	const uint8_t* code=(uint8_t*)data;
	const auto ok = cs_disasm_iter(cs_handle->getHandle(), &code, &size, &address, insn);
	if(!ok)
		insn->size=0;

        auto &op0 = (insn->detail->arm.operands[0]); // might change these.
        auto &op1 = (insn->detail->arm.operands[1]);

	const auto mnemonic=string(insn->mnemonic);
	const auto is_ldr_type = mnemonic=="ldr"  || mnemonic=="ldrsw" ;
	if(is_ldr_type && op1.type==ARM_OP_IMM)
	{
		// no, this is a pcrel load.
		const auto imm=op1.imm;
		op1.type=ARM_OP_MEM;
		op1.mem.base=ARM_REG_PC;
		op1.mem.index=ARM_REG_INVALID;
		op1.mem.disp=imm;
	}
	if(mnemonic=="prfm" && op0.type==ARM_OP_IMM)
	{
		// no, this is a pcrel load.
		const auto imm=op0.imm;
		op0.type=ARM_OP_MEM;
		op0.mem.base=ARM_REG_PC;
		op0.mem.index=ARM_REG_INVALID;
		op0.mem.disp=imm;
	}


	const auto cs_freer=[](cs_insn * insn) -> void 
		{  
			cs_free(insn,1); 
		} ; 
	my_insn.reset(insn,cs_freer);
}

DecodedInstructionCapstoneARM32_t::DecodedInstructionCapstoneARM32_t(const Instruction_t* i)
{
	if(!i) throw std::invalid_argument("No instruction given to DecodedInstruction_t(Instruction_t*)");

        const auto length=i->getDataBits().size();
	const auto &databits=i->getDataBits();
	const auto data=databits.data();
	const auto address=i->getAddress()->getVirtualOffset();
        Disassemble(address,data,length);

	if(!valid()) throw std::invalid_argument("The Instruction_t::getDataBits field is not a valid instruction.");
}

DecodedInstructionCapstoneARM32_t::DecodedInstructionCapstoneARM32_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
        Disassemble(start_addr, data, max_len);
}

DecodedInstructionCapstoneARM32_t::DecodedInstructionCapstoneARM32_t(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
        const auto length=(char*)endptr-(char*)data;
        Disassemble(start_addr,data,length);
}

DecodedInstructionCapstoneARM32_t::DecodedInstructionCapstoneARM32_t(const DecodedInstructionCapstoneARM32_t& copy)
{
	*this=copy;
}

DecodedInstructionCapstoneARM32_t::DecodedInstructionCapstoneARM32_t(const shared_ptr<void> &p_my_insn)
{
	my_insn = p_my_insn;
}

DecodedInstructionCapstoneARM32_t::~DecodedInstructionCapstoneARM32_t()
{
	// no need to cs_free(my_insn) because shared pointer will do that for us!
}

DecodedInstructionCapstoneARM32_t& DecodedInstructionCapstoneARM32_t::operator=(const DecodedInstructionCapstoneARM32_t& copy)
{
	my_insn=copy.my_insn;
	return *this;
}

// public methods

string DecodedInstructionCapstoneARM32_t::getDisassembly() const
{

	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	auto full_str=getMnemonic()+" "+the_insn->op_str;

	return full_str;
}

bool DecodedInstructionCapstoneARM32_t::valid() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size!=0;
}

uint32_t DecodedInstructionCapstoneARM32_t::length() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size;
}

bool DecodedInstructionCapstoneARM32_t::isBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isCall() || isReturn() || isArm32Jmp(the_insn);
}

bool DecodedInstructionCapstoneARM32_t::isCall() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto mnemonic=getMnemonic();
	return mnemonic=="bl" || mnemonic=="blr";
}

bool DecodedInstructionCapstoneARM32_t::isUnconditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	return isArm32Jmp(the_insn) && 
		(getMnemonic()=="b" || getMnemonic()=="br");
}

bool DecodedInstructionCapstoneARM32_t::isConditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isArm32Jmp(the_insn) && !isUnconditionalBranch();
}

bool DecodedInstructionCapstoneARM32_t::isReturn() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn  = static_cast<cs_insn*>(my_insn.get());
	assert(the_insn);
	const auto &arm      = the_insn->detail->arm;
	const auto is_pop_pc = getMnemonic()=="pop" && arm.operands[arm.op_count-1].reg==ARM_REG_PC;

	return is_pop_pc; // || other things?
}

bool DecodedInstructionCapstoneARM32_t::hasOperand(const int op_num) const
{
	if(op_num<0) throw std::logic_error(string("Called ")+ __FUNCTION__+" with opnum="+to_string(op_num));
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &arm = (the_insn->detail->arm);
	return 0 <= op_num  && op_num < arm.op_count;
}

// 0-based.  first operand is numbered 0.
shared_ptr<DecodedOperand_t> DecodedInstructionCapstoneARM32_t::getOperand(const int op_num) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	if(!hasOperand(op_num)) throw std::logic_error(string("Called ")+__FUNCTION__+" on without hasOperand()==true");

	return shared_ptr<DecodedOperand_t>(new DecodedOperandCapstoneARM32_t(my_insn,(uint8_t)op_num));
	
}

DecodedOperandVector_t DecodedInstructionCapstoneARM32_t::getOperands() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &arm = (the_insn->detail->arm);
	const auto opcount=arm.op_count;

	auto ret_val=DecodedOperandVector_t();
	
	for(auto i=0;i<opcount;i++)
		ret_val.push_back(getOperand(i));

	return ret_val;
}

string DecodedInstructionCapstoneARM32_t::getMnemonic() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	// return the new string.
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->mnemonic;

	
}


int64_t DecodedInstructionCapstoneARM32_t::getImmediate() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	// direct calls and jumps have a "immediate" which is really an address"  those are returned by getAddress
	if(isCall() || isArm32Jmp(the_insn))
		return 0;

	return arm32InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
}


virtual_offset_t DecodedInstructionCapstoneARM32_t::getAddress() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	if(isCall() || isArm32Jmp(the_insn))
	{
		const auto mnemonic=getMnemonic();
		if( mnemonic=="tbnz" || mnemonic=="tbz")
			return getOperand(2)->getConstant();
		return arm32InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
	}
	assert(0);

}


bool DecodedInstructionCapstoneARM32_t::setsStackPointer() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	if(!hasOperand(0)) return false;

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &arm = (the_insn->detail->arm);
	return (arm.operands[0].type==ARM_OP_REG && arm.operands[0].reg==ARM_REG_SP);
}

uint32_t DecodedInstructionCapstoneARM32_t::getPrefixCount() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return 0;
}

IRDB_SDK::VirtualOffset_t DecodedInstructionCapstoneARM32_t::getMemoryDisplacementOffset(const IRDB_SDK::DecodedOperand_t* t, const IRDB_SDK::Instruction_t* insn) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	assert(0);
}

bool DecodedInstructionCapstoneARM32_t::hasRelevantRepPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM32_t::hasRelevantRepnePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM32_t::hasRelevantOperandSizePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM32_t::hasRexWPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM32_t::hasImplicitlyModifiedRegs() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

