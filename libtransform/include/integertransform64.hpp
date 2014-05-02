#ifndef _LIBTRANSFORM_INTEGERTRANSFORM64_H_
#define _LIBTRANSFORM_INTEGERTRANSFORM64_H_

#include "integertransform.hpp"

namespace libTransform
{

using namespace std;
using namespace libIRDB;

class IntegerTransform64 : public IntegerTransform
{
	public:
		IntegerTransform64(VariantID_t *, FileIR_t*, std::multimap<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_warnings); 

		int execute();

	protected:
		Instruction_t* addCallbackHandlerSequence(Instruction_t *p_orig, Instruction_t *p_fallthrough, std::string p_detector, int p_policy);
		void handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void addOverflowUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		void saturateSignedMultiplyOverflow(Instruction_t *p_orig, Instruction_t *p_instruction, Instruction_t* p_fallthrough);
		void addOverflowCheckNoFlag(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void addOverflowCheckNoFlag_RegPlusReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy);
		void addOverflowCheckNoFlag_RegTimesReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy);
		void addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constant, const Register::RegisterName& p_reg3, int p_policy);
		void addOverflowCheckNoFlag_RegTimesConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constant, const Register::RegisterName& p_reg3, int p_policy);
};


} // end namespace

#define OVERFLOW_DETECTOR_64             "overflow_detector_64"
#define UNDERFLOW_DETECTOR_64            "underflow_detector_64"

#endif
