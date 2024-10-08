
#include <libIRDB-core.hpp>
#include <decode_base.hpp>
#include <operand_base.hpp>
#include <decode_csx86.hpp>
#include <operand_csx86.hpp>
#include <decode_csarm.hpp>
#include <operand_csarm.hpp>
#include <decode_csmips.hpp>
#include <operand_csmips.hpp>
#include <exception>

using namespace std;
using namespace libIRDB;


unique_ptr<IRDB_SDK::DecodedInstruction_t> IRDB_SDK::DecodedInstruction_t::factory(const IRDB_SDK::Instruction_t* p_i)
{
	const auto i=dynamic_cast<const libIRDB::Instruction_t*>(p_i);
	assert(i);
	auto op = 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtMips32  ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneMIPS32_t(i) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtArm32   ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneARM32_t (i) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtAarch64 ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneARM64_t (i) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtX86_64  ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneX86_t   (i) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtI386    ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneX86_t   (i) : 
	          throw invalid_argument("Unknown machine type");
	return unique_ptr<DecodedInstruction_t>(op);
}

unique_ptr<IRDB_SDK::DecodedInstruction_t> IRDB_SDK::DecodedInstruction_t::factory(const IRDB_SDK::VirtualOffset_t start_addr, const void *data, uint32_t max_len)
{
	auto op = 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtMips32  ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneMIPS32_t(start_addr,data,max_len) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtArm32   ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneARM32_t (start_addr,data,max_len) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtAarch64 ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneARM64_t (start_addr,data,max_len) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtX86_64  ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneX86_t   (start_addr,data,max_len) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtI386    ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneX86_t   (start_addr,data,max_len) : 
	          throw invalid_argument("Unknown machine type");
	return unique_ptr<DecodedInstruction_t>(op);
}

unique_ptr<IRDB_SDK::DecodedInstruction_t> IRDB_SDK::DecodedInstruction_t::factory(const IRDB_SDK::VirtualOffset_t start_addr, const void *data, const void* endptr)
{
	auto op = 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtMips32  ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneMIPS32_t(start_addr,data,endptr) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtArm32   ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneARM32_t (start_addr,data,endptr) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtAarch64 ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneARM64_t (start_addr,data,endptr) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtX86_64  ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneX86_t   (start_addr,data,endptr) : 
	          IRDB_SDK::FileIR_t::getArchitecture()->getMachineType()==IRDB_SDK::admtI386    ? (IRDB_SDK::DecodedInstruction_t*)new DecodedInstructionCapstoneX86_t   (start_addr,data,endptr) : 
	          throw invalid_argument("Unknown machine type");
	return unique_ptr<DecodedInstruction_t>(op);
}

