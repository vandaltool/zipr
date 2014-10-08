#ifndef _MEDS_FRSAFEANNOTATION_H_
#define _MEDS_FRSAFEANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_AnnotationBase.hpp"


namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS (integer vulnerability) annotation
//
class MEDS_FRSafeAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_FRSafeAnnotation() {};
		MEDS_FRSafeAnnotation(const string &p_rawLine);
		virtual ~MEDS_FRSafeAnnotation(){}

		virtual const string toString() const { return "fr safe func: "+m_rawInputLine; }

	private:
		void init();
		void parse();

	private:
		std::string m_rawInputLine;
};

}
#endif
