
#include <libIRDB-decode.hpp>

using namespace libIRDB;
using namespace std;

// constructors, destructors, operators.

DecodedInstructionMeta_t::DecodedInstructionMeta_t(const Instruction_t* i) :
	bea(i),
	cs(i)
{
}

DecodedInstructionMeta_t::DecodedInstructionMeta_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len) :
	bea(start_addr,data,max_len),
	cs(start_addr,data,max_len)
{
}

DecodedInstructionMeta_t::DecodedInstructionMeta_t(const virtual_offset_t start_addr, const void *data, const void* endptr) :
	bea(start_addr,data,endptr),
	cs(start_addr,data,endptr)
{
}

DecodedInstructionMeta_t::DecodedInstructionMeta_t(const DecodedInstructionMeta_t& copy) :
	bea(copy.bea),
	cs(copy.cs)
{
}

DecodedInstructionMeta_t::~DecodedInstructionMeta_t()
{
}

DecodedInstructionMeta_t& DecodedInstructionMeta_t::operator=(const DecodedInstructionMeta_t& copy)
{
	bea=copy.bea;
	cs=copy.cs;
}



#define passthrough_to_bea(ret_type, method_name) 						\
	ret_type DecodedInstructionMeta_t::method_name() const 					\
	{ 											\
		return bea.method_name(); 							\
	} 

#define compare_decoders(ret_type, method_name) \
	ret_type DecodedInstructionMeta_t::method_name() const 					\
	{ 											\
		const auto bea_res= bea.method_name(); 						\
		const auto cs_res= cs.method_name(); 						\
		if(bea_res!=cs_res)  								\
		{ 										\
			cerr<<"Bea/Capstone miscompare: bea='"<<bea_res<<"'"			\
			                             <<" cs='"<<cs_res <<"'";	 		\
			cerr<<"In "<<bea.getDisassembly()<<" at " <<__FUNCTION__<<endl;		\
		} 										\
		return cs_res; 									\
	} 

#define compare_decoders_assert(ret_type, method_name) \
	ret_type DecodedInstructionMeta_t::method_name() const 					\
	{ 											\
		const auto bea_res= bea.method_name(); 						\
		const auto cs_res= cs.method_name(); 						\
		if(bea_res!=cs_res)  								\
		{ 										\
			cerr<<"Bea/Capstone miscompare: bea='"<<bea_res<<"'"			\
			                             <<" cs='"<<cs_res <<"'"<<endl; 		\
			cerr<<"In "<<bea.getDisassembly()<<" at " <<__FUNCTION__<<endl;		\
			abort(); 								\
		} 										\
		return cs_res; 									\
	} 


#define passthrough_to_cs(ret_type, method_name) 						\
	ret_type DecodedInstructionMeta_t::method_name() const 					\
	{ 											\
		return cs.method_name(); 							\
	} 

// public methods

passthrough_to_cs(string,getDisassembly);
compare_decoders_assert(bool, valid);
compare_decoders_assert(uint32_t, length);
compare_decoders_assert(bool, isBranch);
compare_decoders_assert(bool, isCall);
compare_decoders_assert(bool, isUnconditionalBranch);
compare_decoders_assert(bool, isConditionalBranch);
compare_decoders_assert(bool, isReturn);
passthrough_to_cs(string, getMnemonic);
compare_decoders(int64_t, getImmediate);
passthrough_to_bea(virtual_offset_t, getAddress);
passthrough_to_bea(bool, setsStackPointer);
passthrough_to_bea(uint32_t, getPrefixCount);
passthrough_to_bea(bool, hasRepPrefix);
passthrough_to_bea(bool, hasRepnePrefix);
passthrough_to_bea(bool, hasOperandSizePrefix);
passthrough_to_bea(bool, hasRexWPrefix);
passthrough_to_bea(bool, hasImplicitlyModifiedRegs);


bool DecodedInstructionMeta_t::hasOperand(const int op_num) const
{
	return bea.hasOperand(op_num);
}


// 0-based.  first operand is numbered 0.
DecodedOperandMeta_t DecodedInstructionMeta_t::getOperand(const int op_num) const
{
	return DecodedOperandMeta_t(bea.getOperand(op_num));
}

DecodedOperandMetaVector_t DecodedInstructionMeta_t::getOperands() const
{
	auto ret_val=DecodedOperandMetaVector_t();
	auto bea_operands=bea.getOperands();
	for(const auto &op : bea_operands ) 
		ret_val.push_back(DecodedOperandMeta_t(op));
	return ret_val;
}

virtual_offset_t DecodedInstructionMeta_t::getMemoryDisplacementOffset(const DecodedOperandMeta_t& t) const
{
	return bea.getMemoryDisplacementOffset(t.bea);
}



