
#include <libIRDB-core.hpp>
#include <core/decode_base.hpp>
#include <core/operand_base.hpp>
#include <core/decode_csx86.hpp>
#include <core/operand_csx86.hpp>
#include <core/decode_csarm.hpp>
#include <core/operand_csarm.hpp>
#include <exception>

using namespace std;
using namespace libIRDB;


unique_ptr<DecodedInstructionCapstone_t> DecodedInstructionCapstone_t::factory(const libIRDB::Instruction_t* i)
{
	auto op=FileIR_t::GetArchitecture()->getMachineType()==admtAarch64 ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneARM64_t(i) : 
		FileIR_t::GetArchitecture()->getMachineType()==admtX86_64  ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneX86_t  (i) : 
		FileIR_t::GetArchitecture()->getMachineType()==admtI386    ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneX86_t  (i) : 
		throw invalid_argument("Unknown machine type");
	return unique_ptr<DecodedInstructionCapstone_t>(op);
}
unique_ptr<DecodedInstructionCapstone_t> DecodedInstructionCapstone_t::factory(const virtual_offset_t start_addr, const void *data, uint32_t max_len)
{
	auto op=FileIR_t::GetArchitecture()->getMachineType()==admtAarch64 ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneARM64_t(start_addr,data,max_len) : 
		FileIR_t::GetArchitecture()->getMachineType()==admtX86_64  ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneX86_t  (start_addr,data,max_len) : 
		FileIR_t::GetArchitecture()->getMachineType()==admtI386    ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneX86_t  (start_addr,data,max_len) : 
		throw invalid_argument("Unknown machine type");
	return unique_ptr<DecodedInstructionCapstone_t>(op);
}
unique_ptr<DecodedInstructionCapstone_t> DecodedInstructionCapstone_t::factory(const virtual_offset_t start_addr, const void *data, const void* endptr)
{
	auto op=FileIR_t::GetArchitecture()->getMachineType()==admtAarch64 ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneARM64_t(start_addr,data,endptr) : 
		FileIR_t::GetArchitecture()->getMachineType()==admtX86_64  ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneX86_t  (start_addr,data,endptr) : 
		FileIR_t::GetArchitecture()->getMachineType()==admtI386    ? (DecodedInstructionCapstone_t*)new DecodedInstructionCapstoneX86_t  (start_addr,data,endptr) : 
		throw invalid_argument("Unknown machine type");
	return unique_ptr<DecodedInstructionCapstone_t>(op);
}

