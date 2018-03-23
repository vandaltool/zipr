#ifndef libRIDB_decodedoperandcs_hpp
#define libRIDB_decodedoperandcs_hpp

#include <memory>
namespace libIRDB
{

using namespace std;
using namespace libIRDB;


class DecodedOperandCapstone_t
{
	public:
		DecodedOperandCapstone_t() =delete;
		//DecodedOperandCapstone_t& operator=(const DecodedOperandCapstone_t& copy);
		//DecodedOperandCapstone_t(const DecodedOperandCapstone_t& copy);
		virtual ~DecodedOperandCapstone_t();

		bool isConstant() const;
		uint64_t getConstant() const;
		string getString() const;
		bool isRegister() const;
		bool isGeneralPurposeRegister() const;
		bool isMmxRegister() const;
		bool isFpuRegister() const;
		bool isSseRegister() const;
		bool isAvxRegister() const;
		bool isZmmRegister() const;
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

		DecodedOperandCapstone_t( const shared_ptr<void> &my_insn, uint8_t op_num);

		shared_ptr<void> my_insn;
		uint8_t op_num;

		friend class DecodedInstructionCapstone_t;

};

}
#endif
