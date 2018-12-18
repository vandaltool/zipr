
#include <libIRDB-core.hpp>

#include <capstone.h>
#include <arm64.h>
#include <string>
#include <functional>
#include <set>
#include <algorithm>
#include <stdexcept>

using namespace libIRDB;
using namespace std;

#define ALLOF(a) begin(a),end(a)

DecodedInstructionCapstoneARM64_t::CapstoneHandle_t* DecodedInstructionCapstoneARM64_t::cs_handle=NULL ;

DecodedInstructionCapstoneARM64_t::CapstoneHandle_t::CapstoneHandle_t(FileIR_t* firp)
{
	const auto mode = CS_MODE_LITTLE_ENDIAN;
	static_assert(sizeof(csh)==sizeof(handle), "Capstone handle size is unexpected.  Has CS changed?");
	auto err = cs_open(CS_ARCH_ARM64, mode,  (csh*)&handle);

	if (err)
	{
		const auto s=string("Failed on cs_open() with error returned: ")+to_string(err)+"\n";
		throw std::runtime_error(s);
	}
	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
	cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);


}




static bool isPartOfGroup(const cs_insn* the_insn, const arm64_insn_group the_grp) 
{
	const auto grp_it=find(ALLOF(the_insn->detail->groups), the_grp);
	return grp_it!=end(the_insn->detail->groups);

}

static bool isJmp(cs_insn* the_insn) 
{
	return isPartOfGroup(the_insn,ARM64_GRP_JUMP);
}


template<class type>
static inline type insnToImmedHelper(cs_insn* the_insn, csh handle)
{
        const auto count = cs_op_count(handle, the_insn, ARM64_OP_IMM);
	const auto arm = &(the_insn->detail->arm64);

        if (count==0) 
		return 0;	 /* no immediate is the same as an immediate of 0, i guess? */
        else if (count==1) 
	{
		const auto index = cs_op_index(handle, the_insn, ARM64_OP_IMM, 1);
		return arm->operands[index].imm;
	}
	else
		throw std::logic_error(string("Called ")+__FUNCTION__+" with number of immedaites not equal 1");
}



// shared code 
// constructors, destructors, operators.

void DecodedInstructionCapstoneARM64_t::Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len)
{
	using namespace std::placeholders;
	auto insn=cs_malloc(cs_handle->getHandle());
	auto address=(uint64_t)start_addr;
	auto size=(size_t)max_len;
	const uint8_t* code=(uint8_t*)data;
	const auto ok = cs_disasm_iter(cs_handle->getHandle(), &code, &size, &address, insn);
	if(!ok)
		insn->size=0;


	const auto mnemonic=string(insn->mnemonic);

	const auto cs_freer=[](cs_insn * insn) -> void 
		{  
			cs_free(insn,1); 
		} ; 
	my_insn.reset(insn,cs_freer);
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const Instruction_t* i)
{
	if(!cs_handle) cs_handle=new CapstoneHandle_t(NULL);
	if(!i) throw std::invalid_argument("No instruction given to DecodedInstruction_t(Instruction_t*)");

        const auto length=i->GetDataBits().size();
	const auto &databits=i->GetDataBits();
	const auto data=databits.data();
	const auto address=i->GetAddress()->GetVirtualOffset();
        Disassemble(address,data,length);

	if(!valid()) throw std::invalid_argument("The Instruction_t::GetDataBits field is not a valid instruction.");
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
	if(!cs_handle) cs_handle=new CapstoneHandle_t(NULL);
        Disassemble(start_addr, data, max_len);
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
	if(!cs_handle) cs_handle=new CapstoneHandle_t(NULL);
        const auto length=(char*)endptr-(char*)data;
        Disassemble(start_addr,data,length);
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const DecodedInstructionCapstoneARM64_t& copy)
{
	*this=copy;
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const shared_ptr<void> &p_my_insn)
	: my_insn(p_my_insn)
{
}

DecodedInstructionCapstoneARM64_t::~DecodedInstructionCapstoneARM64_t()
{
	// no need to cs_free(my_insn) because shared pointer will do that for us!
}

DecodedInstructionCapstoneARM64_t& DecodedInstructionCapstoneARM64_t::operator=(const DecodedInstructionCapstoneARM64_t& copy)
{
	my_insn=copy.my_insn;
	return *this;
}

// public methods

string DecodedInstructionCapstoneARM64_t::getDisassembly() const
{

	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	auto full_str=getMnemonic()+" "+the_insn->op_str;

	return full_str;
}

bool DecodedInstructionCapstoneARM64_t::valid() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size!=0;
}

uint32_t DecodedInstructionCapstoneARM64_t::length() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size;
}

bool DecodedInstructionCapstoneARM64_t::isBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isCall() || isReturn() || isJmp(the_insn);
}

bool DecodedInstructionCapstoneARM64_t::isCall() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isPartOfGroup(the_insn,ARM64_GRP_CALL);
}

bool DecodedInstructionCapstoneARM64_t::isUnconditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	return isJmp(the_insn) && !isConditionalBranch();
}

bool DecodedInstructionCapstoneARM64_t::isConditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isJmp(the_insn) && getMnemonic()!="br";
}

bool DecodedInstructionCapstoneARM64_t::isReturn() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isPartOfGroup(the_insn,ARM64_GRP_RET);
}

bool DecodedInstructionCapstoneARM64_t::hasOperand(const int op_num) const
{
	if(op_num<0) throw std::logic_error(string("Called ")+ __FUNCTION__+" with opnum="+to_string(op_num));
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &arm = (the_insn->detail->arm64);
	return 0 <= op_num  && op_num < arm.op_count;
}

// 0-based.  first operand is numbered 0.
shared_ptr<DecodedOperandCapstone_t> DecodedInstructionCapstoneARM64_t::getOperand(const int op_num) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	if(!hasOperand(op_num)) throw std::logic_error(string("Called ")+__FUNCTION__+" on without hasOperand()==true");

	return shared_ptr<DecodedOperandCapstone_t>(new DecodedOperandCapstoneARM64_t(my_insn,(uint8_t)op_num));
	
}

DecodedOperandCapstoneVector_t DecodedInstructionCapstoneARM64_t::getOperands() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &arm = (the_insn->detail->arm64);
	const auto opcount=arm.op_count;

	auto ret_val=DecodedOperandCapstoneVector_t();
	
	for(auto i=0;i<opcount;i++)
		ret_val.push_back(getOperand(i));

	return ret_val;
}

string DecodedInstructionCapstoneARM64_t::getMnemonic() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	// return the new string.
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->mnemonic;

	
}


int64_t DecodedInstructionCapstoneARM64_t::getImmediate() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	// direct calls and jumps have a "immediate" which is really an address"  those are returned by getAddress
	if(isCall() || isJmp(the_insn))
		return 0;

	return insnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
}


virtual_offset_t DecodedInstructionCapstoneARM64_t::getAddress() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	if(isCall() || isJmp(the_insn))
		return insnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
	assert(0);

}


bool DecodedInstructionCapstoneARM64_t::setsStackPointer() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	assert (0);
}

uint32_t DecodedInstructionCapstoneARM64_t::getPrefixCount() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return 0;
}

virtual_offset_t DecodedInstructionCapstoneARM64_t::getMemoryDisplacementOffset(const DecodedOperandCapstone_t& t, const Instruction_t* insn) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	assert(0);
}



bool DecodedInstructionCapstoneARM64_t::hasRelevantRepPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM64_t::hasRelevantRepnePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM64_t::hasRelevantOperandSizePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM64_t::hasRexWPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneARM64_t::hasImplicitlyModifiedRegs() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

