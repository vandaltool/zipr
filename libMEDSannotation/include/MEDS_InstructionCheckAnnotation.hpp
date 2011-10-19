#ifndef _MEDS_INSTRUCTIONCHECKANNOTATION_H_
#define _MEDS_INSTRUCTIONCHECKANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"

#define MEDS_ANNOT_INSTR        "INSTR"
#define MEDS_ANNOT_CHECK        "CHECK"
#define MEDS_ANNOT_OVERFLOW     "OVERFLOW"
#define MEDS_ANNOT_UNDERFLOW    "UNDERFLOW"
#define MEDS_ANNOT_SIGNEDNESS   "SIGNEDNESS"
#define MEDS_ANNOT_TRUNCATION   "TRUNCATION"
#define MEDS_ANNOT_SIGNED       "SIGNED"
#define MEDS_ANNOT_UNSIGNED     "UNSIGNED"

//
// Class to handle one MEDS (integer vulnerability) annotation
//
class MEDS_InstructionCheckAnnotation
{
	public:
		MEDS_InstructionCheckAnnotation() { m_isValid = false; }
		MEDS_InstructionCheckAnnotation(const std::string &p_rawLine);

		// valid annotation?
		bool isValid() const { return m_isValid; }

		// integer vulnerability types
		bool isOverflow() const { return m_isOverflow; }
		bool isUnderflow() const { return m_isUnderflow; }
		bool isTruncation() const { return m_isTruncation; }
		bool isSignedness() const { return m_isSignedness; }

		// signed vs. unsigned
		bool isUnsigned() const { return m_isUnsigned; }
		bool isSigned() const { return m_isSigned; }

		// virtual offset
		VirtualOffset getVirtualOffset() const;

	private:
		void parse();

	private:
		std::string    m_rawInputLine;
		bool           m_isOverflow;
		bool           m_isUnderflow;
		bool           m_isSignedness;
		bool           m_isTruncation;
		bool           m_isSigned;
		bool           m_isUnsigned;
		VirtualOffset  m_virtualOffset;
		bool           m_isValid;
};

#endif
