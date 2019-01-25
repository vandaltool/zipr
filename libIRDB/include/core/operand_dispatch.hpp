namespace libIRDB
{

using namespace std;
using namespace libIRDB;


class DecodedOperandDispatcher_t : virtual public IRDB_SDK::DecodedOperand_t
{
	public:
		DecodedOperandDispatcher_t() =delete;
		DecodedOperandDispatcher_t& operator=(const DecodedOperandDispatcher_t& copy);
		DecodedOperandDispatcher_t(const DecodedOperandDispatcher_t& copy);
		virtual ~DecodedOperandDispatcher_t();

		bool isConstant() const;
		uint64_t getConstant() const;
		string getString() const;
		bool isRegister() const;
		bool isGeneralPurposeRegister() const;
		bool isMmxRegister() const;
		bool isFpuRegister() const;
		bool isSseRegister() const;
		bool isAvxRegister() const;
		bool isSpecialRegister() const;
		bool isSegmentRegister() const;
		uint32_t getRegNumber() const;
		bool isMemory() const;
		bool hasSegmentRegister() const;
		uint32_t getSegmentRegister() const;
		bool hasBaseRegister() const;
		bool hasIndexRegister() const;
		uint32_t getBaseRegister() const;
		uint32_t getIndexRegister() const;
		bool hasMemoryDisplacement() const;
		virtual_offset_t getMemoryDisplacement() const;
		bool isPcrel() const;
		uint32_t getScaleValue() const;
		uint32_t getMemoryDisplacementEncodingSize() const;
		uint32_t getArgumentSizeInBytes() const;
		uint32_t getArgumentSizeInBits() const;
		bool isRead() const; 
		bool isWritten() const; 


	private:
		DecodedOperandDispatcher_t(const shared_ptr<DecodedOperandCapstone_t> in);

		std::shared_ptr<DecodedOperandCapstone_t> cs;

		friend class DecodedInstructionDispatcher_t;

};

}
