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

#ifndef nonce_relocs_h
#define nonce_relocs_h


class NonceRelocs_t
{
	public:
	
		// constructors
		NonceRelocs_t(MemorySpace_t &p_ms, ELFIO::elfio& p_elfio, libIRDB::FileIR_t& p_firp, Options_t& p_opts) :
			m_memory_space(p_ms), 
			m_elfio(p_elfio),
			m_firp(p_firp),
			m_opts(p_opts)
		{
		}

		// main workhorse
		void HandleNonceRelocs();

	private:

		// helpers
		bool IsNonceRelocation(libIRDB::Relocation_t& reloc);
		int GetNonceValue(libIRDB::Relocation_t& reloc);
		int GetNonceSize(libIRDB::Relocation_t& reloc);
		void HandleNonceRelocation(libIRDB::Instruction_t& insn, libIRDB::Relocation_t& reloc);
		libIRDB::Relocation_t* FindNonceRelocation(libIRDB::Instruction_t* insn);
		void AddSlowPathInstructions();
		libIRDB::Relocation_t* FindSlowpathRelocation(libIRDB::Instruction_t* insn);



		// references to input
		MemorySpace_t &m_memory_space;	
		ELFIO::elfio&  m_elfio;
		libIRDB::FileIR_t& m_firp;
		Options_t& m_opts;

		// local data.

		// the set of instructions where we were asked to insert a nonce, but 
		// couldn't.  This will be necessary when we need to emit code for the slow path 
		libIRDB::InstructionSet_t slow_path_nonces;

		
};

#endif
