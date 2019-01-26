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

#ifndef PATCHER_X86
#define PATCHER_X86

class ZiprPatcherX86_t : public ZiprPatcherBase_t
{
	// data
	zipr::ZiprImpl_t* m_parent;
	IRDB_SDK::FileIR_t* m_firp;
	Zipr_SDK::MemorySpace_t &memory_space;

	// methods
	void RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos);

	public:
		ZiprPatcherX86_t(Zipr_SDK::Zipr_t* p_parent) ;
		void ApplyNopToPatch(RangeAddress_t addr) override;
		void ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr) override;
		void PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr) override;
                void PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr) override;
		void CallToNop(RangeAddress_t at_addr) override;

	


};
#endif
