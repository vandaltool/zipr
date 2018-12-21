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


#ifndef PATCHER_BASE
#define PATCHER_BASE

class ZiprPatcherBase_t
{
	public:
		static unique_ptr<ZiprPatcherBase_t> factory(Zipr_SDK::Zipr_t* p_parent);
		virtual void ApplyNopToPatch(RangeAddress_t addr)=0;
		virtual void ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)=0;
		virtual void PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)=0;
};
#endif
