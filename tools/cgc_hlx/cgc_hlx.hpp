/*
 * Copyright (c) 2015 - University of Virginia
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from the 
 * University of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef hlx_hpp
#define hlx_hpp

#include <libIRDB-core.hpp>

#define DEFAULT_MALLOC_PADDING 64
#define DEFAULT_SHR_FACTOR 5
#define DEFAULT_ALLOCATE_PADDING 4096

class HLX_Instrument
{
	public:
		HLX_Instrument(libIRDB::FileIR_t *the_firp) : 
			m_firp(the_firp) {
			m_enable_malloc_padding = false;
			m_enable_allocate_padding = false;
			m_malloc_padding = 0;
			m_allocate_padding = 0;
			m_shr_factor = DEFAULT_SHR_FACTOR; 
		}
		virtual ~HLX_Instrument() {}
		void enableMallocPadding(const int malloc_padding, const int shr_factor=DEFAULT_SHR_FACTOR) {
			m_enable_malloc_padding = true;
			m_malloc_padding = malloc_padding;
			m_shr_factor = shr_factor;
		}
		void enableAllocatePadding(const int allocate_padding) {
			m_enable_allocate_padding = true;
			m_allocate_padding = allocate_padding;
		}

		bool mallocPaddingEnabled() const { return m_enable_malloc_padding; }
		bool allocatePaddingEnabled() const { return m_enable_allocate_padding; }
		int getMallocPadding() const { return m_malloc_padding; }
		int getAllocatePadding() const { return m_allocate_padding; }

		int getShiftRightFactor() const { return m_shr_factor; }
		bool execute();

	private:
		libIRDB::Function_t* findFunction(std::string);
		bool padSizeOnAllocation(libIRDB::Function_t* const, const int padding, const int shr_factor = 0);
		bool padSizeOnDeallocation(libIRDB::Function_t* const, const int padding);

	private:
		libIRDB::FileIR_t* m_firp;
		bool m_enable_malloc_padding; 
		int m_malloc_padding; 
		bool m_enable_allocate_padding; 
		int m_allocate_padding; 
		int m_shr_factor; // shift right factor (for malloc)
};

#endif
