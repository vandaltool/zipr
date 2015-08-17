/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information. 
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *      
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#ifndef zipr_options_h
#define zipr_options_h

#include <zipr_all.h>
#include <string>
#include <unistd.h>
#include <libIRDB-core.hpp>

class ZiprOptions_t : public Options_t
{
	public:
		ZiprOptions_t() : 
			m_outname("b.out"), 
			m_objcopy_path("/usr/bin/objcopy")
		{
			m_verbose=false; 
			m_var_id=-1;
			m_architecture=-1;
			m_no_replop=true;
		}

		static ZiprOptions_t* parse_args(int p_argc, char* p_argv[]);
		static void print_usage(int p_argc, char *p_argv[]);

		std::string GetOutputFileName(libIRDB::File_t* p_file) { return m_outname; }
		std::string GetCallbackFileName() { return m_callbackname; }
		int GetVariantID() { return m_var_id; }
		int GetVerbose() { return m_verbose; }
		int GetNoReplop() { return m_no_replop; }
		int GetArchitecture();
		std::string GetObjcopyPath() { return m_objcopy_path; };
		
		void EnableOptimization(Optimizations_t::OptimizationName_t opt) 
		{ 
			EnabledOptimizations[opt] = 1; 
		};

		bool IsEnabledOptimization(Optimizations_t::OptimizationName_t opt) 
		{ 
			return EnabledOptimizations[opt] == 1; 
		};

		void SetVerbose(bool verbose)
		{
			m_verbose = verbose;
		}

		void SetNoReplop(bool no_replop)
		{
			m_no_replop = no_replop;
		}

	private:
		std::string m_outname;
		std::string m_callbackname;
		std::string m_objcopy_path;
		bool m_verbose;
		bool m_no_replop;
		int m_var_id;
		int m_architecture;
		int EnabledOptimizations[Optimizations_t::NumberOfOptimizations];
};

#endif
