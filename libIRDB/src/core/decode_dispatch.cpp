
#include <libIRDB-core.hpp>
#include <core/decode_base.hpp>
#include <core/decode_csx86.hpp>
#include <core/decode_csarm.hpp>

#include <core/operand_base.hpp>
#include <core/operand_csx86.hpp>
#include <core/operand_csarm.hpp>

#include <core/decode_dispatch.hpp>
#include <core/operand_dispatch.hpp>


using namespace libIRDB;
using namespace std;

// constructors, destructors, operators.

DecodedInstructionDispatcher_t::DecodedInstructionDispatcher_t(const Instruction_t* i) 
{
	cs=DecodedInstructionCapstone_t::factory(i);
}

DecodedInstructionDispatcher_t::DecodedInstructionDispatcher_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len) 
{
	cs=DecodedInstructionCapstone_t::factory(start_addr,data,max_len);
}

DecodedInstructionDispatcher_t::DecodedInstructionDispatcher_t(const virtual_offset_t start_addr, const void *data, const void* endptr) 
{
	cs=DecodedInstructionCapstone_t::factory(start_addr,data,endptr);
}

DecodedInstructionDispatcher_t::DecodedInstructionDispatcher_t(const DecodedInstructionDispatcher_t& copy) 
{
	cs=copy.cs; 
}

DecodedInstructionDispatcher_t::~DecodedInstructionDispatcher_t()
{
}

DecodedInstructionDispatcher_t& DecodedInstructionDispatcher_t::operator=(const DecodedInstructionDispatcher_t& copy)
{
	cs=copy.cs; 
	return *this;
}



#define passthrough_to_cs(ret_type, method_name) 						\
	ret_type DecodedInstructionDispatcher_t::method_name() const 				\
	{ 											\
		return cs->method_name(); 							\
	} 

// public methods

/* demonstrating things are the same */
passthrough_to_cs(bool, valid);
passthrough_to_cs(uint32_t, length);
passthrough_to_cs(bool, isBranch);
passthrough_to_cs(bool, isCall);
passthrough_to_cs(bool, isUnconditionalBranch);
passthrough_to_cs(bool, isConditionalBranch);
passthrough_to_cs(bool, isReturn);
passthrough_to_cs(uint32_t, getPrefixCount);
passthrough_to_cs(bool, hasRexWPrefix);
passthrough_to_cs(bool, setsStackPointer);
passthrough_to_cs(bool, hasRelevantRepPrefix);		// rep ret disagreement.
passthrough_to_cs(bool, hasRelevantRepnePrefix);
passthrough_to_cs(bool, hasRelevantOperandSizePrefix);

/* demonstrating when capstone is better */
passthrough_to_cs(string, getMnemonic);
passthrough_to_cs(string,getDisassembly);
passthrough_to_cs(virtual_offset_t, getAddress);	 
passthrough_to_cs(int64_t, getImmediate);

// demonstrating they different and trying to determine which is better.
passthrough_to_cs(bool, hasImplicitlyModifiedRegs); 	

// not yet completed
// needs more complicated impl.


bool DecodedInstructionDispatcher_t::hasOperand(const int op_num) const
{
	const auto cs_res = cs->hasOperand(op_num);
	return cs_res; 									
}

// 0-based.  first operand is numbered 0.
unique_ptr<DecodedOperand_t> DecodedInstructionDispatcher_t::getOperand(const int op_num) const
{
	return unique_ptr<DecodedOperand_t>(new DecodedOperandDispatcher_t(cs->getOperand(op_num)));
}

DecodedOperandMetaVector_t DecodedInstructionDispatcher_t::getOperands() const
{
	auto ret_val=DecodedOperandMetaVector_t();
	auto cs_operands=cs->getOperands();
	for(const auto &op : cs_operands ) 
		ret_val.push_back(DecodedOperandDispatcher_t(op));
	return ret_val;
}

virtual_offset_t DecodedInstructionDispatcher_t::getMemoryDisplacementOffset(const DecodedOperandDispatcher_t& t, const Instruction_t* insn) const
{
	return cs->getMemoryDisplacementOffset(*t.cs, insn);
}



