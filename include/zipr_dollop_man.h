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

#ifndef zipr_dollop_man_h
#define zipr_dollop_man_h 

#include <dollop.h>

class ZiprDollopManager_t {
	public:
		ZiprDollopManager_t() {}
		void AddDollop(Dollop_t *dollop);
		Zipr_SDK::Dollop_t *GetContainingDollop(libIRDB::Instruction_t *insn) {
			try {
				return m_insn_to_dollop.at(insn);
			} catch (const std::out_of_range &oor) {
				return NULL;
			}
		}

		static Zipr_SDK::Dollop_t *CreateDollop(libIRDB::Instruction_t *start) {
			return new Zipr_SDK::Dollop_t(start);
		}

	private:
		std::map<libIRDB::Instruction_t*,Dollop_t*> m_insn_to_dollop;
		std::list<Dollop_t*> m_dollops;
};

#endif
