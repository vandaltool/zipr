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

#ifndef range_h
#define range_h


typedef uintptr_t RangeAddress_t;

class Range_t
{
	public:
		Range_t(RangeAddress_t p_s, RangeAddress_t p_e) : m_start(p_s), m_end(p_e) { }
		Range_t() : m_start(0), m_end(0) { }

		RangeAddress_t GetStart() { return m_start; }
		RangeAddress_t GetEnd() { return m_end; }

		bool Is2ByteRange()
		{
			return (m_end - m_start) == 2;
		};
		bool Is5ByteRange()
		{
			return (m_end - m_start) == 5;
		};

	protected:

		RangeAddress_t m_start, m_end;
};

#endif
