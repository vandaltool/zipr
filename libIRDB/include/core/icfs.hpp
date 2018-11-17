/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

class Instruction_t;
typedef std::set<Instruction_t*> InstructionSet_t;

typedef enum ICFS_Analysis_Status_t { ICFS_Analysis_Incomplete, ICFS_Analysis_Module_Complete, ICFS_Analysis_Complete } ICFS_Analysis_Status_t; 

// these must match the allowed values for type icfs_analysis_result in Postgres DB
#define ICFS_ANALYSIS_INCOMPLETE_STR "icfs_analysis_incomplete"
#define ICFS_ANALYSIS_MODULE_COMPLETE_STR "icfs_analysis_module_complete"
#define ICFS_ANALYSIS_COMPLETE_STR "icfs_analysis_complete"

// Keep track of instruction control flow sets
class ICFS_t : public InstructionSet_t, public BaseObj_t
{
	public:
		ICFS_t(): BaseObj_t(NULL), m_icfs_analysis_status(ICFS_Analysis_Incomplete) {}
		ICFS_t(const ICFS_Analysis_Status_t p_status) : BaseObj_t(NULL), m_icfs_analysis_status(p_status) {}
		ICFS_t(db_id_t p_set_id, const ICFS_Analysis_Status_t p_status = ICFS_Analysis_Incomplete);
		ICFS_t(db_id_t p_set_id, const std::string);
		std::string WriteToDB(File_t *fid);
		 

		// this is bad -- you loose data with this operator=.

		void SetTargets(const InstructionSet_t &other) 
		{
			InstructionSet_t::operator=(other);
		}
		void AddTargets(const InstructionSet_t &other) 
		{
			insert(std::begin(other), std::end(other)); 
		}

		bool IsIncomplete() const {
			return GetAnalysisStatus() == ICFS_Analysis_Incomplete;
		}

		bool IsComplete() const {
			return GetAnalysisStatus() == ICFS_Analysis_Complete;
		}

		bool IsModuleComplete() const {
			return GetAnalysisStatus() == ICFS_Analysis_Module_Complete;
		}

		void SetAnalysisStatus(const ICFS_Analysis_Status_t p_status) {
			m_icfs_analysis_status = p_status;
		}

		ICFS_Analysis_Status_t GetAnalysisStatus() const { 
			return m_icfs_analysis_status;
		}

	private:
		ICFS_Analysis_Status_t m_icfs_analysis_status;
};

typedef std::set<ICFS_t*> ICFSSet_t;
