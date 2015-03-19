/*
 * Copyright (c) 2014 - Zephyr Software LLC
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
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#ifndef _MEDS_FUNCPROTOTYPEANNOTATION_H_
#define _MEDS_FUNCPROTOTYPEANNOTATION_H_

#include <string>
#include <vector>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_FuncAnnotation.hpp"


namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Must match STARS type system
//    UNINIT = 0,      // Operand type not yet analyzed; type lattice top
//    NUMERIC = 1,     // Definitely holds non-pointer value (char, int, etc.)
//    CODEPTR = 2,     // Definitely holds a code address; mmStrata considers this NUMERIC
//    POINTER = 4,     // Definitely holds a data address
//    STACKPTR = 8,    // Definitely holds a stack data address (refinement of POINTER)
//    GLOBALPTR = 16,   // Definitely holds a global static data address (refinement of POINTER)
//    HEAPPTR = 32,     // Definitely holds a heap data address (refinement of POINTER)
//    PTROFFSET = 64,   // Offset from a pointer, e.g. a difference between two pointers
//    NEGATEDPTR = 65,  // Temporary ptr arithmetic leaves NUMERIC-POINTER; should get +POINTER later to make PTROFFSET
//    UNKNOWN = 96,     // Might hold an address, might not (Bad!); type lattice bottom
//
typedef enum {
	MEDS_TYPE_UNINIT = 0,
	MEDS_TYPE_NUMERIC = 1, 
	MEDS_TYPE_CODEPTR = 2,
	MEDS_TYPE_POINTER = 4, 
	MEDS_TYPE_STACKPTR = 8, 
	MEDS_TYPE_GLOBALPTR = 16,
	MEDS_TYPE_HEAPPTR = 32, 
	MEDS_TYPE_PTROFFSET = 64,
	MEDS_TYPE_NEGATEDPTR = 65,
	MEDS_TYPE_UNKNOWN = 96     
} MEDS_ArgType;

// wrapper class around MEDS typing system
class MEDS_Arg {
	public:
		MEDS_Arg() { m_type = MEDS_TYPE_UNKNOWN; m_reg = Register::UNKNOWN; }
		MEDS_Arg(int p_type, Register::RegisterName p_reg = Register::UNKNOWN) { m_type = (MEDS_ArgType) p_type; m_reg = p_reg; }
		virtual ~MEDS_Arg() {}
		bool isNumericType() { return m_type == MEDS_TYPE_NUMERIC; }
		bool isPointerType() { 
			return m_type == MEDS_TYPE_CODEPTR || m_type == MEDS_TYPE_POINTER || 
				m_type == MEDS_TYPE_STACKPTR || m_type == MEDS_TYPE_GLOBALPTR || 
				m_type == MEDS_TYPE_HEAPPTR;
		}
		bool isUnknownType() { return !(isNumericType() || isPointerType()); }

	private:
		MEDS_ArgType m_type;
		Register::RegisterName m_reg;
};

//
// Class to handle one function prototype annotations
//
class MEDS_FuncPrototypeAnnotation : public MEDS_FuncAnnotation
{
	public:
		MEDS_FuncPrototypeAnnotation() { init(); };
		MEDS_FuncPrototypeAnnotation(const string &p_rawLine);
		virtual ~MEDS_FuncPrototypeAnnotation(){}
		virtual bool isFuncPrototypeAnnotation() const { return true; }

		virtual const string toString() const { return "func proto: " + m_rawInputLine; }

		int getNumArgs() const { return m_arguments ? m_arguments->size() : 0; }
		std::vector<MEDS_Arg>* getArgs() { return m_arguments; }
		void addArg(MEDS_Arg p_arg) { 
			if (!m_arguments) m_arguments = new std::vector<MEDS_Arg>;
			m_arguments->push_back(p_arg);
		}
		void setReturnArg(MEDS_Arg p_arg) { 
			if (!m_returnArg) m_returnArg = new MEDS_Arg;
			*m_returnArg = p_arg; 
		}
		MEDS_Arg* getReturnArg() const { return m_returnArg; }

	private:
		void init();
		void parse();

	private:
		string m_rawInputLine;

		std::vector<MEDS_Arg> *m_arguments;
		MEDS_Arg *m_returnArg;
};

}

#endif
