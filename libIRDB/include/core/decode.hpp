#ifndef libdecode_decode_hpp
#define libdecode_decode_hpp

#define USECS 

#if defined(USEMETA)
#include <core/decode_meta.hpp>
#include <core/operand_meta.hpp>
#elif defined(USECS)
#include <core/decode_cs.hpp>
#include <core/operand_cs.hpp>
#elif defined(USEBEA)
#include <core/decode_bea.hpp>
#include <core/operand_bea.hpp>
#else
#pragma "Error:  Source did not adaquately specify a disassembler. "
#endif


namespace libIRDB
{
using namespace std;
using namespace libIRDB;



#if defined(USEMETA)
typedef DecodedInstructionMeta_t DecodedInstruction_t;
typedef DecodedOperandMeta_t DecodedOperand_t;
typedef DecodedOperandMetaVector_t DecodedOperandVector_t;
#elif defined(USECS)
typedef DecodedInstructionCapstone_t DecodedInstruction_t;
typedef DecodedOperandCapstone_t DecodedOperand_t;
typedef DecodedOperandCapstoneVector_t DecodedOperandVector_t;
#elif defined(USEBEA)
typedef DecodedInstructionBea_t DecodedInstruction_t;
typedef DecodedOperandBea_t DecodedOperand_t;
typedef DecodedOperandBeaVector_t DecodedOperandVector_t;
#else
#pragma "Error:  Source did not adaquately specify a disassembler. "
#endif

}

#endif
