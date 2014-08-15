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
		IntegerTransform64(VariantID_t *, FileIR_t*, MEDS_Annotations_t *p_annotations, 
			set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_warnings); 

		int execute();

	protected:
		Instruction_t* addCallbackHandlerSequence(Instruction_t *p_orig, Instruction_t *p_fallthrough, std::string p_detector, int p_policy);
		void handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleSignedness(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		bool addOverflowUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		void saturateSignedMultiplyOverflow(Instruction_t *p_orig, Instruction_t *p_instruction, Instruction_t* p_fallthrough);
		bool addOverflowCheckNoFlag(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void addOverflowCheckNoFlag_RegPlusReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy);
		void addOverflowCheckNoFlag_RegTimesReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy);
		void addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constant, const Register::RegisterName& p_reg3, int p_policy);
		void addOverflowCheckNoFlag_RegTimesConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constant, const Register::RegisterName& p_reg3, int p_policy);
		bool addTruncationCheck32(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		bool addTruncationCheck64(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		bool addSignednessCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		string buildSaturationAssembly(Instruction_t *p_instruction, string p_pattern, string p_value);
};


} // end namespace

#define OVERFLOW64_DETECTOR            "overflow_detector_64"
#define UNDERFLOW64_DETECTOR           "underflow_detector_64"

#define	TRUNCATION64_DETECTOR_UNSIGNED_64_32      "truncation_detector_64_32"
#define	TRUNCATION64_DETECTOR_SIGNED_64_32        "truncation_detector_64_32"
#define	TRUNCATION64_DETECTOR_UNKNOWN_64_32       "truncation_detector_64_32"

#define	TRUNCATION64_DETECTOR_UNSIGNED_64_16      "truncation_detector_64_16"
#define	TRUNCATION64_DETECTOR_SIGNED_64_16        "truncation_detector_64_16"
#define	TRUNCATION64_DETECTOR_UNKNOWN_64_16       "truncation_detector_64_16"

#define	TRUNCATION64_DETECTOR_UNSIGNED_64_8       "truncation_detector_64_8"
#define	TRUNCATION64_DETECTOR_SIGNED_64_8         "truncation_detector_64_8"
#define	TRUNCATION64_DETECTOR_UNKNOWN_64_8        "truncation_detector_64_8"

#define	TRUNCATION64_DETECTOR_UNSIGNED_32_16      "truncation_detector_32_16"
#define	TRUNCATION64_DETECTOR_SIGNED_32_16        "truncation_detector_32_16"
#define	TRUNCATION64_DETECTOR_UNKNOWN_32_16       "truncation_detector_32_16"

#define	TRUNCATION64_DETECTOR_UNSIGNED_32_8       "truncation_detector_32_8"
#define	TRUNCATION64_DETECTOR_SIGNED_32_8         "truncation_detector_32_8"
#define	TRUNCATION64_DETECTOR_UNKNOWN_32_8        "truncation_detector_32_8"

#define	TRUNCATION64_FORCE_EXIT                   "truncation_force_exit"

#define SIGNEDNESS64_DETECTOR_SIGNED              "signedness_detector_signed"
#define SIGNEDNESS64_DETECTOR_UNSIGNED            "signedness_detector_unsigned"

#endif
