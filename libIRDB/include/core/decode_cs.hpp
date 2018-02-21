#ifndef libirdb_decodecsa_hpp
#define libirdb_decodecsa_hpp

#include <stdint.h>
#include <vector>
#include <memory>

namespace libIRDB
{

using namespace libIRDB;
using namespace std;

class DecodedOperandCapstone_t;
typedef std::vector<DecodedOperandCapstone_t> DecodedOperandCapstoneVector_t;

class DecodedInstructionCapstone_t
{
	public:
		DecodedInstructionCapstone_t()=delete;
		DecodedInstructionCapstone_t(const Instruction_t*);
		DecodedInstructionCapstone_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len);
		DecodedInstructionCapstone_t(const virtual_offset_t start_addr, const void *data, const void* endptr);
		DecodedInstructionCapstone_t(const DecodedInstructionCapstone_t& copy);
		DecodedInstructionCapstone_t& operator=(const DecodedInstructionCapstone_t& copy);

		virtual ~DecodedInstructionCapstone_t();

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
		virtual_offset_t getMemoryDisplacementOffset(const DecodedOperandCapstone_t& t) const;

		// 0-based.  first operand is numbered 0.
		bool hasOperand(const int op_num) const;
		DecodedOperandCapstone_t getOperand(const int op_num) const;
		DecodedOperandCapstoneVector_t getOperands() const;

	private:

		void Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len);

		shared_ptr<void> my_insn;

};

}

#endif
