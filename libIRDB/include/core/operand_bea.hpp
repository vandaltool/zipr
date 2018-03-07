#ifndef libRIDB_decodedoperandbea_hpp
#define libRIDB_decodedoperandbea_hpp

namespace libIRDB
{

using namespace std;
using namespace libIRDB;


class DecodedOperandBea_t
{
	public:
		DecodedOperandBea_t() =delete;
		DecodedOperandBea_t& operator=(const DecodedOperandBea_t& copy);
		DecodedOperandBea_t(const DecodedOperandBea_t& copy);
		virtual ~DecodedOperandBea_t();

		bool isConstant() const;
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


		void* arg_data;
		DecodedOperandBea_t(void* arg_p);

		friend class DecodedInstructionBea_t;

};

}
#endif
