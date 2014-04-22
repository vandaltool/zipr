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
		IntegerTransform64(VariantID_t *, FileIR_t*, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_warnings); 

		int execute();

	protected:
		void handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
		void addOverflowUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);

		void saturateSignedMultiplyOverflow(Instruction_t *p_orig, Instruction_t *p_instruction, Instruction_t* p_fallthrough);
};


} // end namespace

#define OVERFLOW_DETECTOR_64             "overflow_detector_64"
#define UNDERFLOW_DETECTOR_64            "underflow_detector_64"

#endif
