#ifndef libRIDB_decodedoperand_hpp
#define libRIDB_decodedoperand_hpp

namespace libIRDB
{

using namespace std;
using namespace libIRDB;


class DecodedOperand_t
{
	public:
		DecodedOperand_t() =delete;
		DecodedOperand_t& operator=(const DecodedOperand_t& copy);
		DecodedOperand_t(const DecodedOperand_t& copy);
		virtual ~DecodedOperand_t();

		bool isConstant() const;
		string getString() const;
		bool isWrite() const;
		bool isRegister() const;
		uint32_t getRegNumber() const;
		bool isMemory() const;
		bool hasBaseRegister() const;
		bool hasIndexRegister() const;
		uint32_t getBaseRegister() const;
		uint32_t getIndexRegister() const;
		virtual_offset_t getMemoryDisplacement() const;
		bool isPcrel() const;
		uint32_t getScaleValue() const;
		uint32_t getMemoryDisplacementEncodingSize() const;
		



	private:


		void* arg_data;
		DecodedOperand_t(void* arg_p);

		friend class DecodedInstruction_t;

};

}
#endif
