#ifndef libdecode_decode_hpp
#define libdecode_decode_hpp

#define USEMETA 

#if defined(USEMETA)
#include <core/decode_dispatch.hpp>
#include <core/operand_dispatch.hpp>
#elif defined(USECS)
#include <core/decode_csx86.hpp>
#include <core/operand_csx86.hpp>
#pragma "Error:  Source did not adaquately specify a disassembler. "
#endif


namespace libIRDB
{
using namespace std;
using namespace libIRDB;



#if defined(USEMETA)
typedef DecodedInstructionDispatcher_t DecodedInstruction_t;
typedef DecodedOperandDispatcher_t DecodedOperand_t;
typedef DecodedOperandMetaVector_t DecodedOperandVector_t;
#elif defined(USECS)
typedef DecodedInstructionCapstoneX86_t DecodedInstruction_t;
typedef DecodedOperandCapstoneX86_t DecodedOperand_t;
typedef DecodedOperandCapstoneVector_t DecodedOperandVector_t;
#else
#pragma "Error:  Source did not adaquately specify a disassembler. "
#endif

}

#endif
