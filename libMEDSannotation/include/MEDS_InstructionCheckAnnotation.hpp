#ifndef _MEDS_INSTRUCTIONCHECKANNOTATION_H_
#define _MEDS_INSTRUCTIONCHECKANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"


namespace MEDS_Annotation 
{

// These strings must match those emitted by MEDS in the information annotation file exactly
#define MEDS_ANNOT_INSTR        "INSTR"
#define MEDS_ANNOT_CHECK        "CHECK"
#define MEDS_ANNOT_OVERFLOW     "OVERFLOW"
#define MEDS_ANNOT_UNDERFLOW    "UNDERFLOW"
#define MEDS_ANNOT_SIGNEDNESS   "SIGNEDNESS"
#define MEDS_ANNOT_TRUNCATION   "TRUNCATION"
#define MEDS_ANNOT_SIGNED       "SIGNED"
#define MEDS_ANNOT_UNSIGNED     "UNSIGNED"
#define MEDS_ANNOT_UNKNOWNSIGN  "UNKNOWNSIGN"
#define MEDS_ANNOT_NOFLAG       "NOFLAG"

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS (integer vulnerability) annotation
//
class MEDS_InstructionCheckAnnotation
{
	public:
		MEDS_InstructionCheckAnnotation();
		MEDS_InstructionCheckAnnotation(const string &p_rawLine);

		// valid annotation?
		bool isValid() const { return m_isValid; }
		void setValid() { m_isValid = true; }

		// integer vulnerability types
		bool isOverflow() const { return m_isOverflow; }
		bool isUnderflow() const { return m_isUnderflow; }
		bool isTruncation() const { return m_isTruncation; }
		bool isSignedness() const { return m_isSignedness; }
		void setOverflow() { m_isOverflow = true; }

		// signed vs. unsigned
		bool isUnsigned() const { return m_isUnsigned; }
		bool isSigned() const { return m_isSigned; }
		bool isUnknownSign() const { return m_isUnknownSign; }

		void setSigned() { m_isSigned = true; }
		void setUnsigned() { m_isUnsigned = true; }
		void setUnknownSign() { m_isUnknownSign = true; }

		// overflow with no flags, e.g. lea
		bool isNoFlag() const { return m_isNoFlag; }

		// get bitwidth
		int getBitWidth() const { return m_bitWidth; }
		void setBitWidth(int p_bitWidth) { m_bitWidth = p_bitWidth; }
		int getTruncationFromWidth() const { return m_truncationFromWidth; }
		int getTruncationToWidth() const { return m_truncationToWidth; }

		// get register
		MEDS_Annotation::Register::RegisterName getRegister() const { return m_register; }

		// virtual offset
		VirtualOffset getVirtualOffset() const;

		const string getTarget() const { return m_target; }

		const string& toString() const { return m_rawInputLine; }

	private:
		void init();
		void parse();

	private:
		string         m_rawInputLine;
		bool           m_isOverflow;
		bool           m_isUnderflow;
		bool           m_isSignedness;
		bool           m_isTruncation;
		bool           m_isSigned;
		bool           m_isUnsigned;
		bool           m_isUnknownSign;
		bool           m_isNoFlag;
		int            m_bitWidth;
		int            m_truncationFromWidth;
		int            m_truncationToWidth;
		VirtualOffset  m_virtualOffset;
		bool           m_isValid;
		MEDS_Annotation::Register::RegisterName       m_register;
		string         m_target;
};

}
#endif
