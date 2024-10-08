/*
 * Copyright (c) 2014 - Zephyr Software LLC
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

#include <all.hpp>
#include <irdb-util>

using namespace libIRDB;
using namespace std;

static string getICFSAnalysisStatus(const ICFS_Analysis_Status_t p_status) {
	// strings must match DB definition
	switch (p_status) {
		case IRDB_SDK::iasAnalysisIncomplete:
			return string("icfs_analysis_incomplete");
			break;
		case IRDB_SDK::iasAnalysisModuleComplete:
			return string("icfs_analysis_module_complete");
			break;
		case IRDB_SDK::iasAnalysisComplete:
			return string("icfs_analysis_complete");
			break;
		default:
			return string("icfs_analysis_incomplete");
			break;
	}

	std::cerr << "error: unknown ICFS analysis status: " << p_status << std::endl;
	assert(0);
}

ICFS_t::ICFS_t(db_id_t p_set_id, const ICFS_Analysis_Status_t p_status) : BaseObj_t(NULL)
{
	setBaseID(p_set_id);
	setAnalysisStatus(p_status);	
}

ICFS_t::ICFS_t(db_id_t p_set_id, const string p_statusString) : BaseObj_t(NULL)
{
	setBaseID(p_set_id);
	if (p_statusString == "icfs_analysis_incomplete") {
		setAnalysisStatus(IRDB_SDK::iasAnalysisIncomplete);	
	} else if (p_statusString == "icfs_analysis_module_complete") {
		setAnalysisStatus(IRDB_SDK::iasAnalysisModuleComplete);	
	} else if (p_statusString == "icfs_analysis_complete") {
		setAnalysisStatus(IRDB_SDK::iasAnalysisComplete);	
	} else {
		std::cerr << "error: unknown ICFS analysis status string: " << p_statusString << std::endl;
		assert(0);
	}
}

string ICFS_t::WriteToDB(File_t *fid)
{
	assert(fid);

	db_id_t icfs_id = getBaseID();

	string analysis_status = getICFSAnalysisStatus(getAnalysisStatus());

	string q=string("insert into ") + fid->icfs_table_name + 
		string(" (icfs_id, icfs_status) VALUES (") + 
		string("'") + to_string(icfs_id) + string("', ") + 
		string("'") + analysis_status + string("'); ") ;

	for (InstructionSet_t::const_iterator it = this->begin(); 
		it != this->end(); ++it)
	{
		auto insn = *it;		
		assert(insn);

		auto address_id = insn->getAddress()->getBaseID();

		q += string("insert into ") + fid->icfs_map_table_name +
			string(" (icfs_id, address_id) VALUES(") +
			string("'") + to_string(icfs_id) + string("', ") +
			string("'") + to_string(address_id) + string("'); ");
	}

	return q;
}

