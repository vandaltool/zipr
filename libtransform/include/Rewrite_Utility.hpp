#include <libIRDB-core.hpp>

using namespace libIRDB;
using namespace std;

namespace IRDBUtility {
// make sure these match values in detector_handlers.h in the strata library
enum mitigation_policy 
{
	P_NONE=0, 
	P_CONTINUE_EXECUTION, 
	P_CONTROLLED_EXIT, 
	P_CONTINUE_EXECUTION_SATURATING_ARITHMETIC, 
	P_CONTINUE_EXECUTION_WARNONLY
};


//The "first" instruction will have its contents replaced and a duplicate of "first" will be in the follow of first. 
//This duplicate is returned since the user already has a pointer to first. 
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target);
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly);

//Does not insert into any variant, just copies data about the instruction itself, see the copyInstruction(src,dest) to see what is copied. 
Instruction_t* copyInstruction(Instruction_t* instr);

Instruction_t* copyInstruction(FileIR_t* virp, Instruction_t* instr);
//copy src to destination
void copyInstruction(Instruction_t* src, Instruction_t* dest);

Instruction_t* allocateNewInstruction(FileIR_t* virp, db_id_t p_fileID,Function_t* func);
Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr);
void setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, string p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target);
Instruction_t* getHandlerCode(FileIR_t* virp, Instruction_t* fallthrough, mitigation_policy policy );

}
