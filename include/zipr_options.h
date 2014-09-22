/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of GrammaTech, Inc. Title to,
 * ownership of, and all rights in the software is retained by
 * GrammaTech, Inc.
 *
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

#include <string>
// #include <libIRDB-core.hpp>

class Options_t 
{
	public:
		Options_t() : m_outname("b.out") { };

		static Options_t* parse_args(int p_argc, char* p_argv[]) 
		{ 
			Options_t *opt=new Options_t;
			assert(opt);
			opt->m_var_id=::atoi(p_argv[1]); 
			return opt;
		};
		
		std::string GetOutputFileName(libIRDB::File_t* p_file) { return m_outname; }
		int GetVariantID() { return m_var_id; }

	private:
		std::string m_outname;
		int m_var_id;
};

#endif
