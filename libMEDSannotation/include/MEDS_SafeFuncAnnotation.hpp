#ifndef _MEDS_SAFEFUNCANNOTATION_H_
#define _MEDS_SAFEFUNCANNOTATION_H_

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
class MEDS_SafeFuncAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_SafeFuncAnnotation() {};
		MEDS_SafeFuncAnnotation(const string &p_rawLine);
		virtual ~MEDS_SafeFuncAnnotation(){}

		virtual const string& toString() const { return m_rawInputLine; }

		virtual void markSafe() { m_safe_func = true; setValid(); }
		virtual void markUnsafe() { m_safe_func = false; setValid(); }
		virtual bool isSafe() { return m_safe_func; }


	private:
		void init();
		void parse();

	private:
		std::string m_rawInputLine;

		bool m_safe_func;
};

}
#endif
