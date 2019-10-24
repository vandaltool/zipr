
namespace libIRDB
{

	using namespace std;
	using namespace libIRDB;

	class DecodedOperandCapstoneMIPS_t : virtual public IRDB_SDK::DecodedOperand_t
	{
		public:
			DecodedOperandCapstoneMIPS_t() =delete;
			virtual ~DecodedOperandCapstoneMIPS_t() { }

		protected:

			DecodedOperandCapstoneMIPS_t( const shared_ptr<void> &p_my_insn, uint8_t p_op_num)
				: my_insn(p_my_insn), op_num(p_op_num)
			{
			}

			shared_ptr<void> my_insn;
			uint8_t op_num;

			friend class DecodedInstructionCapstoneMIPS_t;
	};

	class DecodedOperandCapstoneMIPS32_t : public DecodedOperandCapstoneMIPS_t
	{
		public:
			DecodedOperandCapstoneMIPS32_t() =delete;
			virtual ~DecodedOperandCapstoneMIPS32_t();
			virtual bool isConstant() const override;
			virtual uint64_t getConstant() const override;
			virtual string getString() const override;
			virtual bool isRegister() const override;
			virtual bool isGeneralPurposeRegister() const override;
			virtual bool isMmxRegister() const override;
			virtual bool isFpuRegister() const override;
			virtual bool isSseRegister() const override;
			virtual bool isAvxRegister() const override;
			virtual bool isZmmRegister() const override;
			virtual bool isSpecialRegister() const override;
			virtual bool isSegmentRegister() const override;
			virtual uint32_t getRegNumber() const override;
			virtual bool isMemory() const override;
			virtual bool hasSegmentRegister() const override;
			virtual uint32_t getSegmentRegister() const override;
			virtual bool hasBaseRegister() const override;
			virtual bool hasIndexRegister() const override;
			virtual uint32_t getBaseRegister() const override;
			virtual uint32_t getIndexRegister() const override;
			virtual bool hasMemoryDisplacement() const override;
			virtual virtual_offset_t getMemoryDisplacement() const override;
			virtual bool isPcrel() const override;
			virtual uint32_t getScaleValue() const override;
			virtual uint32_t getMemoryDisplacementEncodingSize() const override;
			virtual uint32_t getArgumentSizeInBytes() const override;
			virtual uint32_t getArgumentSizeInBits() const override;
			virtual bool isRead() const override; 
			virtual bool isWritten() const override; 
		private:
			DecodedOperandCapstoneMIPS32_t( const shared_ptr<void> &p_my_insn, uint8_t p_op_num);
			friend class DecodedInstructionCapstoneMIPS32_t;


	};

	class DecodedOperandCapstoneMIPS64_t : public DecodedOperandCapstoneMIPS_t
	{
		public:
			DecodedOperandCapstoneMIPS64_t() =delete;
			virtual ~DecodedOperandCapstoneMIPS64_t();
			virtual bool isConstant() const override;
			virtual uint64_t getConstant() const override;
			virtual string getString() const override;
			virtual bool isRegister() const override;
			virtual bool isGeneralPurposeRegister() const override;
			virtual bool isMmxRegister() const override;
			virtual bool isFpuRegister() const override;
			virtual bool isSseRegister() const override;
			virtual bool isAvxRegister() const override;
			virtual bool isZmmRegister() const override;
			virtual bool isSpecialRegister() const override;
			virtual bool isSegmentRegister() const override;
			virtual uint32_t getRegNumber() const override;
			virtual bool isMemory() const override;
			virtual bool hasSegmentRegister() const override;
			virtual uint32_t getSegmentRegister() const override;
			virtual bool hasBaseRegister() const override;
			virtual bool hasIndexRegister() const override;
			virtual uint32_t getBaseRegister() const override;
			virtual uint32_t getIndexRegister() const override;
			virtual bool hasMemoryDisplacement() const override;
			virtual virtual_offset_t getMemoryDisplacement() const override;
			virtual bool isPcrel() const override;
			virtual uint32_t getScaleValue() const override;
			virtual uint32_t getMemoryDisplacementEncodingSize() const override;
			virtual uint32_t getArgumentSizeInBytes() const override;
			virtual uint32_t getArgumentSizeInBits() const override;
			virtual bool isRead() const override; 
			virtual bool isWritten() const override; 

		private:
			DecodedOperandCapstoneMIPS64_t( const shared_ptr<void> &p_my_insn, uint8_t p_op_num);
			friend class DecodedInstructionCapstoneMIPS64_t;
	};
}
