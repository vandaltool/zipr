
#include <libIRDB-core.hpp>
#include <decode_base.hpp>
#include <decode_csarm.hpp>
#include <operand_base.hpp>
#include <operand_csarm.hpp>


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
static const auto ARM64_REG_PC=(arm64_reg)(ARM64_REG_ENDING+1);


DecodedInstructionCapstoneARM_t::CapstoneHandle_t* DecodedInstructionCapstoneARM_t::cs_handle=nullptr;

DecodedInstructionCapstoneARM_t::CapstoneHandle_t::CapstoneHandle_t(FileIR_t* firp)
{
	static_assert(sizeof(csh)==sizeof(handle), "Capstone handle size is unexpected.  Has CS changed?");

	const auto mode = static_cast<cs_mode>(CS_MODE_LITTLE_ENDIAN | ( firp->getArchitectureBitWidth() == 32 ? CS_MODE_V8 : 0));
	const auto arch = 
		firp->getArchitectureBitWidth() == 64 ? CS_ARCH_ARM64 : 
		firp->getArchitectureBitWidth() == 32 ? CS_ARCH_ARM   : 
		throw invalid_argument("Unknown bit width: "+to_string(firp->getArchitectureBitWidth()));
	const auto err  = cs_open(arch, mode,  (csh*)&handle);
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

static bool isArm64Jmp(cs_insn* the_insn) 
{
	return isPartOfGroup(the_insn,ARM64_GRP_JUMP);
}

template<class type>
static inline type arm64InsnToImmedHelper(cs_insn* the_insn, csh handle)
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
	const auto cs_freer=[](cs_insn * insn) -> void 
		{  
			cs_free(insn,1); 
		} ; 
	my_insn.reset(insn,cs_freer);
	if(!ok)
	{
		insn->size=0;
		return;
	}

        auto &op0 = (insn->detail->arm64.operands[0]); // might change these.
        auto &op1 = (insn->detail->arm64.operands[1]);

	const auto mnemonic=string(insn->mnemonic);
	const auto is_ldr_type = mnemonic=="ldr"  || mnemonic=="ldrsw" ;
	if(is_ldr_type && op1.type==ARM64_OP_IMM)
	{
		// no, this is a pcrel load.
		const auto imm=op1.imm;
		op1.type=ARM64_OP_MEM;
		op1.mem.base=ARM64_REG_PC;
		op1.mem.index=ARM64_REG_INVALID;
		op1.mem.disp=imm;
	}
	if(mnemonic=="prfm" && op0.type==ARM64_OP_IMM)
	{
		// no, this is a pcrel load.
		const auto imm=op0.imm;
		op0.type=ARM64_OP_MEM;
		op0.mem.base=ARM64_REG_PC;
		op0.mem.index=ARM64_REG_INVALID;
		op0.mem.disp=imm;
	}


}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const Instruction_t* i)
{
	if(!i) throw std::invalid_argument("No instruction given to DecodedInstruction_t(Instruction_t*)");

        const auto length=i->getDataBits().size();
	const auto &databits=i->getDataBits();
	const auto data=databits.data();
	const auto address=i->getAddress()->getVirtualOffset();
        Disassemble(address,data,length);

	if(!valid()) throw std::invalid_argument("The Instruction_t::getDataBits field is not a valid instruction.");
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
        Disassemble(start_addr, data, max_len);
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
        const auto length=(char*)endptr-(char*)data;
        Disassemble(start_addr,data,length);
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const DecodedInstructionCapstoneARM64_t& copy)
{
	*this=copy;
}

DecodedInstructionCapstoneARM64_t::DecodedInstructionCapstoneARM64_t(const shared_ptr<void> &p_my_insn)
{
	my_insn = p_my_insn;
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
	return isCall() || isReturn() || isArm64Jmp(the_insn);
}

bool DecodedInstructionCapstoneARM64_t::isCall() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto mnemonic=getMnemonic();
	return mnemonic=="bl" || mnemonic=="blr";
}

bool DecodedInstructionCapstoneARM64_t::isUnconditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	return isArm64Jmp(the_insn) && 
		(getMnemonic()=="b" || getMnemonic()=="br");
}

bool DecodedInstructionCapstoneARM64_t::isConditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isArm64Jmp(the_insn) && !isUnconditionalBranch();
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
shared_ptr<DecodedOperand_t> DecodedInstructionCapstoneARM64_t::getOperand(const int op_num) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	if(!hasOperand(op_num)) throw std::logic_error(string("Called ")+__FUNCTION__+" on without hasOperand()==true");

	return shared_ptr<DecodedOperand_t>(new DecodedOperandCapstoneARM64_t(my_insn,(uint8_t)op_num));
	
}

DecodedOperandVector_t DecodedInstructionCapstoneARM64_t::getOperands() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &arm = (the_insn->detail->arm64);
	const auto opcount=arm.op_count;

	auto ret_val=DecodedOperandVector_t();
	
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
	if(isCall() || isArm64Jmp(the_insn))
		return 0;

	return arm64InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
}


virtual_offset_t DecodedInstructionCapstoneARM64_t::getAddress() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	if(isCall() || isArm64Jmp(the_insn))
	{
		const auto mnemonic=getMnemonic();
		if( mnemonic=="tbnz" || mnemonic=="tbz")
			return getOperand(2)->getConstant();
		return arm64InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
	}
	assert(0);

}


bool DecodedInstructionCapstoneARM64_t::setsStackPointer() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	if(!hasOperand(0)) return false;

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &arm = (the_insn->detail->arm64);
	return (arm.operands[0].type==ARM64_OP_REG && arm.operands[0].reg==ARM64_REG_SP);
}

uint32_t DecodedInstructionCapstoneARM64_t::getPrefixCount() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return 0;
}

IRDB_SDK::VirtualOffset_t DecodedInstructionCapstoneARM64_t::getMemoryDisplacementOffset(const IRDB_SDK::DecodedOperand_t* t, const IRDB_SDK::Instruction_t* insn) const
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

