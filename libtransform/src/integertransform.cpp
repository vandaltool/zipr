#include <assert.h>
#include "integertransform.hpp"
#include "leapattern.hpp"

// 
// For list of blacklisted functions, see: isBlacklisted()
//

using namespace libTransform;

IntegerTransform::IntegerTransform(VariantID_t *p_variantID, FileIR_t *p_fileIR, std::multimap<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : Transform(p_variantID, p_fileIR, p_filteredFunctions) 
{
	m_benignFalsePositives = p_benignFalsePositives;
	m_policySaturatingArithmetic = false;
	m_policyWarningsOnly = false;
	m_pathManipulationDetected = false;

	m_annotations = p_annotations;              

	m_numAnnotations = 0;
	m_numIdioms = 0;
	m_numBlacklisted = 0;
	m_numBenign = 0;
	m_numOverflows = 0;
	m_numUnderflows = 0;
	m_numTruncations = 0;
	m_numSignedness = 0;
	m_numFP = 0;
	m_numOverflowsSkipped = 0;
	m_numUnderflowsSkipped = 0;
	m_numTruncationsSkipped = 0;
	m_numSignednessSkipped = 0;
}

bool IntegerTransform::isBlacklisted(Function_t *func)
{
	if (!func) return false;

	const char *funcName = func->GetName().c_str();
	return (strcasestr(funcName, "hash") ||
		strcasestr(funcName, "compress") ||
		strcasestr(funcName, "encode") ||
		strcasestr(funcName, "decode") ||
		strcasestr(funcName, "crypt") ||
		strcasestr(funcName, "yyparse") ||
		strcasestr(funcName, "yyerror") ||
		strcasestr(funcName, "yydestruct") ||
		strcasestr(funcName, "yyrestart") ||
		strcasestr(funcName, "yylex") ||
		strcasestr(funcName, "yy_"));
}

void IntegerTransform::logStats()
{
	std::string fileURL = getFileIR()->GetFile()->GetURL();	

	std::cerr << "# ATTRIBUTE file_name=" << fileURL << std::endl;
	std::cerr << "# ATTRIBUTE num_annotations_processed=" << dec << m_numAnnotations << std::endl;
	std::cerr << "# ATTRIBUTE num_idioms=" << m_numIdioms << std::endl;
	std::cerr << "# ATTRIBUTE num_blacklisted=" << m_numBlacklisted << std::endl;
	std::cerr << "# ATTRIBUTE num_benign=" << m_numBenign << std::endl;
	std::cerr << "# ATTRIBUTE num_overflows_instrumented=" << m_numOverflows << std::endl;
	std::cerr << "# ATTRIBUTE num_overflows_skipped=" << m_numOverflowsSkipped << std::endl;
	std::cerr << "# ATTRIBUTE num_underflows_instrumented=" << m_numUnderflows << std::endl;
	std::cerr << "# ATTRIBUTE num_underflows_skipped=" << m_numUnderflowsSkipped << std::endl;
	std::cerr << "# ATTRIBUTE num_truncations_instrumented=" << m_numTruncations << std::endl;
	std::cerr << "# ATTRIBUTE num_truncations_skipped=" << m_numTruncationsSkipped << std::endl;
	std::cerr << "# ATTRIBUTE num_signedness_instrumented=" << m_numSignedness << std::endl;
	std::cerr << "# ATTRIBUTE num_signedness_skipped=" << m_numSignednessSkipped << std::endl;
	std::cerr << "# ATTRIBUTE num_floating_point=" << m_numFP << std::endl;
}
