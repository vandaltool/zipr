
#include <libIRDB-core.hpp>
#include <decode_base.hpp>
#include <decode_csmips.hpp>
#include <operand_base.hpp>
#include <operand_csmips.hpp>


#include <capstone.h>
#include <mips.h>
#include <string>
#include <functional>
#include <set>
#include <algorithm>
#include <stdexcept>

using namespace libIRDB;
using namespace std;

#define ALLOF(a) begin(a),end(a)

static bool isPartOfGroup(const cs_insn* the_insn, const mips_insn_group the_grp) 
{
	const auto grp_it=find(ALLOF(the_insn->detail->groups), the_grp);
	return grp_it!=end(the_insn->detail->groups);
}

template<class type>
static inline type mips32InsnToImmedHelper(cs_insn* the_insn, csh handle)
{
        const auto count = cs_op_count(handle, the_insn, ARM_OP_IMM);
        const auto mips = &(the_insn->detail->mips);

        if (count==0)
                return 0;        /* no immediate is the same as an immediate of 0, i guess? */
        else if (count==1)
        {
                const auto index = cs_op_index(handle, the_insn, MIPS_OP_IMM, 1);
                return mips->operands[index].imm;
        }
        else
                throw std::logic_error(string("Called ")+__FUNCTION__+" with number of immedaites not equal 1");
}


// shared code 
// constructors, destructors, operators.

void DecodedInstructionCapstoneMIPS32_t::Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len)
{
	using namespace std::placeholders;
	auto insn=cs_malloc(cs_handle->getHandle());
	auto address=(uint64_t)start_addr;
	auto size=(size_t)max_len;
	const uint8_t* code=(uint8_t*)data;
	const auto ok = cs_disasm_iter(cs_handle->getHandle(), &code, &size, &address, insn);
	if(!ok)
		insn->size=0;

	const auto cs_freer=[](cs_insn * insn) -> void 
		{  
			cs_free(insn,1); 
		} ; 
	my_insn.reset(insn,cs_freer);
}

DecodedInstructionCapstoneMIPS32_t::DecodedInstructionCapstoneMIPS32_t(const Instruction_t* i)
{
	if(!i) throw std::invalid_argument("No instruction given to DecodedInstruction_t(Instruction_t*)");

        const auto length=i->getDataBits().size();
	const auto &databits=i->getDataBits();
	const auto data=databits.data();
	const auto address=i->getAddress()->getVirtualOffset();
        Disassemble(address,data,length);

	if(!valid()) throw std::invalid_argument("The Instruction_t::getDataBits field is not a valid instruction.");
}

DecodedInstructionCapstoneMIPS32_t::DecodedInstructionCapstoneMIPS32_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
        Disassemble(start_addr, data, max_len);
}

DecodedInstructionCapstoneMIPS32_t::DecodedInstructionCapstoneMIPS32_t(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
        const auto length=(char*)endptr-(char*)data;
        Disassemble(start_addr,data,length);
}

DecodedInstructionCapstoneMIPS32_t::DecodedInstructionCapstoneMIPS32_t(const DecodedInstructionCapstoneMIPS32_t& copy)
{
	*this=copy;
}

DecodedInstructionCapstoneMIPS32_t::DecodedInstructionCapstoneMIPS32_t(const shared_ptr<void> &p_my_insn)
{
	my_insn = p_my_insn;
}

DecodedInstructionCapstoneMIPS32_t::~DecodedInstructionCapstoneMIPS32_t()
{
	// no need to cs_free(my_insn) because shared pointer will do that for us!
}

DecodedInstructionCapstoneMIPS32_t& DecodedInstructionCapstoneMIPS32_t::operator=(const DecodedInstructionCapstoneMIPS32_t& copy)
{
	my_insn=copy.my_insn;
	return *this;
}

// public methods

string DecodedInstructionCapstoneMIPS32_t::getDisassembly() const
{

	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	auto full_str=getMnemonic()+" "+the_insn->op_str;

	return full_str;
}

bool DecodedInstructionCapstoneMIPS32_t::valid() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size!=0;
}

uint32_t DecodedInstructionCapstoneMIPS32_t::length() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size;
}

bool DecodedInstructionCapstoneMIPS32_t::isBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isCall() || isReturn() || isPartOfGroup(the_insn,MIPS_GRP_JUMP);
}

bool DecodedInstructionCapstoneMIPS32_t::isCall() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	assert(the_insn);

	const auto marked_call = isPartOfGroup(the_insn, MIPS_GRP_CALL);

	const auto unmarked_call  = isPartOfGroup(the_insn, MIPS_GRP_BRANCH_RELATIVE) &&
		( 
		 getMnemonic() == "bal"  || 
		 getMnemonic() == "balr" || 
		 getMnemonic() == "jal"  || 
		 getMnemonic() == "jalr" 
		);

	return marked_call || unmarked_call;

}

bool DecodedInstructionCapstoneMIPS32_t::isUnconditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
 	return isPartOfGroup(the_insn, MIPS_GRP_JUMP) && getMnemonic()=="b";
}

bool DecodedInstructionCapstoneMIPS32_t::isConditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	assert(the_insn);
 	return isPartOfGroup(the_insn, MIPS_GRP_JUMP)  && !isUnconditionalBranch();
}

bool DecodedInstructionCapstoneMIPS32_t::isReturn() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn  = static_cast<cs_insn*>(my_insn.get());
	assert(the_insn);
	return isPartOfGroup(the_insn, MIPS_GRP_RET);
}

bool DecodedInstructionCapstoneMIPS32_t::hasOperand(const int op_num) const
{
	if(op_num<0) throw std::logic_error(string("Called ")+ __FUNCTION__+" with opnum="+to_string(op_num));
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &mips = (the_insn->detail->mips);
	return 0 <= op_num  && op_num < mips.op_count;
}

// 0-based.  first operand is numbered 0.
shared_ptr<DecodedOperand_t> DecodedInstructionCapstoneMIPS32_t::getOperand(const int op_num) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	if(!hasOperand(op_num)) throw std::logic_error(string("Called ")+__FUNCTION__+" on without hasOperand()==true");

	return shared_ptr<DecodedOperand_t>(new DecodedOperandCapstoneMIPS32_t(my_insn,(uint8_t)op_num));
	
}

DecodedOperandVector_t DecodedInstructionCapstoneMIPS32_t::getOperands() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &mips = (the_insn->detail->mips);
	const auto opcount=mips.op_count;

	auto ret_val=DecodedOperandVector_t();
	
	for(auto i=0;i<opcount;i++)
		ret_val.push_back(getOperand(i));

	return ret_val;
}

string DecodedInstructionCapstoneMIPS32_t::getMnemonic() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	// return the new string.
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->mnemonic;

	
}


int64_t DecodedInstructionCapstoneMIPS32_t::getImmediate() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	// direct calls and jumps have a "immediate" which is really an address"  those are returned by getAddress
	if(isCall() || isPartOfGroup(the_insn, MIPS_GRP_JUMP) )
		return 0;

	return mips32InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
}


virtual_offset_t DecodedInstructionCapstoneMIPS32_t::getAddress() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	if(isCall() || isPartOfGroup(the_insn, MIPS_GRP_JUMP) )
	{
		const auto mnemonic=getMnemonic();
		if( mnemonic=="tbnz" || mnemonic=="tbz")
			return getOperand(2)->getConstant();
		return mips32InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
	}
	return 0;

}


bool DecodedInstructionCapstoneMIPS32_t::setsStackPointer() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	if(!hasOperand(0)) return false;

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &mips = (the_insn->detail->mips);
	return (mips.operands[0].type==MIPS_OP_REG && mips.operands[0].reg==MIPS_REG_SP);
}

uint32_t DecodedInstructionCapstoneMIPS32_t::getPrefixCount() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return 0;
}

IRDB_SDK::VirtualOffset_t DecodedInstructionCapstoneMIPS32_t::getMemoryDisplacementOffset(const IRDB_SDK::DecodedOperand_t* t, const IRDB_SDK::Instruction_t* insn) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	assert(0);
}

bool DecodedInstructionCapstoneMIPS32_t::hasRelevantRepPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS32_t::hasRelevantRepnePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS32_t::hasRelevantOperandSizePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS32_t::hasRexWPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS32_t::hasImplicitlyModifiedRegs() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

