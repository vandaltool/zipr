
#include <libIRDB-core.hpp>
#include <decode_base.hpp>
#include <decode_csmips.hpp>
#include <operand_base.hpp>
#include <operand_csmips.hpp>


#include <capstone.h>
#include <mips64.h>
#include <string>
#include <functional>
#include <set>
#include <algorithm>
#include <stdexcept>

using namespace libIRDB;
using namespace std;

#define ALLOF(a) begin(a),end(a)
static const auto MIPS64_REG_PC=(mips64_reg)(MIPS64_REG_ENDING+1);


DecodedInstructionCapstoneMIPS_t::CapstoneHandle_t* DecodedInstructionCapstoneMIPS_t::cs_handle=nullptr;

DecodedInstructionCapstoneMIPS_t::CapstoneHandle_t::CapstoneHandle_t(FileIR_t* firp)
{
	static_assert(sizeof(csh)==sizeof(handle), "Capstone handle size is unexpected.  Has CS changed?");

	const auto mode = CS_MODE_LITTLE_ENDIAN;
	const auto arch = 
		firp->getArchitectureBitWidth() == 64 ? CS_ARCH_MIPS64 : 
		firp->getArchitectureBitWidth() == 32 ? CS_ARCH_MIPS   : 
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

static bool isPartOfGroup(const cs_insn* the_insn, const mips64_insn_group the_grp) 
{
	const auto grp_it=find(ALLOF(the_insn->detail->groups), the_grp);
	return grp_it!=end(the_insn->detail->groups);
}

static bool isArm64Jmp(cs_insn* the_insn) 
{
	return isPartOfGroup(the_insn,MIPS64_GRP_JUMP);
}

template<class type>
static inline type mips64InsnToImmedHelper(cs_insn* the_insn, csh handle)
{
        const auto count = cs_op_count(handle, the_insn, MIPS64_OP_IMM);
	const auto mips = &(the_insn->detail->mips64);

        if (count==0) 
		return 0;	 /* no immediate is the same as an immediate of 0, i guess? */
        else if (count==1) 
	{
		const auto index = cs_op_index(handle, the_insn, MIPS64_OP_IMM, 1);
		return mips->operands[index].imm;
	}
	else
		throw std::logic_error(string("Called ")+__FUNCTION__+" with number of immedaites not equal 1");
}


// shared code 
// constructors, destructors, operators.

void DecodedInstructionCapstoneMIPS64_t::Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len)
{
	using namespace std::placeholders;
	auto insn=cs_malloc(cs_handle->getHandle());
	auto address=(uint64_t)start_addr;
	auto size=(size_t)max_len;
	const uint8_t* code=(uint8_t*)data;
	const auto ok = cs_disasm_iter(cs_handle->getHandle(), &code, &size, &address, insn);
	if(!ok)
		insn->size=0;

        auto &op0 = (insn->detail->mips64.operands[0]); // might change these.
        auto &op1 = (insn->detail->mips64.operands[1]);

	const auto mnemonic=string(insn->mnemonic);
	const auto is_ldr_type = mnemonic=="ldr"  || mnemonic=="ldrsw" ;
	if(is_ldr_type && op1.type==MIPS64_OP_IMM)
	{
		// no, this is a pcrel load.
		const auto imm=op1.imm;
		op1.type=MIPS64_OP_MEM;
		op1.mem.base=MIPS64_REG_PC;
		op1.mem.index=MIPS64_REG_INVALID;
		op1.mem.disp=imm;
	}
	if(mnemonic=="prfm" && op0.type==MIPS64_OP_IMM)
	{
		// no, this is a pcrel load.
		const auto imm=op0.imm;
		op0.type=MIPS64_OP_MEM;
		op0.mem.base=MIPS64_REG_PC;
		op0.mem.index=MIPS64_REG_INVALID;
		op0.mem.disp=imm;
	}


	const auto cs_freer=[](cs_insn * insn) -> void 
		{  
			cs_free(insn,1); 
		} ; 
	my_insn.reset(insn,cs_freer);
}

DecodedInstructionCapstoneMIPS64_t::DecodedInstructionCapstoneMIPS64_t(const Instruction_t* i)
{
	if(!i) throw std::invalid_argument("No instruction given to DecodedInstruction_t(Instruction_t*)");

        const auto length=i->getDataBits().size();
	const auto &databits=i->getDataBits();
	const auto data=databits.data();
	const auto address=i->getAddress()->getVirtualOffset();
        Disassemble(address,data,length);

	if(!valid()) throw std::invalid_argument("The Instruction_t::getDataBits field is not a valid instruction.");
}

DecodedInstructionCapstoneMIPS64_t::DecodedInstructionCapstoneMIPS64_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
        Disassemble(start_addr, data, max_len);
}

DecodedInstructionCapstoneMIPS64_t::DecodedInstructionCapstoneMIPS64_t(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
        const auto length=(char*)endptr-(char*)data;
        Disassemble(start_addr,data,length);
}

DecodedInstructionCapstoneMIPS64_t::DecodedInstructionCapstoneMIPS64_t(const DecodedInstructionCapstoneMIPS64_t& copy)
{
	*this=copy;
}

DecodedInstructionCapstoneMIPS64_t::DecodedInstructionCapstoneMIPS64_t(const shared_ptr<void> &p_my_insn)
{
	my_insn = p_my_insn;
}

DecodedInstructionCapstoneMIPS64_t::~DecodedInstructionCapstoneMIPS64_t()
{
	// no need to cs_free(my_insn) because shared pointer will do that for us!
}

DecodedInstructionCapstoneMIPS64_t& DecodedInstructionCapstoneMIPS64_t::operator=(const DecodedInstructionCapstoneMIPS64_t& copy)
{
	my_insn=copy.my_insn;
	return *this;
}

// public methods

string DecodedInstructionCapstoneMIPS64_t::getDisassembly() const
{

	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	auto full_str=getMnemonic()+" "+the_insn->op_str;

	return full_str;
}

bool DecodedInstructionCapstoneMIPS64_t::valid() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size!=0;
}

uint32_t DecodedInstructionCapstoneMIPS64_t::length() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->size;
}

bool DecodedInstructionCapstoneMIPS64_t::isBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isCall() || isReturn() || isArm64Jmp(the_insn);
}

bool DecodedInstructionCapstoneMIPS64_t::isCall() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto mnemonic=getMnemonic();
	return mnemonic=="bl" || mnemonic=="blr";
}

bool DecodedInstructionCapstoneMIPS64_t::isUnconditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	return isArm64Jmp(the_insn) && 
		(getMnemonic()=="b" || getMnemonic()=="br");
}

bool DecodedInstructionCapstoneMIPS64_t::isConditionalBranch() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isArm64Jmp(the_insn) && !isUnconditionalBranch();
}

bool DecodedInstructionCapstoneMIPS64_t::isReturn() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return isPartOfGroup(the_insn,MIPS64_GRP_RET);
}

bool DecodedInstructionCapstoneMIPS64_t::hasOperand(const int op_num) const
{
	if(op_num<0) throw std::logic_error(string("Called ")+ __FUNCTION__+" with opnum="+to_string(op_num));
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &mips = (the_insn->detail->mips64);
	return 0 <= op_num  && op_num < mips.op_count;
}

// 0-based.  first operand is numbered 0.
shared_ptr<DecodedOperand_t> DecodedInstructionCapstoneMIPS64_t::getOperand(const int op_num) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	if(!hasOperand(op_num)) throw std::logic_error(string("Called ")+__FUNCTION__+" on without hasOperand()==true");

	return shared_ptr<DecodedOperand_t>(new DecodedOperandCapstoneMIPS64_t(my_insn,(uint8_t)op_num));
	
}

DecodedOperandVector_t DecodedInstructionCapstoneMIPS64_t::getOperands() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &mips = (the_insn->detail->mips64);
	const auto opcount=mips.op_count;

	auto ret_val=DecodedOperandVector_t();
	
	for(auto i=0;i<opcount;i++)
		ret_val.push_back(getOperand(i));

	return ret_val;
}

string DecodedInstructionCapstoneMIPS64_t::getMnemonic() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	// return the new string.
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	return the_insn->mnemonic;

	
}


int64_t DecodedInstructionCapstoneMIPS64_t::getImmediate() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());

	// direct calls and jumps have a "immediate" which is really an address"  those are returned by getAddress
	if(isCall() || isArm64Jmp(the_insn))
		return 0;

	return mips64InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
}


virtual_offset_t DecodedInstructionCapstoneMIPS64_t::getAddress() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	if(isCall() || isArm64Jmp(the_insn))
	{
		const auto mnemonic=getMnemonic();
		if( mnemonic=="tbnz" || mnemonic=="tbz")
			return getOperand(2)->getConstant();
		return mips64InsnToImmedHelper<int64_t>(the_insn, cs_handle->getHandle());
	}
	assert(0);

}


bool DecodedInstructionCapstoneMIPS64_t::setsStackPointer() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");

	if(!hasOperand(0)) return false;

	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
	const auto &mips = (the_insn->detail->mips64);
	return (mips.operands[0].type==MIPS64_OP_REG && mips.operands[0].reg==MIPS64_REG_SP);
}

uint32_t DecodedInstructionCapstoneMIPS64_t::getPrefixCount() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return 0;
}

IRDB_SDK::VirtualOffset_t DecodedInstructionCapstoneMIPS64_t::getMemoryDisplacementOffset(const IRDB_SDK::DecodedOperand_t* t, const IRDB_SDK::Instruction_t* insn) const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	assert(0);
}

bool DecodedInstructionCapstoneMIPS64_t::hasRelevantRepPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS64_t::hasRelevantRepnePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS64_t::hasRelevantOperandSizePrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS64_t::hasRexWPrefix() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

bool DecodedInstructionCapstoneMIPS64_t::hasImplicitlyModifiedRegs() const
{
	if(!valid()) throw std::logic_error(string("Called ")+__FUNCTION__+" on invalid instruction");
	return false;
}

