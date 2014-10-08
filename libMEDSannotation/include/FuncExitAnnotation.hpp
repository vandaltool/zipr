#ifndef _FUNCEXITANNOTATION_H_
#define _FUNCEXITANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_AnnotationBase.hpp"


namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS  annotation
//
class MEDS_FuncExitAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_FuncExitAnnotation() {};
		MEDS_FuncExitAnnotation(const string &p_rawLine);
		virtual ~MEDS_FuncExitAnnotation(){}

		virtual const string toString() const { return "tail call: "+m_rawInputLine; }

	private:
		void init();
		void parse();

	private:
		std::string m_rawInputLine;
};

}
#endif
