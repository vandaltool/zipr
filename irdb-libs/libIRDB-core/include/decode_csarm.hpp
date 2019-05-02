
namespace libIRDB
{

	using namespace libIRDB;
	using namespace std;

	class DecodedOperandCapstoneARM_t;
	class DecodedInstructionCapstoneARM_t : virtual public IRDB_SDK::DecodedInstruction_t
	{
		public:
			DecodedInstructionCapstoneARM_t& operator=(const DecodedInstructionCapstoneARM_t& copy)
			{
				my_insn = copy.my_insn;
				return *this;
			}

			virtual ~DecodedInstructionCapstoneARM_t() { } 

		protected:
			DecodedInstructionCapstoneARM_t()
			{
				if(!cs_handle) cs_handle=new CapstoneHandle_t(NULL);
			}

			shared_ptr<void> my_insn;

			class CapstoneHandle_t
			{
				public:
					CapstoneHandle_t(FileIR_t* firp=NULL);
					inline unsigned long getHandle() { return handle; }

				private:
					// csh handle;   # this is the real type, but it's an alias and I don't want to export capstone values.
					unsigned long handle;
			};
			static CapstoneHandle_t *cs_handle;

			friend class IRDB_SDK::DecodedInstruction_t;
			friend class DecodedOperandCapstoneARM_t;
	};

	class DecodedOperandCapstoneARM32_t;
	class DecodedInstructionCapstoneARM32_t : public DecodedInstructionCapstoneARM_t
	{
		public:
			DecodedInstructionCapstoneARM32_t()=delete;
			DecodedInstructionCapstoneARM32_t& operator=(const DecodedInstructionCapstoneARM32_t& copy);

			virtual ~DecodedInstructionCapstoneARM32_t();

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
			virtual IRDB_SDK::VirtualOffset_t getMemoryDisplacementOffset(const DecodedOperand_t* t, const IRDB_SDK::Instruction_t* insn) const override;

			// 0-based.  first operand is numbered 0.
			virtual bool hasOperand(const int op_num) const override;
			virtual std::shared_ptr<DecodedOperand_t> getOperand(const int op_num) const override;
			virtual DecodedOperandVector_t getOperands() const override;

		protected:

			void Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len);

		private:
			DecodedInstructionCapstoneARM32_t(const shared_ptr<void> &my_insn) ;
			DecodedInstructionCapstoneARM32_t(const Instruction_t* my_insn) ;
			DecodedInstructionCapstoneARM32_t(const virtual_offset_t sa, const void *data, uint32_t max_len  ) ;
			DecodedInstructionCapstoneARM32_t(const virtual_offset_t sa, const void *data, const void* endptr) ;
			DecodedInstructionCapstoneARM32_t(const DecodedInstructionCapstoneARM32_t& copy) ;

			friend class IRDB_SDK::DecodedInstruction_t;
			friend class DecodedOperandCapstoneARM32_t;
	};

	class DecodedOperandCapstoneARM64_t;
	class DecodedInstructionCapstoneARM64_t : public DecodedInstructionCapstoneARM_t
	{
		public:
			DecodedInstructionCapstoneARM64_t()=delete;
			DecodedInstructionCapstoneARM64_t& operator=(const DecodedInstructionCapstoneARM64_t& copy);

			virtual ~DecodedInstructionCapstoneARM64_t();

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
			virtual IRDB_SDK::VirtualOffset_t getMemoryDisplacementOffset(const DecodedOperand_t* t, const IRDB_SDK::Instruction_t* insn) const override;

			// 0-based.  first operand is numbered 0.
			virtual bool hasOperand(const int op_num) const override;
			virtual std::shared_ptr<DecodedOperand_t> getOperand(const int op_num) const override;
			virtual DecodedOperandVector_t getOperands() const override;

		protected:

			void Disassemble(const virtual_offset_t start_addr, const void *data, const uint32_t max_len);

		private:
			DecodedInstructionCapstoneARM64_t(const shared_ptr<void> &my_insn) ;
			DecodedInstructionCapstoneARM64_t(const Instruction_t*) ;
			DecodedInstructionCapstoneARM64_t(const virtual_offset_t sa, const void *data, uint32_t max_len  ) ;
			DecodedInstructionCapstoneARM64_t(const virtual_offset_t sa, const void *data, const void* endptr) ;
			DecodedInstructionCapstoneARM64_t(const DecodedInstructionCapstoneARM64_t& copy) ;

			friend class IRDB_SDK::DecodedInstruction_t;
			friend class DecodedOperandCapstoneARM64_t;
	};

}

