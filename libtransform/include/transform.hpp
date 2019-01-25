/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#ifndef _LIBTRANSFORM_TRANSFORM_H
#define _LIBTRANSFORM_TRANSFORM_H

#include <string>
#include <set>
#include <map>

#include <irdb-core>

#include "MEDS_InstructionCheckAnnotation.hpp"
#include "VirtualOffset.hpp"

#define MAX_ASSEMBLY_SIZE 2048

namespace libIRDB
{
	class FileIR_t;
}
namespace libTransform
{
using namespace std;
using namespace IRDB_SDK;
using namespace MEDS_Annotation;


class Transform 
{

	public:
		Transform(VariantID_t *, FileIR_t *, set<std::string> *p_filteredFunctions);

	protected:
		void setAssembly(Instruction_t *p_instr, string p_asm);
		Instruction_t* addNewAssembly(string p_asm);
		Instruction_t* addNewAssembly(Instruction_t *p_instr, string p_asm);
		Instruction_t* registerCallbackHandler64(string p_callbackHandler, int p_numArgs);
		void addCallbackHandler64(Instruction_t *p_instr, string p_callbackHandler, int p_numArgs);
		void addInstruction(Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target);
		Instruction_t* carefullyInsertBefore(Instruction_t* &p_target, Instruction_t* &p_new);
		Instruction_t* insertAssemblyBefore(Instruction_t* before, const string &the_asm, Instruction_t* target=nullptr);
		Instruction_t* insertAssemblyAfter(Instruction_t* after, const string &the_asm, Instruction_t* target=nullptr);

		void addPushRegister(Instruction_t *p_instr, RegisterName, Instruction_t *p_fallThrough);
		void addPopRegister(Instruction_t *p_instr, RegisterName, Instruction_t *p_fallThrough);
		void addPusha(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addPushf(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addPopa(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addPopf(Instruction_t *p_instr, Instruction_t *p_fallThrough);
		void addNop(Instruction_t *p_instr, Instruction_t *p_fallThrough);

		void addCallbackHandler(string p_detector, Instruction_t *p_instrumented, Instruction_t *p_instr, Instruction_t *p_fallThrough, int p_policy, AddressID_t *addressOriginal = NULL);

		void addTestRegister(Instruction_t *p_instr, RegisterName, Instruction_t *p_fallThrough);
		void addTestRegisterMask(Instruction_t *p_instr, RegisterName, unsigned p_mask, Instruction_t *p_fallThrough);
		void addCmpRegisterMask(Instruction_t *p_instr, RegisterName, unsigned p_mask, Instruction_t *p_fallThrough);

		void addJns(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJz(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJo(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJno(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJc(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJnc(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJnz(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addJae(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target);
		void addNot(Instruction_t *p_instr, RegisterName, Instruction_t *p_fallThrough);
		void addHlt(Instruction_t *p_instr, Instruction_t *p_fallThrough);

		void addAddRegisters(Instruction_t *p_instr, RegisterName p_regTgt, RegisterName p_regSrc, Instruction_t *p_fallThrough);
		void addAddRegisterConstant(Instruction_t *p_instr, RegisterName p_regTgt, int p_constantValue, Instruction_t *p_fallThrough);
		void addMulRegisterConstant(Instruction_t *p_instr, RegisterName p_regTgt, int p_constantValue, Instruction_t *p_fallThrough);
		void addMovRegisters(Instruction_t *p_instr, RegisterName p_regTgt, RegisterName p_regSrc, Instruction_t *p_fallThrough);

		Instruction_t* allocateNewInstruction(DatabaseID_t p_fileID=BaseObj_t::NOT_IN_DATABASE, Function_t* p_func=NULL);

		VirtualOffset_t getAvailableAddress();

		VariantID_t* getVariantID() { return m_variantID; }
		FileIR_t* getFileIR(); //  { return dynamic_cast<FileIR_t*>(m_fileIR); }
		set<std::string>* getFilteredFunctions() { return m_filteredFunctions; }

		Instruction_t* addNewMaxSaturation(Instruction_t *p_prev, RegisterName p_reg, const MEDS_InstructionCheckAnnotation p_annotation);
		void addMinSaturation(Instruction_t *p_instruction, RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough);
		void addMaxSaturation(Instruction_t *p_instruction, RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough);
		void addMovRegisterUnsignedConstant(Instruction_t *p_instr, RegisterName p_regTgt, unsigned long p_constant, Instruction_t *p_fallThrough);
		void addMovRegisterSignedConstant(Instruction_t *p_instr, RegisterName p_regTgt, long int p_constant, Instruction_t *p_fallThrough);
		void addAndRegister32Mask(Instruction_t *p_instr, RegisterName p_regTgt, unsigned int p_mask, Instruction_t *p_fallThrough);

	protected:
		void logMessage(const std::string &p_method, const std::string &p_msg);
		void logMessage(const std::string &p_method, const MEDS_InstructionCheckAnnotation&, const std::string &p_msg);

	private:
		void addTestRegister8(Instruction_t *p_instr, RegisterName, Instruction_t *p_fallThrough);
		void addTestRegister16(Instruction_t *p_instr, RegisterName, Instruction_t *p_fallThrough);
		void addTestRegister32(Instruction_t *p_instr, RegisterName, Instruction_t *p_fallThrough);
		void addTestRegisterMask32(Instruction_t *p_instr, RegisterName, unsigned p_mask, Instruction_t *p_fallThrough);
		void addCmpRegisterMask32(Instruction_t *p_instr, RegisterName, unsigned p_mask, Instruction_t *p_fallThrough);

		VariantID_t 		*m_variantID;
		libIRDB::FileIR_t       *m_fileIR;
		set<std::string>	*m_filteredFunctions;
		std::map<std::string, Instruction_t*> m_handlerMap;
};

// make sure these match values in detector_handlers.h in the strata library
#define POLICY_DEFAULT                            0   
#define POLICY_CONTINUE                           1  
#define POLICY_EXIT                               2   
#define POLICY_CONTINUE_SATURATING_ARITHMETIC     3   

#define DEBUG_CALLBACK_HANDLER "debug_handler"

// utility function
void convertToLowercase(string&);

}

#endif

