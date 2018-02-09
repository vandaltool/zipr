#ifndef libirdb_decode_hpp
#define libirdb_decode_hpp

#include <stdint.h>
#include <vector>

namespace libIRDB
{

using namespace libIRDB;
using namespace std;

class DecodedOperand_t;
typedef std::vector<DecodedOperand_t> DecodedOperandVector_t;

class DecodedInstruction_t
{
	public:
		DecodedInstruction_t()=delete;
		DecodedInstruction_t(const Instruction_t*);
		DecodedInstruction_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len);
		DecodedInstruction_t(const virtual_offset_t start_addr, const void *data, const void* endptr);
		DecodedInstruction_t(const DecodedInstruction_t& copy);
		DecodedInstruction_t& operator=(const DecodedInstruction_t& copy);

		virtual ~DecodedInstruction_t();

		string getDisassembly() const;
		bool valid() const;
		uint32_t length() const;
		bool isBranch() const;
		bool isCall() const;
		bool isUnconditionalBranch() const;
		bool isConditionalBranch() const;
		bool isReturn() const;
		string getMnemonic() const;
		virtual_offset_t getImmediate() const;
		virtual_offset_t getAddress() const;
		bool setsStackPointer() const;
		uint32_t getPrefixCount() const;
		virtual_offset_t getMemoryDisplacementOffset(const DecodedOperand_t& t) const;

		// 0-based.  first operand is numbered 0.
		bool hasOperand(const int op_num) const;
		DecodedOperand_t getOperand(const int op_num) const;
		DecodedOperandVector_t getOperands() const;

	private:

		void Disassemble(const virtual_offset_t start_addr, const void *, uint32_t max_len);
		void *disasm_data;
		uint32_t disasm_length;
};

}

#endif
