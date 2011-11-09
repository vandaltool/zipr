#ifndef _LIBTRANSFORM_INTEGERTRANSFORM_H_
#define _LIBTRANSFORM_INTEGERTRANSFORM_H_

#include "transform.hpp"

namespace libTransform
{

using namespace std;
using namespace libIRDB;


class IntegerTransform : public Transform
{
	public:
		IntegerTransform(VariantID_t *, VariantIR_t*, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions); 
		int execute();
	
	private:
		void handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);
		void handleSignedness(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);
		void addOverflowCheck16(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);
		void addOverflowCheck32(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);
		void handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);
		void addTruncationCheck32to16(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);
		void addSignednessCheck16(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);
		void addAddSubSignedOverflowCheck16(libIRDB::Instruction_t*, const MEDS_Annotation::MEDS_InstructionCheckAnnotation&);
};

};

// make sure these match the function names in $STRATA/src/posix/x86_linux/detector_number_handling/overflow_detector.c

#define	INTEGER_OVERFLOW_DETECTOR            "integer_overflow_detector"
#define	ADDSUB_OVERFLOW_DETECTOR_SIGNED_16   "addsub_overflow_detector_signed_16"
#define	ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_16 "addsub_overflow_detector_unsigned_16"
#define	ADDSUB_OVERFLOW_DETECTOR_SIGNED_32   "addsub_overflow_detector_signed_32"
#define	ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_32 "addsub_overflow_detector_unsigned_32"
#define	MUL_OVERFLOW_DETECTOR_16             "mul_overflow_detector_16"
#define	MUL_OVERFLOW_DETECTOR_32             "mul_overflow_detector_32"
#define	TRUNCATION_DETECTOR                  "truncation_detector"
#define	SIGNEDNESS_DETECTOR_16               "signedness_detector_16"

#endif
