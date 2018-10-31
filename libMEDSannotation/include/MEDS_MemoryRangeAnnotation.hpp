/*
 * Copyright (c) 2018 - University of Virginia
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.edu
 *
 */

#ifndef _MEDS_MEMORYRANGEANNOTATION_H_
#define _MEDS_MEMORYRANGEANNOTATION_H_

#include <algorithm>
#include <string>
#include <stdint.h>
#include "VirtualOffset.hpp"
#include "MEDS_ShadowAnnotation.hpp"
#include "MEDS_Register.hpp"

namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

#define MEDS_ANNOT_STATICMEMWRITE "STATICMEMWRITE"
#define MEDS_ANNOT_STACKMEMRANGE "STACKMEMRANGE"

//
// Class to handle one MEDS memory range annotation
//
// Examples:
//    417748     12 INSTR STATICMEMWRITE MIN 3c60320  LIMIT 4e53730  ZZ
//     Meaning: instruction at 0x417748 is 12 bytes in length and writes to
//      a static memory data scoop includes the MIN address 0x3c60320.
//      The LIMIT address 0x4e53730 is beyond the range written to by
//      this instruction. The assembly language instruction is:
//
//      mov 0x3c3d630[RAX + RDX*8], 0 ; zero out buffer in 434.zeusmp
//
//      Note that 0x3c3d630 in the assembly language could trick an
//      IRDB transform, such as the move_globals defense, into thinking
//      that the instruction writes to the data scoop containing address
//      0x3c3d630, when in fact the initial value of register RDX is large
//      enough to push the initial memory address up to 0x3c60320, which is
//      a different data scoop in global static memory. The annotation provides
//      enough info to associate the instruction with the data scoop written.
//
//    4992ea      4 INSTR STACKMEMRANGE MIN RSP-568 LIMIT RSP-48 INSTRSPDELTA -592 ZZ
//     Meaning: instruction at 0x4992ea is 4 bytes in length and writes
//      to a stack memory location from RSP-568 to just below RSP-48, where
//      the stack offsets are relative to the value of the stack pointer on
//      function entry. The stack pointer has a current value at this instruction
//      that is -592 from the function entry value. This means that the instruction
//      writes a minimum of 24 bytes higher than the current stack pointer value.
//      The assembly language instruction is:
//
//      mov 0x8[R8],RCX   ; store RCX into [R8]+8 in busybox.psexe
//
//      It is not apparent from the assembly language that stack memory is involved,
//      but STARS loop analysis and symbolic analysis have determined that the memory
//      write is to the range [MIN..LIMIT-1] as seen in the annotation.
//

class MEDS_MemoryRangeAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_MemoryRangeAnnotation();
		MEDS_MemoryRangeAnnotation(const string& p_rawLine);
		virtual ~MEDS_MemoryRangeAnnotation() {};
		virtual const string toString() const { return "Memory Range: " + m_rawInputLine; };

		uint64_t getRangeMin() const { return m_rangeMin; };
		uint64_t getRangeLimit() const { return m_rangeLimit; };

		const bool isStackRange() const { return m_stackRange; };
		const bool isStaticGlobalRange() const { return m_staticGlobalRange; };

	private:  // methods
		void parse();
		void setStackRange(const bool p_val) { m_stackRange = p_val; };
		void setStaticGlobalRange(const bool p_val) { m_staticGlobalRange = p_val; };
		void setRangeMin(const uint64_t p_val) { m_rangeMin = p_val; };
		void setRangeLimit(const uint64_t p_val) { m_rangeLimit = p_val; };

	private: // data
		string m_rawInputLine;
		bool m_stackRange;
		bool m_staticGlobalRange;
		uint64_t m_rangeMin;
		uint64_t m_rangeLimit;
};

}

#endif
