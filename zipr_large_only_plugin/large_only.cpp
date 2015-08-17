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


#include <zipr_sdk.h>
#include <string>
#include <algorithm>
#include "utils.hpp"
#include "Rewrite_Utility.hpp"
#include "large_only.h"

using namespace libIRDB;
using namespace std;
using namespace Zipr_SDK;
using namespace ELFIO;

LargeOnly_t::LargeOnly_t(MemorySpace_t *p_ms,
	elfio *p_elfio,
	FileIR_t *p_firp,
	Options_t *p_opts,
	InstructionLocationMap_t *p_fil) :
		m_memory_space(*p_ms), 
		m_elfio(*p_elfio),
		m_firp(*p_firp),
		m_opts(*p_opts),
		final_insn_locations(*p_fil)
{
}

void LargeOnly_t::RemoveSmallMemorySpaces(void)
{
	Range_t removableRange;
	Range_t largeRange;
	if (m_opts.GetVerbose())
		cout << "Starting to remove all small memory spaces." << endl;
	while (m_memory_space.GetRangeCount() != 0)
	{
		removableRange = m_memory_space.GetFreeRange(0);
		if (removableRange.GetEnd() == -1)
			largeRange = removableRange;
		m_memory_space.RemoveFreeRange(removableRange);
		if (m_opts.GetVerbose())
			cout << "Memory space size: "
			     << m_memory_space.GetRangeCount()
			     << endl;
	}
	m_memory_space.AddFreeRange(largeRange);
	if (m_opts.GetVerbose())
	{
		cout << "After removing all small memory spaces:" << endl;
		m_memory_space.PrintMemorySpace(cout);
	}
}

extern "C" 
Zipr_SDK::ZiprPluginInterface_t* GetPluginInterface(
	Zipr_SDK::MemorySpace_t *p_ms, 
	ELFIO::elfio *p_elfio, 
	libIRDB::FileIR_t *p_firp, 
	Zipr_SDK::Options_t *p_opts,
	Zipr_SDK::InstructionLocationMap_t *p_fil) 
{
	return new LargeOnly_t(p_ms,p_elfio,p_firp,p_opts,p_fil);
}
