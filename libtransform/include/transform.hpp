#ifndef _LIBTRANSFORM_TRANSFORM_H
#define _LIBTRANSFORM_TRANSFORM_H

#include <string>
#include <set>
#include <map>

#include <libIRDB-core.hpp>


#include "MEDS_InstructionCheckAnnotation.hpp"
#include "VirtualOffset.hpp"

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;

namespace libTransform
{

class Transform {

	public:
		Transform(VariantID_t *, VariantIR_t *, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions);

	protected:

		void addInstruction(Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target);
	    Instruction_t* carefullyInsertBefore(Instruction_t* &p_target, Instruction_t* &p_new);

		void addPushRegister(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addPopRegister(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addPusha(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addPushf(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addPopa(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addPopf(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addNop(Instruction_t *p_instr, Instruction_t *p_fallThrough);

		void addCallbackHandler(string p_detector, Instruction_t *p_instrumented, Instruction_t *p_instr, Instruction_t *_fallThrough);
		void addTestRegister(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addJns(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);

		Instruction_t* allocateNewInstruction(db_id_t p_fileID, Function_t* p_func);

		virtual_offset_t getAvailableAddress();

		VariantID_t* getVariantID() { return m_variantID; }
		VariantIR_t* getVariantIR() { return m_variantIR; }
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation>* getAnnotations() { return m_annotations; }
		set<std::string>* getFilteredFunctions() { return m_filteredFunctions; }

		bool isMultiplyInstruction(libIRDB::Instruction_t*);
		bool isAddSubNonEspInstruction(libIRDB::Instruction_t*);

	private:
		void addTestRegister8(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addTestRegister16(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addTestRegister32(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);

		VariantID_t            *m_variantID;
		VariantIR_t            *m_variantIR;
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *m_annotations;
		set<std::string>       *m_filteredFunctions;
};

}

#endif
