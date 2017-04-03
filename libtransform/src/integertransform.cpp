/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <assert.h>
#include "integertransform.hpp"
#include "leapattern.hpp"


/*
 * Find the first occurrence of find in s, ignore case.
 */
static char *
my_strcasestr(const char* s, const char *find)
{
        char c, sc;
        size_t len;

        if ((c = *find++) != 0) {
                c = tolower((unsigned char)c);
                len = strlen(find);
                do {
                        do {
                                if ((sc = *s++) == 0)
                                        return (NULL);
                        } while ((char)tolower((unsigned char)sc) != c);
                } while (strncasecmp(s, find, len) != 0);
                s--;
        }
        return ((char *)s);
}


// 
// For list of blacklisted functions, see: isBlacklisted()
//

using namespace libTransform;

IntegerTransform::IntegerTransform(VariantID_t *p_variantID, FileIR_t *p_fileIR, 
// std::multimap<VirtualOffset, MEDS_AnnotationBase> 
MEDS_Annotations_t *p_annotations, 
set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : Transform(p_variantID, p_fileIR, p_filteredFunctions) 
{
	m_benignFalsePositives = p_benignFalsePositives;
	m_policySaturatingArithmetic = false;
	m_policyWarningsOnly = false;
	m_pathManipulationDetected = false;
	m_instrumentIdioms = false;

	m_annotations = p_annotations;              

	m_numAnnotations = 0;
	m_numIdioms = 0;
	m_numBlacklisted = 0;
	m_numBenign = 0;
	m_numFP = 0;

	m_numTotalOverflows = 0;
	m_numTotalUnderflows = 0;
	m_numTotalTruncations = 0;
	m_numTotalSignedness = 0;

	m_numOverflows = 0;
	m_numUnderflows = 0;
	m_numTruncations = 0;
	m_numSignedness = 0;

	m_numOverflowsSkipped = 0;
	m_numUnderflowsSkipped = 0;
	m_numTruncationsSkipped = 0;
	m_numSignednessSkipped = 0;

	m_instrumentSP = false;
	m_instrumentFP = false;
}

bool IntegerTransform::isBlacklisted(Function_t *func)
{
	if (!func) return false;

	const char *funcName = func->GetName().c_str();
	return (my_strcasestr(funcName, "hash") ||
		my_strcasestr(funcName, "compress") ||
		my_strcasestr(funcName, "encode") ||
		my_strcasestr(funcName, "decode") ||
		my_strcasestr(funcName, "crypt") ||
		my_strcasestr(funcName, "yyparse") ||
		my_strcasestr(funcName, "yyerror") ||
		my_strcasestr(funcName, "yydestruct") ||
		my_strcasestr(funcName, "yyrestart") ||
		my_strcasestr(funcName, "yylex") ||
		my_strcasestr(funcName, "yyparse") ||
		my_strcasestr(funcName, "yyerror") ||
		my_strcasestr(funcName, "yydestruct") ||
		my_strcasestr(funcName, "yyrestart") ||
		my_strcasestr(funcName, "yylex") ||
		my_strcasestr(funcName, "yy_"));
}

void IntegerTransform::logStats()
{
	std::string fileURL = getFileIR()->GetFile()->GetURL();	

	std::cerr << "# ATTRIBUTE file_name=" << fileURL << std::endl;
	std::cerr << "# ATTRIBUTE num_annotations_processed=" << dec << m_numAnnotations << std::endl;
	std::cerr << "# ATTRIBUTE num_idioms=" << m_numIdioms << std::endl;
	std::cerr << "# ATTRIBUTE num_blacklisted=" << m_numBlacklisted << std::endl;
	std::cerr << "# ATTRIBUTE num_benign=" << m_numBenign << std::endl;

	std::cerr << "# ATTRIBUTE num_total_overflows=" << m_numTotalOverflows << std::endl;
	std::cerr << "# ATTRIBUTE num_overflows_instrumented=" << m_numOverflows << std::endl;
	std::cerr << "# ATTRIBUTE num_overflows_skipped=" << m_numOverflowsSkipped << std::endl;
	std::cerr << "# ATTRIBUTE overflows_coverage=" << (double)m_numOverflows/(double)m_numTotalOverflows << std::endl;

	std::cerr << "# ATTRIBUTE num_total_underflows=" << m_numTotalUnderflows << std::endl;
	std::cerr << "# ATTRIBUTE num_underflows_instrumented=" << m_numUnderflows << std::endl;
	std::cerr << "# ATTRIBUTE num_underflows_skipped=" << m_numUnderflowsSkipped << std::endl;
	std::cerr << "# ATTRIBUTE underflows_coverage=" << (double)m_numUnderflows/(double)m_numTotalUnderflows << std::endl;

	std::cerr << "# ATTRIBUTE num_total_truncations=" << m_numTotalTruncations << std::endl;
	std::cerr << "# ATTRIBUTE num_truncations_instrumented=" << m_numTruncations << std::endl;
	std::cerr << "# ATTRIBUTE num_truncations_skipped=" << m_numTruncationsSkipped << std::endl;
	std::cerr << "# ATTRIBUTE truncation_coverage=" << (double)m_numTruncations/(double)m_numTotalTruncations << std::endl;

	std::cerr << "# ATTRIBUTE num_total_signedness=" << m_numTotalSignedness << std::endl;
	std::cerr << "# ATTRIBUTE num_signedness_instrumented=" << m_numSignedness << std::endl;
	std::cerr << "# ATTRIBUTE num_signedness_skipped=" << m_numSignednessSkipped << std::endl;
	std::cerr << "# ATTRIBUTE signedness_coverage=" << (double)m_numSignedness/(double)m_numTotalSignedness << std::endl;

	std::cerr << "# ATTRIBUTE num_floating_point=" << m_numFP << std::endl;
}
