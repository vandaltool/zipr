#ifndef libirdb_decodecsarm_hpp
#define libirdb_decodecsarm_hpp

#include <stdint.h>
#include <vector>
#include <memory>
#include <core/decode_base.hpp>
#include <core/operand_base.hpp>

namespace libIRDB
{

using namespace libIRDB;
using namespace std;

class DecodedOperandCapstoneARM_t;
class DecodedInstructionCapstoneARM_t : public DecodedInstructionCapstone_t
{
	public:
		DecodedInstructionCapstoneARM_t()=delete;
		DecodedInstructionCapstoneARM_t(const Instruction_t*);
		DecodedInstructionCapstoneARM_t(const virtual_offset_t start_addr, const void *data, uint32_t max_len);
		DecodedInstructionCapstoneARM_t(const virtual_offset_t start_addr, const void *data, const void* endptr);
		DecodedInstructionCapstoneARM_t(const DecodedInstructionCapstoneARM_t& copy);
		DecodedInstructionCapstoneARM_t& operator=(const DecodedInstructionCapstoneARM_t& copy);

		virtual ~DecodedInstructionCapstoneARM_t();

		virtual string getDisassembly() const override;
		virtual bool valid() const override;
		virtual uint32_t length() const override;
		virtual bool isBranch() const override;
		virtual bool isCall() const override;
		virtual bool isUnconditionalBranch() const override; 
		virtual bool isConditionalBranch() const override; 
		virtual bool isReturn() const override; 
		virtual string getMnemonic() const override; 
		virtual int64_t getImmediate() const override; 
		virtual virtual_offset_t getAddress() const override; 
		virtual bool setsStackPointer() const override; 
		virtual uint32_t getPrefixCount() const override; 
		virtual bool hasRelevantRepPrefix() const override; 
		virtual bool hasRelevantRepnePrefix() const override; 
		virtual bool hasRelevantOperandSizePrefix() const override; 
		virtual bool hasRexWPrefix() const override; 
		virtual bool hasImplicitlyModifiedRegs() const override;
 		virtual virtual_offset_t getMemoryDisplacementOffset(const DecodedOperandCapstone_t& t, const Instruction_t* insn) const override;

		// 0-based.  first operand is numbered 0.
		virtual bool hasOperand(const int op_num) const override;
		virtual std::shared_ptr<DecodedOperandCapstone_t> getOperand(const int op_num) const override;
		virtual DecodedOperandCapstoneVector_t getOperands() const override;

	private:

		void Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len);

		shared_ptr<void> my_insn;

		class CapstoneHandle_t
		{
			public:
				CapstoneHandle_t(FileIR_t* firp=NULL);
				inline unsigned long getHandle() { return handle; }

			private:
				// csh handle; // this is the real type, but it's an alias and I don't want to export capstone values.
				unsigned long handle;
		};
		static CapstoneHandle_t *cs_handle;


		friend class DecodedOperandCapstoneARM_t;

		DecodedInstructionCapstoneARM_t(const shared_ptr<void> &my_insn);

};

}

#endif
