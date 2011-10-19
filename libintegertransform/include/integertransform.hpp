#ifndef _INTEGER_TRANSFORM_H_
#define _INTEGER_TRANSFORM_H_

#include <string>
#include <set>
#include <map>

#include <libIRDB-core.hpp>
#include "MEDS_InstructionCheckAnnotation.hpp"
#include "VirtualOffset.hpp"

using namespace std;
using namespace libIRDB;

class IntegerTransform
{
	public:
		IntegerTransform(VariantID_t *, VariantIR_t*, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions); 
		int execute();
	
	private:
		void addOverflowCheck(libIRDB::Instruction_t *p_instruction, std::string p_handlerName);
		virtual_offset_t getAvailableAddress(VariantIR_t *p_virp);

	private:
		VariantID_t            *m_variantID;
		VariantIR_t            *m_variantIR;
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *m_annotations;
		set<std::string>       *m_filteredFunctions;
};

#endif
