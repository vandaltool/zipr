#ifndef _LIBTRANSFORM_INTEGERTRANSFORM_H_
#define _LIBTRANSFORM_INTEGERTRANSFORM_H_

#include "transform.hpp"
#include "MEDS_Register.hpp"
#include "VirtualOffset.hpp"

namespace libTransform
{

using namespace std;
using namespace libIRDB;

class IntegerTransform : public Transform
{
	public:
		IntegerTransform(VariantID_t *, FileIR_t*, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_warnings); 
		virtual int execute() = 0;

		void setSaturatingArithmetic(bool p_satArithmetic) { m_policySaturatingArithmetic = p_satArithmetic; }
		bool isSaturatingArithmetic() { return m_policySaturatingArithmetic; }
		void setPathManipulationDetected(bool p_pathManip) { m_pathManipulationDetected = p_pathManip; }
		bool isPathManipulationDetected() { return m_pathManipulationDetected; }
		void setWarningsOnly(bool p_warn) { m_policyWarningsOnly = p_warn; }
		bool isWarningsOnly() { return m_policyWarningsOnly; }
		void logStats();
		bool isBlacklisted(Function_t *func);
	
	protected:
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation>* getAnnotations() { return m_annotations; }

/*
		void logMessage(const std::string &p_method, const std::string &p_msg);
		void logMessage(const std::string &p_method, const MEDS_InstructionCheckAnnotation&, const std::string &p_msg);
*/

		std::set<VirtualOffset>*  m_benignFalsePositives;
		bool                      m_policySaturatingArithmetic;
		bool                      m_policyWarningsOnly;
		bool                      m_pathManipulationDetected;
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *m_annotations;

		unsigned m_numAnnotations; 
		unsigned m_numIdioms; 
		unsigned m_numBlacklisted; 
		unsigned m_numBenign; 
		unsigned m_numOverflows; 
		unsigned m_numUnderflows; 
		unsigned m_numTruncations; 
		unsigned m_numSignedness; 
		unsigned m_numOverflowsSkipped; 
		unsigned m_numUnderflowsSkipped; 
		unsigned m_numTruncationsSkipped; 
		unsigned m_numSignednessSkipped; 
		unsigned m_numFP; 
};

}

#endif
