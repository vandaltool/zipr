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

		void addCallbackHandler(string p_detector, Instruction_t *p_instrumented, Instruction_t *p_instr, Instruction_t *p_fallThrough, int p_policy, AddressID_t *addressOriginal = NULL);

		void addTestRegister(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addTestRegisterMask(Instruction_t *p_instr, Register::RegisterName, unsigned p_mask, Instruction_t *p_fallThrough);

		void addJns(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJz(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addNot(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);

		void addAddRegisters(Instruction_t *p_instr, Register::RegisterName p_regTgt, Register::RegisterName p_regSrc, Instruction_t *p_fallThrough);
		void addAddRegisterConstant(Instruction_t *p_instr, Register::RegisterName p_regTgt, int p_constantValue, Instruction_t *p_fallThrough);
		void addMulRegisterConstant(Instruction_t *p_instr, Register::RegisterName p_regTgt, int p_constantValue, Instruction_t *p_fallThrough);
		void addMovRegisters(Instruction_t *p_instr, Register::RegisterName p_regTgt, Register::RegisterName p_regSrc, Instruction_t *p_fallThrough);

		Instruction_t* allocateNewInstruction(db_id_t p_fileID, Function_t* p_func);

		virtual_offset_t getAvailableAddress();

		VariantID_t* getVariantID() { return m_variantID; }
		VariantIR_t* getVariantIR() { return m_variantIR; }
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation>* getAnnotations() { return m_annotations; }
		set<std::string>* getFilteredFunctions() { return m_filteredFunctions; }

		bool isMultiplyInstruction(libIRDB::Instruction_t*);
		bool isAddSubNonEspInstruction(libIRDB::Instruction_t*);
		Register::RegisterName getTargetRegister(libIRDB::Instruction_t*);

		void addMovRegisterUnsignedConstant(Instruction_t *p_instr, Register::RegisterName p_regTgt, unsigned p_constant, Instruction_t *p_fallThrough);
		void addMovRegisterSignedConstant(Instruction_t *p_instr, Register::RegisterName p_regTgt, int p_constant, Instruction_t *p_fallThrough);

	private:
		void addTestRegister8(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addTestRegister16(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addTestRegister32(Instruction_t *p_instr, Register::RegisterName, Instruction_t *p_fallThrough);
		void addTestRegisterMask32(Instruction_t *p_instr, Register::RegisterName, unsigned p_mask, Instruction_t *p_fallThrough);
		bool hasTargetRegister(libIRDB::Instruction_t*);

		VariantID_t            *m_variantID;
		VariantIR_t            *m_variantIR;
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *m_annotations;
		set<std::string>       *m_filteredFunctions;
};

// make sure these match values in detector_handlers.h in the strata library
#define POLICY_DEFAULT                            0   
#define POLICY_CONTINUE                           1  
#define POLICY_EXIT                               2   
#define POLICY_CONTINUE_SATURATING_ARITHMETIC     3   

}

#endif

