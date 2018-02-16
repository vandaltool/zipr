#ifndef libRIDB_decodedoperandmeta_hpp
#define libRIDB_decodedoperandmeta_hpp

#include <decode/decode_bea.hpp>
#include <decode/operand_bea.hpp>
namespace libIRDB
{

using namespace std;
using namespace libIRDB;


class DecodedOperandMeta_t
{
	public:
		DecodedOperandMeta_t() =delete;
		DecodedOperandMeta_t& operator=(const DecodedOperandMeta_t& copy);
		DecodedOperandMeta_t(const DecodedOperandMeta_t& copy);
		virtual ~DecodedOperandMeta_t();

		bool isConstant() const;
		string getString() const;
		bool isWrite() const;
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
		DecodedOperandMeta_t(const DecodedOperandBea_t& in);

		DecodedOperandBea_t bea;

		friend class DecodedInstructionMeta_t;

};

}
#endif
