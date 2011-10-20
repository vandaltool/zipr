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
		void addOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation);

		virtual_offset_t getAvailableAddress(VariantIR_t *p_virp);

		// utility functions
		bool isMultiplyInstruction32(libIRDB::Instruction_t*);
		bool isAddSubNonEspInstruction32(libIRDB::Instruction_t*);

	private:
		VariantID_t            *m_variantID;
		VariantIR_t            *m_variantIR;
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *m_annotations;
		set<std::string>       *m_filteredFunctions;
};

// make sure these match the function names in $STRATA/src/posix/x86_linux/detector_number_handling/overflow_detector.c

#define	INTEGER_OVERFLOW_DETECTOR            "integer_overflow_detector"
#define	ADDSUB_OVERFLOW_DETECTOR_SIGNED_32   "addsub_overflow_detector_signed_32"
#define	ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_32 "addsub_overflow_detector_unsigned_32"
#define	MUL_OVERFLOW_DETECTOR_32             "mul_overflow_detector_32"

#endif
