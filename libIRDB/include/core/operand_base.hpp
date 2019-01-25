#if 0
namespace libIRDB
{

using namespace std;
using namespace libIRDB;


class DecodedOperandCapstone_t : virtual public IRDB_SDK::DecodedOperand_t 
{
	public:
		virtual bool isConstant() const=0;
		virtual uint64_t getConstant() const=0;
		virtual string getString() const=0;
		virtual bool isRegister() const=0;
		virtual bool isGeneralPurposeRegister() const=0;
		virtual bool isMmxRegister() const=0;
		virtual bool isFpuRegister() const=0;
		virtual bool isSseRegister() const=0;
		virtual bool isAvxRegister() const=0;
		virtual bool isZmmRegister() const=0;
		virtual bool isSpecialRegister() const=0;
		virtual bool isSegmentRegister() const=0;
		virtual uint32_t getRegNumber() const=0;
		virtual bool isMemory() const=0;
		virtual bool hasSegmentRegister() const=0;
		virtual uint32_t getSegmentRegister() const=0;
		virtual bool hasBaseRegister() const=0;
		virtual bool hasIndexRegister() const=0;
		virtual uint32_t getBaseRegister() const=0;
		virtual uint32_t getIndexRegister() const=0;
		virtual bool hasMemoryDisplacement() const=0;
		virtual virtual_offset_t getMemoryDisplacement() const=0;
		virtual bool isPcrel() const=0;
		virtual uint32_t getScaleValue() const=0;
		virtual uint32_t getMemoryDisplacementEncodingSize() const=0;
		virtual uint32_t getArgumentSizeInBytes() const=0;
		virtual uint32_t getArgumentSizeInBits() const=0;
		virtual bool isRead() const=0; 
		virtual bool isWritten() const=0; 

};

}
#endif
