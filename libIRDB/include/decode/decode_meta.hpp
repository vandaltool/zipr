#ifndef libirdb_decodemeta_hpp
#define libirdb_decodemeta_hpp

#include <stdint.h>
#include <vector>
#include <decode/decode_bea.hpp>
#include <decode/operand_bea.hpp>
#include <decode/decode_cs.hpp>
#include <decode/operand_cs.hpp>

namespace libIRDB
{

using namespace libIRDB;
using namespace std;

class DecodedOperandMeta_t;
typedef std::vector<DecodedOperandMeta_t> DecodedOperandMetaVector_t;

class DecodedInstructionMeta_t
{
	public:
		DecodedInstructionMeta_t()=delete;
		DecodedInstructionMeta_t(const Instruction_t*);
		DecodedInstructionMeta_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len);
		DecodedInstructionMeta_t(const virtual_offset_t start_addr, const void *data, const void* endptr);
		DecodedInstructionMeta_t(const DecodedInstructionMeta_t& copy);
		DecodedInstructionMeta_t& operator=(const DecodedInstructionMeta_t& copy);

		virtual ~DecodedInstructionMeta_t();

		string getDisassembly() const;
		bool valid() const;
		uint32_t length() const;
		bool isBranch() const;
		bool isCall() const;
		bool isUnconditionalBranch() const;
		bool isConditionalBranch() const;
		bool isReturn() const;
		string getMnemonic() const;
		int64_t getImmediate() const;
		virtual_offset_t getAddress() const;
		bool setsStackPointer() const;
		uint32_t getPrefixCount() const;
		bool hasRelevantRepPrefix() const;
		bool hasRelevantRepnePrefix() const;
		bool hasRelevantOperandSizePrefix() const;
		bool hasRexWPrefix() const;
		bool hasImplicitlyModifiedRegs() const;
		virtual_offset_t getMemoryDisplacementOffset(const DecodedOperandMeta_t& t) const;

		// 0-based.  first operand is numbered 0.
		bool hasOperand(const int op_num) const;
		DecodedOperandMeta_t getOperand(const int op_num) const;
		DecodedOperandMetaVector_t getOperands() const;

	private:

		DecodedInstructionBea_t bea;
		DecodedInstructionCapstone_t cs;

};

}

#endif
