#if 0
namespace libIRDB
{

using namespace libIRDB;
using namespace std;

class DecodedOperandCapstone_t;

class DecodedInstructionCapstone_t : virtual public IRDB_SDK::DecodedInstruction_t 
{
	public:
		virtual ~DecodedInstructionCapstone_t(){}
		virtual string getDisassembly() const =0;
		virtual bool valid() const =0;
		virtual uint32_t length() const =0;
		virtual bool isBranch() const =0;
		virtual bool isCall() const =0;
		virtual bool isUnconditionalBranch() const =0;
		virtual bool isConditionalBranch() const =0;
		virtual bool isReturn() const =0;
		virtual string getMnemonic() const =0;
		virtual int64_t getImmediate() const =0;
		virtual virtual_offset_t getAddress() const =0;
		virtual bool setsStackPointer() const =0;
		virtual uint32_t getPrefixCount() const =0;
		virtual bool hasRelevantRepPrefix() const =0;
		virtual bool hasRelevantRepnePrefix() const =0;
		virtual bool hasRelevantOperandSizePrefix() const =0;
		virtual bool hasRexWPrefix() const =0;
		virtual bool hasImplicitlyModifiedRegs() const =0;
		virtual virtual_offset_t getMemoryDisplacementOffset(const IRDB_SDK::DecodedOperand_t& t, const Instruction_t* insn) const =0;

		// 0-based.  first operand is numbered 0.
		virtual bool hasOperand(const int op_num) const =0;
		virtual shared_ptr<IRDB_SDK::DecodedOperand_t> getOperand(const int op_num) const =0;
		virtual DecodedOperandVector_t getOperands() const =0;

	private:

		// static unique_ptr<DecodedInstructionCapstone_t> factory(const Instruction_t* i);
		// static unique_ptr<DecodedInstructionCapstone_t> factory(const IRDB_SDK::VirtualOffset_t start_addr, const void *data, uint32_t max_len);
		// static unique_ptr<DecodedInstructionCapstone_t> factory(const IRDB_SDK::VirtualOffset_t start_addr, const void *data, const void* endptr);
		// friend class DecodedInstructionDispatcher_t;

};

}

#endif
