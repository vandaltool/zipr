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
		void addOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy);
};


} // end namespace

#endif
