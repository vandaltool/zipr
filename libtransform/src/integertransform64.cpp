#include <assert.h>

#include "integertransform64.hpp"

using namespace libTransform;

IntegerTransform64::IntegerTransform64(VariantID_t *p_variantID, FileIR_t *p_fileIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : Transform(p_variantID, p_fileIR, p_annotations, p_filteredFunctions) 
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

// iterate through all functions
// filter those functions that should be ignored
//    iterate through all instructions in function
//    if MEDS annotation says to instrument
//       add instrumentation
int IntegerTransform64::execute()
{
	if (isWarningsOnly())
		logMessage(__func__, "warnings only mode");

	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;

		if (getFilteredFunctions()->find(func->GetName()) != getFilteredFunctions()->end())
		{
			logMessage(__func__, "filter out: " + func->GetName());
			continue;
		}

		if (isBlacklisted(func))
		{
			logMessage(__func__, "blacklisted: " + func->GetName());
			m_numBlacklisted++;
			continue;
		}

		logMessage(__func__, "processing fn: " + func->GetName());

		for(
		  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  it!=func->GetInstructions().end();
		  ++it)
		{
			Instruction_t* insn=*it;

			if (insn && insn->GetAddress())
			{
				int policy = POLICY_EXIT; //  default for now is exit -- no callback handlers yet
				virtual_offset_t irdb_vo = insn->GetAddress()->GetVirtualOffset();
				if (irdb_vo == 0) continue;

				VirtualOffset vo(irdb_vo);

				MEDS_InstructionCheckAnnotation annotation = (*getAnnotations())[vo];
				if (!annotation.isValid()) 
					continue;

				logMessage(__func__, annotation, "-- instruction: " + insn->getDisassembly());
				m_numAnnotations++;

				if (annotation.isIdiom())
				{
					logMessage(__func__, "skip IDIOM");
					m_numIdioms++;
					continue;
				}

				if (!insn->GetFallthrough())
				{
					logMessage(__func__, "Warning: no fall through for instruction -- skipping");
					continue;
				}

				if (annotation.isOverflow())
				{
					// nb: safe with respect to esp (except for lea)
					handleOverflowCheck(insn, annotation, policy);
				}
			}
		} // end iterate over all instructions in a function
	} // end iterate over all functions

	return 0;
}

void IntegerTransform64::handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (isMultiplyInstruction(p_instruction) || (p_annotation.isOverflow() && !p_annotation.isUnknownSign()))
	{
		// handle signed/unsigned add/sub overflows (non lea)
		addOverflowCheck(p_instruction, p_annotation, p_policy);
	}
	else
	{
		m_numOverflowsSkipped++;
		logMessage(__func__, "OVERFLOW type not yet handled");
	}
}

//
//		mul a, b                 ; <instruction to instrument>
//		jno <OrigNext>
//      	halt
// OrigNext:	<nextInstruction>
//
void IntegerTransform64::addOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	Instruction_t* jncond_i = NULL;
	Instruction_t* hlt_i = NULL;

	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());

	cerr << __func__ <<  ": instr: " << p_instruction->getDisassembly() << " address: " << std::hex << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;

	jncond_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	hlt_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

	Instruction_t* next_i = p_instruction->GetFallthrough();
	p_instruction->SetFallthrough(jncond_i); 

	if (p_annotation.isUnsigned())
	{
		addJnc(jncond_i, hlt_i, next_i);
	}
	else
	{
		addJno(jncond_i, hlt_i, next_i);
	}
	addHlt(hlt_i, next_i);

	if (p_annotation.isUnderflow())
		m_numUnderflows++;
	else
		m_numOverflows++;
}

// @todo: move to base class
void IntegerTransform64::logMessage(const std::string &p_method, const std::string &p_msg)
{
	std::cerr << p_method << ": " << p_msg << std::endl;
}

// @todo: move to base class
void IntegerTransform64::logMessage(const std::string &p_method, const MEDS_InstructionCheckAnnotation& p_annotation, const std::string &p_msg)
{
	logMessage(p_method, p_msg + " annotation: " + p_annotation.toString());
}

void IntegerTransform64::logStats()
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

// functions known to be problematic b/c of bitwise manipulation
bool IntegerTransform64::isBlacklisted(Function_t *func)
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

