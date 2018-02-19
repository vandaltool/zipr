
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


void do_breakpoint(const char* s) { (void)s; }


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
			cerr<<" in bea="<<bea.getDisassembly();					\
			cerr<<" or cs="<<bea.getDisassembly();					\
			cerr<<" at " <<__FUNCTION__<<endl;					\
			do_breakpoint(__FUNCTION__);						\
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
			cerr<<" in bea="<<bea.getDisassembly();					\
			cerr<<" or cs="<<bea.getDisassembly();					\
			cerr<<" at " <<__FUNCTION__<<endl;					\
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

/* demonstrating things are the same */
compare_decoders_assert(bool, valid);
compare_decoders_assert(uint32_t, length);
compare_decoders_assert(bool, isBranch);
compare_decoders_assert(bool, isCall);
compare_decoders_assert(bool, isUnconditionalBranch);
compare_decoders_assert(bool, isConditionalBranch);
compare_decoders_assert(bool, isReturn);
compare_decoders_assert(uint32_t, getPrefixCount);
compare_decoders_assert(bool, hasRexWPrefix);

/* demonstrating when capstone is better */
passthrough_to_cs(string, getMnemonic);
passthrough_to_cs(string,getDisassembly);
passthrough_to_cs(virtual_offset_t, getAddress);	 // bea sometimes calcs this wrong.  way wrong.

// demonstrating they different and trying to determine which is better.
compare_decoders(int64_t, getImmediate);
compare_decoders(bool, hasRelevantRepPrefix);
compare_decoders(bool, hasRelevantRepnePrefix);
compare_decoders(bool, hasRelevantOperandSizePrefix);

// not yet completed
passthrough_to_bea(bool, setsStackPointer);
passthrough_to_bea(bool, hasImplicitlyModifiedRegs);


// needs more complicated impl.


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



