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

#include <irdb-transform>

using namespace IRDB_SDK;

Transform_t::Transform_t(FileIR_t *p_fileIR)
	:
	m_fileIR (p_fileIR) 
{
	assert(m_fileIR);
}

FileIR_t* Transform_t::getFileIR() 
{ 
	return m_fileIR; 
}

const FileIR_t* Transform_t::getFileIR() const
{ 
	return m_fileIR; 
}

Instruction_t* Transform_t::insertAssemblyBefore(Instruction_t* before, const string &the_asm, Instruction_t* target)
{
	return IRDB_SDK::insertAssemblyBefore(getFileIR(), before, the_asm, target);
}


Instruction_t* Transform_t::insertAssemblyAfter(Instruction_t* before, const string &the_asm, Instruction_t* target)
{
	return IRDB_SDK::insertAssemblyAfter(getFileIR(), before, the_asm, target);
}

Instruction_t* Transform_t::insertDataBitsBefore(Instruction_t* before, const string &the_asm, Instruction_t* target)
{
	return IRDB_SDK::insertDataBitsBefore(getFileIR(), before, the_asm, target);
}

Instruction_t* Transform_t::insertDataBitsAfter(Instruction_t* before, const string &the_asm, Instruction_t* target)
{
	return IRDB_SDK::insertDataBitsAfter(getFileIR(), before, the_asm, target);
}

vector<Instruction_t*> Transform_t::insertAssemblyInstructionsBefore(Instruction_t* before, string instructions, Instruction_t* target) {
	return IRDB_SDK::insertAssemblyInstructionsBefore(getFileIR(), before, instructions, target);
}

vector<Instruction_t*> Transform_t::insertAssemblyInstructionsAfter(Instruction_t* after, string instructions, Instruction_t* target) {
        return IRDB_SDK::insertAssemblyInstructionsAfter(getFileIR(), after, instructions, target);
}

Instruction_t* Transform_t::addNewDataBits(const string& p_bits)
{
	return IRDB_SDK::addNewDataBits(getFileIR(), p_bits);
}

Instruction_t* Transform_t::addNewAssembly(const string& p_bits)
{
	return IRDB_SDK::addNewAssembly(getFileIR(), p_bits);
}

void Transform_t::setInstructionAssembly(Instruction_t* instr, const string& p_asm, Instruction_t *p_new_fallthrough, Instruction_t* p_new_target)
{
	return IRDB_SDK::setInstructionAssembly(getFileIR(), instr, p_asm, p_new_fallthrough, p_new_target);
}

void Transform_t::setInstructionAssembly(Instruction_t* instr, const string& p_asm)
{
	return IRDB_SDK::setInstructionAssembly(getFileIR(), instr, p_asm);
}


