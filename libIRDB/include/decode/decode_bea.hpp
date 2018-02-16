#ifndef libirdb_decodebea_hpp
#define libirdb_decodebea_hpp

#include <stdint.h>
#include <vector>

namespace libIRDB
{

using namespace libIRDB;
using namespace std;

class DecodedOperandBea_t;
typedef std::vector<DecodedOperandBea_t> DecodedOperandBeaVector_t;

class DecodedInstructionBea_t
{
	public:
		DecodedInstructionBea_t()=delete;
		DecodedInstructionBea_t(const Instruction_t*);
		DecodedInstructionBea_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len);
		DecodedInstructionBea_t(const virtual_offset_t start_addr, const void *data, const void* endptr);
		DecodedInstructionBea_t(const DecodedInstructionBea_t& copy);
		DecodedInstructionBea_t& operator=(const DecodedInstructionBea_t& copy);

		virtual ~DecodedInstructionBea_t();

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
		bool hasRepPrefix() const;
		bool hasRepnePrefix() const;
		bool hasOperandSizePrefix() const;
		bool hasRexWPrefix() const;
		bool hasImplicitlyModifiedRegs() const;
		virtual_offset_t getMemoryDisplacementOffset(const DecodedOperandBea_t& t) const;

		// 0-based.  first operand is numbered 0.
		bool hasOperand(const int op_num) const;
		DecodedOperandBea_t getOperand(const int op_num) const;
		DecodedOperandBeaVector_t getOperands() const;

	private:

		void Disassemble(const virtual_offset_t start_addr, const void *, uint32_t max_len);
		void *disasm_data;
		uint32_t disasm_length;
};

}

#endif
