#ifndef _LIBTRANSFORM_INTEGERTRANSFORM_H_
#define _LIBTRANSFORM_INTEGERTRANSFORM_H_

#include "transform.hpp"
#include "MEDS_Register.hpp"
#include "VirtualOffset.hpp"

namespace libTransform
{

using namespace std;
using namespace libIRDB;


class IntegerTransform : public Transform
{
	public:
		IntegerTransform(VariantID_t *, VariantIR_t*, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_warnings); 
		int execute();

		void setSaturatingArithmetic(bool p_satArithmetic) { m_saturatingArithmetic = p_satArithmetic; }
		bool getSaturatingArithmetic() { return m_saturatingArithmetic; }
	
	private:
		void handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleSignedness(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		void addSignednessCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void addTruncationCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		void addOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy, AddressID_t *p_original = NULL);
		void addUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		void addOverflowCheckNoFlag(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void addOverflowCheckNoFlag_RegPlusReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName&, const Register::RegisterName&, const Register::RegisterName&, int p_policy);
		void addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName&, int p_constantValue, const Register::RegisterName&, int p_policy);
		void addOverflowCheckNoFlag_RegTimesConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName&, int p_constantValue, const Register::RegisterName&, int p_policy);

		void addMaxSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough);
		void addMinSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough);

	private:
		std::set<VirtualOffset>*  m_benignFalsePositives;
		bool                      m_saturatingArithmetic;
};

// make sure these match the function names in $STRATA/src/posix/x86_linux/detector_number_handling/overflow_detector.c

#define	INTEGER_OVERFLOW_DETECTOR            "integer_overflow_detector"
#define	ADDSUB_OVERFLOW_DETECTOR_SIGNED_32   "addsub_overflow_detector_signed_32"
#define	ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_32 "addsub_overflow_detector_unsigned_32"
#define	ADDSUB_OVERFLOW_DETECTOR_SIGNED_16   "addsub_overflow_detector_signed_16"
#define	ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_16 "addsub_overflow_detector_unsigned_16"
#define	ADDSUB_OVERFLOW_DETECTOR_SIGNED_8    "addsub_overflow_detector_signed_8"
#define	ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_8  "addsub_overflow_detector_unsigned_8"
#define	MUL_OVERFLOW_DETECTOR_32             "mul_overflow_detector_32"
#define	MUL_OVERFLOW_DETECTOR_16             "mul_overflow_detector_16"
#define	MUL_OVERFLOW_DETECTOR_8              "mul_overflow_detector_8"
#define	TRUNCATION_DETECTOR                  "truncation_detector"
#define	TRUNCATION_DETECTOR_32_16            "truncation_detector_32_16"
#define	TRUNCATION_DETECTOR_32_8             "truncation_detector_32_8"
#define	SIGNEDNESS_DETECTOR_32               "signedness_detector_32"
#define	SIGNEDNESS_DETECTOR_16               "signedness_detector_16"
#define	SIGNEDNESS_DETECTOR_8                "signedness_detector_8"
#define UNDERFLOW_DETECTOR_32                "underflow_detector_32"
#define UNDERFLOW_DETECTOR_16                "underflow_detector_16"
#define UNDERFLOW_DETECTOR_8                 "underflow_detector_8"
#define UNDERFLOW_DETECTOR_UNSIGNED_32       "underflow_detector_unsigned_32"
#define UNDERFLOW_DETECTOR_UNSIGNED_16       "underflow_detector_unsigned_16"
#define UNDERFLOW_DETECTOR_UNSIGNED_8        "underflow_detector_unsigned_8"
#define UNDERFLOW_DETECTOR_SIGNED_32         "underflow_detector_signed_32"
#define UNDERFLOW_DETECTOR_SIGNED_16         "underflow_detector_signed_16"
#define UNDERFLOW_DETECTOR_SIGNED_8          "underflow_detector_signed_8"
}

#endif
