#ifndef _MEDS_PROBLEMFUNCANNOTATION_H_
#define _MEDS_PROBLEMFUNCANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_AnnotationBase.hpp"


namespace MEDS_Annotation 
{

// These strings must match those emitted by MEDS in the information annotation file exactly
#define MEDS_ANNOT_FUNC        "FUNC"

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS (integer vulnerability) annotation
//
class MEDS_ProblemFuncAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_ProblemFuncAnnotation() {};
		MEDS_ProblemFuncAnnotation(const string &p_rawLine);
		virtual ~MEDS_ProblemFuncAnnotation(){}

		virtual const string toString() const { return "problem func: "+m_rawInputLine; }

		virtual void markCallUnresolved() { m_call_unresolved = true; setValid(); }
		virtual void markJumpUnresolved() { m_jump_unresolved = true; setValid(); }
		virtual bool isCallUnresolved() { return m_call_unresolved; }
		virtual bool isJumpUnresolved() { return m_jump_unresolved; }

	private:
		void init();
		void parse();

	private:
		std::string m_rawInputLine;

		bool m_call_unresolved;
		bool m_jump_unresolved;
};

}
#endif
