#ifndef _MEDS_FUNCANNOTATION_H_
#define _MEDS_FUNCANNOTATION_H_

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
class MEDS_FuncAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_FuncAnnotation() {}
		virtual ~MEDS_FuncAnnotation(){}

                virtual bool isFuncAnnotation() const { return true; } 

		virtual string getFuncName() const { return m_func_name; }
		virtual void setFuncName(const string &p_func_name) { m_func_name=p_func_name; }

	private:
		string m_func_name;
};

}
#endif
