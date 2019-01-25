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

namespace libIRDB
{
using InstructionSet_t       = IRDB_SDK::InstructionSet_t;
using ICFS_Analysis_Status_t = IRDB_SDK::ICFSAnalysisStatus_t;
using ICFSSet_t              = IRDB_SDK::ICFSSet_t;

// these must match the allowed values for type icfs_analysis_result in Postgres DB
#define ICFS_ANALYSIS_INCOMPLETE_STR "icfs_analysis_incomplete"
#define ICFS_ANALYSIS_MODULE_COMPLETE_STR "icfs_analysis_module_complete"
#define ICFS_ANALYSIS_COMPLETE_STR "icfs_analysis_complete"

// Keep track of instruction control flow sets
class ICFS_t : virtual public InstructionSet_t, public BaseObj_t, virtual public IRDB_SDK::ICFS_t
{
	public:
		virtual ~ICFS_t(){}
		ICFS_t(): BaseObj_t(NULL), m_icfs_analysis_status(IRDB_SDK::iasAnalysisIncomplete) {}
		ICFS_t(const ICFS_Analysis_Status_t p_status) : BaseObj_t(NULL), m_icfs_analysis_status(p_status) {}
		ICFS_t(db_id_t p_set_id, const ICFS_Analysis_Status_t p_status = IRDB_SDK::iasAnalysisIncomplete);
		ICFS_t(db_id_t p_set_id, const std::string);
		std::string WriteToDB(File_t *fid);
		 

		// this is bad -- you loose data with this operator=.

		void setTargets(const InstructionSet_t &other) 
		{
			InstructionSet_t::operator=(other);
		}
		void addTargets(const InstructionSet_t &other) 
		{
			insert(std::begin(other), std::end(other)); 
		}

		bool isIncomplete() const {
			return getAnalysisStatus() == IRDB_SDK::iasAnalysisIncomplete;
		}

		bool isComplete() const {
			return getAnalysisStatus() == IRDB_SDK::iasAnalysisComplete;
		}

		bool isModuleComplete() const {
			return getAnalysisStatus() == IRDB_SDK::iasAnalysisModuleComplete;
		}

		void setAnalysisStatus(const ICFS_Analysis_Status_t p_status) {
			m_icfs_analysis_status = p_status;
		}

		ICFS_Analysis_Status_t getAnalysisStatus() const { 
			return m_icfs_analysis_status;
		}

	private:
		ICFS_Analysis_Status_t m_icfs_analysis_status;
};

}
