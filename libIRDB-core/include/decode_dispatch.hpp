
namespace libIRDB
{

using namespace libIRDB;
using namespace std;

class DecodedOperandDispatcher_t;
typedef std::vector<DecodedOperandDispatcher_t> DecodedOperandMetaVector_t;

class DecodedInstructionDispatcher_t : virtual public IRDB_SDK::DecodedInstruction_t
{
	public:
		DecodedInstructionDispatcher_t()=delete;
		DecodedInstructionDispatcher_t(const Instruction_t*);
		DecodedInstructionDispatcher_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len);
		DecodedInstructionDispatcher_t(const virtual_offset_t start_addr, const void *data, const void* endptr);
		DecodedInstructionDispatcher_t(const DecodedInstructionDispatcher_t& copy);
		DecodedInstructionDispatcher_t& operator=(const DecodedInstructionDispatcher_t& copy);

		virtual ~DecodedInstructionDispatcher_t();

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
		IRDB_SDK::VirtualOffset_t getMemoryDisplacementOffset(const IRDB_SDK::DecodedOperand_t* t, const IRDB_SDK::Instruction_t* insn) const;

		// 0-based.  first operand is numbered 0.
		bool hasOperand(const int op_num) const;
		shared_ptr<IRDB_SDK::DecodedOperand_t> getOperand(const int op_num) const;
		IRDB_SDK::DecodedOperandVector_t getOperands() const;

	private:

		std::shared_ptr<DecodedInstructionCapstone_t> cs;

};

}

