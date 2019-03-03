

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



#include <irdb-core>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <exeio.h>

// disable sign compare warnings in 3rd party code 
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#pragma GCC diagnostic pop


#include "fill_in_indtargs.hpp"


using namespace std;
using namespace IRDB_SDK;






uint32_t ptrsize=0;
ELFIO::elfio *elfiop=NULL;
void* eh_frame_addr=0;
char* eh_frame_data=0;
int eh_frame_data_total_size;
intptr_t eh_offset=0;



typedef          int  sword;
typedef unsigned int  uword;
typedef unsigned int  uaddr;
typedef          int  saddr;
typedef unsigned char ubyte;
typedef struct dwarf_fde fde;
typedef uintptr_t _Unwind_Ptr;

union unaligned
{
  void *p;
  unsigned u2 __attribute__ ((mode (HI)));
  unsigned u4 __attribute__ ((mode (SI)));
  unsigned u8 __attribute__ ((mode (DI)));
  signed s2 __attribute__ ((mode (HI)));
  signed s4 __attribute__ ((mode (SI)));
  signed s8 __attribute__ ((mode (DI)));
} __attribute__ ((packed));


#define DW_EH_PE_absptr         0x00
#define DW_EH_PE_omit           0xff

#define DW_EH_PE_uleb128        0x01
#define DW_EH_PE_udata2         0x02
#define DW_EH_PE_udata4         0x03
#define DW_EH_PE_udata8         0x04
#define DW_EH_PE_sleb128        0x09
#define DW_EH_PE_sdata2         0x0A
#define DW_EH_PE_sdata4         0x0B
#define DW_EH_PE_sdata8         0x0C
#define DW_EH_PE_signed         0x08

#define DW_EH_PE_pcrel          0x10
#define DW_EH_PE_textrel        0x20
#define DW_EH_PE_datarel        0x30
#define DW_EH_PE_funcrel        0x40
#define DW_EH_PE_aligned        0x50

#define DW_EH_PE_indirect       0x80

#if __SIZEOF_LONG__ >= __SIZEOF_POINTER__
  typedef long _sleb128_t;
  typedef unsigned long _uleb128_t;
#elif __SIZEOF_LONG_LONG__ >= __SIZEOF_POINTER__
  typedef long long _sleb128_t;
  typedef unsigned long long _uleb128_t;
#else
# error "What type shall we use for _sleb128_t?"
#endif


struct dwarf_cie
{
  uword length;
  sword CIE_id;
  ubyte version;
  unsigned char augmentation[1];
} __attribute__ ((packed, aligned (__alignof__ (void* ))));



struct dwarf_fde
{
  uword length;
  sword CIE_delta;
  unsigned char pc_begin[1];
} __attribute__ ((packed, aligned (__alignof__ (void* ))));


struct fde_vector
{
  void *orig_data;
  size_t count;
  struct dwarf_fde *array[1];
};

struct object
{
  void *pc_begin;
  void *tbase;
  void *dbase;
  union {
    struct dwarf_fde *single;
    struct dwarf_fde **array;
    struct fde_vector *sort;
  } u;

  union {
    struct {
      unsigned long sorted : 1;
      unsigned long from_array : 1;
      unsigned long mixed_encoding : 1;
      unsigned long encoding : 8;
      /* ??? Wish there was an easy way to detect a 64-bit host here;
         we've got 32 bits left to play with...  */
      unsigned long count : 21;
    } b;
    size_t i;
  } s;
#ifdef DWARF2_OBJECT_END_PTR_EXTENSION
  char *fde_end;
#endif

  struct object *next;

};

void * ptr_to_data_to_addr(uintptr_t data_addr);
void * addr_to_ptr_to_data(uintptr_t to_deref);
intptr_t addr_to_p_offset(uintptr_t addr);


#include "unwind-pe.h"

void * ptr_to_data_to_addr(uintptr_t data_addr)
{
	int i;
	if(eh_frame_data<=(void*)data_addr && 
		(uintptr_t)data_addr <= (uintptr_t)eh_frame_data+(uintptr_t)eh_frame_data_total_size)
	{
		intptr_t offset=((uintptr_t)data_addr-(uintptr_t)eh_frame_data);
		const char* data=(const char*)(uintptr_t)eh_frame_addr;
		return (void*)&data[offset];
	}
	for(i=0; i<elfiop->sections.size();i++)
	{
		// skip sections that aren't loaded.
		if((elfiop->sections[i]->get_flags() & SHF_ALLOC) != SHF_ALLOC)
			continue;
		if(elfiop->sections[i]->get_data()<=(void*)data_addr && 
			(uintptr_t)data_addr < (uintptr_t)elfiop->sections[i]->get_data() + (uintptr_t)elfiop->sections[i]->get_size() )
		{
			intptr_t offset=((uintptr_t)data_addr-(uintptr_t)elfiop->sections[i]->get_data());
			const char* data=(const char*)(uintptr_t)elfiop->sections[i]->get_address();
			return (void*)&data[offset];
		}
	}
	assert(0);
}


intptr_t addr_to_p_offset(uintptr_t to_deref)
{
	int i;
	for(i=0; i<elfiop->sections.size();i++)
	{
		// skip sections that aren't loaded.
		if((elfiop->sections[i]->get_flags() & SHF_ALLOC) != SHF_ALLOC)
			continue;
		if(elfiop->sections[i]->get_address()<=to_deref && 
			to_deref < elfiop->sections[i]->get_address() + elfiop->sections[i]->get_size() )
		{
			intptr_t offset=((uintptr_t)to_deref-(uintptr_t)elfiop->sections[i]->get_address());
			const char* data=elfiop->sections[i]->get_data();
			return (intptr_t)(to_deref-(uintptr_t)&data[offset]); 
		}
	}
	assert(0);
}

void * addr_to_ptr_to_data(uintptr_t to_deref)
{
	int i;
	for(i=0; i<elfiop->sections.size();i++)
	{
		// skip sections that aren't loaded.
		if((elfiop->sections[i]->get_flags() & SHF_ALLOC) != SHF_ALLOC)
			continue;
		if(elfiop->sections[i]->get_address()<=to_deref && 
			to_deref < elfiop->sections[i]->get_address() + elfiop->sections[i]->get_size() )
		{
			intptr_t offset=((uintptr_t)to_deref-(uintptr_t)elfiop->sections[i]->get_address());
			const char* data=elfiop->sections[i]->get_data();
			return (void*)&data[offset];
		}
	}
	assert(0);
}

_Unwind_Internal_Ptr deref_unwind_ptr(uintptr_t to_deref)
{
	int i;
	for(i=0; i<elfiop->sections.size();i++)
	{
		// skip sections that aren't loaded.
		if((elfiop->sections[i]->get_flags() & SHF_ALLOC) != SHF_ALLOC)
			continue;
		if(elfiop->sections[i]->get_address()<=to_deref && 
			to_deref + ptrsize <= elfiop->sections[i]->get_address() + elfiop->sections[i]->get_size() )
		{
			intptr_t offset=(to_deref-elfiop->sections[i]->get_address());
			const char* data=elfiop->sections[i]->get_data();
			return deref_unwind_ptr_in_memory(&data[offset]);
		}
	}
	assert(0);
}

struct lsda_header_info
{
  _Unwind_Ptr Start;
  _Unwind_Ptr LPStart;
  _Unwind_Ptr ttype_base;
  unsigned char *TType;
  unsigned char *action_table;
  unsigned char ttype_encoding;
  unsigned char call_site_encoding;
};



struct object *all_objects=NULL;

void register_frame_info(void *begin, struct object *ob)
{
	memset(ob,0,sizeof(struct object));

  	ob->pc_begin = (void *)-1;
  	ob->tbase = 0;
  	ob->dbase = 0;
  	ob->u.single = (struct dwarf_fde *)begin;
  	ob->s.i = 0;
  	ob->s.b.encoding = DW_EH_PE_omit;
#ifdef DWARF2_OBJECT_END_PTR_EXTENSION
  	ob->fde_end = NULL;
#endif

  	ob->next = all_objects;
  	all_objects = ob;

}

struct dwarf_cie * get_cie (struct dwarf_fde *f)
{
  	return (struct dwarf_cie*)((long long)&f->CIE_delta - (long long)f->CIE_delta);
}

fde * next_fde (fde *f)
{
  	return (fde *) ((char *) f + f->length + sizeof (f->length));
}

int last_fde (struct object *obj __attribute__ ((__unused__)), fde *f)
{
#ifdef DWARF2_OBJECT_END_PTR_EXTENSION
  	return (char *)f == obj->fde_end || f->length == 0;
#else
  	return f->length == 0;
#endif
}

static _Unwind_Ptr
base_from_object (unsigned char encoding, struct object *ob)
{
  if (encoding == DW_EH_PE_omit)
    return 0;

  switch (encoding & 0x70)
    {
    case DW_EH_PE_absptr:
    case DW_EH_PE_pcrel:
    case DW_EH_PE_aligned:
      return 0;

    case DW_EH_PE_textrel:
      return (_Unwind_Ptr) ob->tbase;
    case DW_EH_PE_datarel:
      return (_Unwind_Ptr) ob->dbase;
    default:
      assert(0);
    }
}

int get_cie_encoding (struct dwarf_cie *cie)
{
  unsigned char *aug, *p;
  _Unwind_Ptr dummy;
  _uleb128_t utmp;
  _sleb128_t stmp;

  aug = cie->augmentation;
  if (aug[0] != 'z')
    return DW_EH_PE_absptr;

  p = aug + strlen ((char *)aug) + 1; /* Skip the augmentation string.  */
  p = read_uleb128 (p, &utmp);          /* Skip code alignment.  */
  p = read_sleb128 (p, &stmp);          /* Skip data alignment.  */
  if (cie->version == 1)                /* Skip return address column.  */
    p++;
  else
    p = read_uleb128 (p, &utmp);

  aug++;                                /* Skip 'z' */
  p = read_uleb128 (p, &utmp);          /* Skip augmentation length.  */
  while (1)
    {
      /* This is what we're looking for.  */
      if (*aug == 'R')
        return *p;
      /* Personality encoding and pointer.  */
      else if (*aug == 'P')
        {
          /* ??? Avoid dereferencing indirect pointers, since we're
             faking the base address.  Gotta keep DW_EH_PE_aligned
             intact, however.  */
          p = read_encoded_value_with_base (*p & 0x7F, 0, p + 1, &dummy);
        }
      /* LSDA encoding.  */
      else if (*aug == 'L')
        p++;
      /* Otherwise end of string, or unknown augmentation.  */
      else
        return DW_EH_PE_absptr;
      aug++;
    }
}




void * read_pointer (void *p) { union unaligned *up = (union unaligned*)p; return (void*)up->p; }


unsigned char *
extract_cie_info (struct dwarf_cie *cie,
		int *saw_z, 
		int *lsda_encoding,
		int *fde_encoding)
{
	unsigned char *aug = cie->augmentation;
	unsigned char *p = aug + strlen ((char *)aug) + 1;
	unsigned char *ret = NULL;
	_uleb128_t utmp=0;
	_sleb128_t stmp=0;

	/* g++ v2 "eh" has pointer immediately following augmentation string,
	so it must be handled first.  */
	if (aug[0] == 'e' && aug[1] == 'h')
	{
		void* eh_ptr = read_pointer (p);
		if(getenv("EH_VERBOSE"))
			cout<<"eh_ptr: "<< std::dec << eh_ptr<<endl;
		p += ptrsize;
		aug += 2;
	}

	/* Immediately following the augmentation are the code and
	 * data alignment and return address column.  */
  	p = read_uleb128 (p, &utmp);
	if(getenv("EH_VERBOSE"))
		cout<<"code align: "<< std::dec << utmp<<endl;
  	p = read_sleb128 (p, &stmp);
	if(getenv("EH_VERBOSE"))
		cout<<"data align: "<< std::dec << stmp<<endl;
  	if (cie->version == 1)
	{
    		int ret_col= *p++;
		if(getenv("EH_VERBOSE"))
			cout<<"ret_col: "<< std::dec << ret_col<<endl;
	}
  	else
    	{
      		p = read_uleb128 (p, &utmp);
		if(getenv("EH_VERBOSE"))
			cout<<"ret_col: "<< std::dec << utmp<<endl;
    	}

  /* If the augmentation starts with 'z', then a uleb128 immediately
     follows containing the length of the augmentation field following
     the size.  */
  	if (*aug == 'z')
    	{
      		p = read_uleb128 (p, &utmp);
      		ret = p + utmp;
		*saw_z=1;
		if(getenv("EH_VERBOSE"))
			cout<<"Saw z, length: "<<utmp<<endl;
      		++aug;
    	}

  /* Iterate over recognized augmentation subsequences.  */
  	while (*aug != '\0')
	{
		/* "L" indicates a byte showing how the LSDA pointer is encoded.  */
		if (aug[0] == 'L')
		{
			*lsda_encoding = *p++;
			if(getenv("EH_VERBOSE"))
				cout<<"lsda encoding "<<std::hex<<*lsda_encoding<<endl;
			aug += 1;
		}

		/* "R" indicates a byte indicating how FDE addresses are encoded.  */
		else if (aug[0] == 'R')
		{
			*fde_encoding = *p++;
			if(getenv("EH_VERBOSE"))
				cout<<"fde encoding "<<std::hex<<*fde_encoding<<endl;
			aug += 1;
		}

		/* "P" indicates a personality routine in the CIE augmentation.  */
		else if (aug[0] == 'P')
		{
			_Unwind_Ptr personality=0;
			int personality_encoding=*p;


			p = read_encoded_value_with_base(*p, 0, p + 1, &personality);

			if(getenv("EH_VERBOSE"))
			{
				cout << "encoding personality = " << std::hex << personality_encoding << endl;
				cout << "fs->personality = " << std::hex << personality << endl;
			}
			possible_target(personality, 0, ibt_provenance_t::ibtp_eh_frame);
			aug += 1;
		}

		/* "S" indicates a signal frame.  */
		else if (aug[0] == 'S')
		{
			if(getenv("EH_VERBOSE"))
				cout << "fs->signal_frame = 1 " <<  endl;
			aug += 1;
		}

		/* Otherwise we have an unknown augmentation string.
		Bail unless we saw a 'z' prefix.  */
		else
			return ret;
	}

  	return ret ? ret : p;
}



static size_t
classify_object_over_fdes (struct object *ob, fde *this_fde)
{
  struct dwarf_cie *last_cie = 0;
  size_t count = 0;
  int encoding = DW_EH_PE_absptr;
  _Unwind_Ptr base = 0;

  for (; ! last_fde (ob, this_fde); this_fde = next_fde (this_fde))
    {
//	printf("analysis addr=%p\n", this_fde);
//	printf("pgm addr=%p\n", (uintptr_t)this_fde+(uintptr_t)eh_offset);
//	printf("offset=%p\n", (uintptr_t)this_fde+(uintptr_t)eh_offset-(uintptr_t)eh_frame_addr);
      struct dwarf_cie *this_cie;
      _Unwind_Ptr mask, pc_begin;

      /* Skip CIEs.  */
      if (this_fde->CIE_delta == 0)
        continue;

      /* Determine the encoding for this FDE.  Note mixed encoded
         objects for later.  */
      this_cie = get_cie (this_fde);
      if (this_cie != last_cie)
        {
          last_cie = this_cie;
          encoding = get_cie_encoding (this_cie);
          base = base_from_object (encoding, ob);
          if (ob->s.b.encoding == DW_EH_PE_omit)
            ob->s.b.encoding = encoding;
          else if (ob->s.b.encoding != encoding)
            ob->s.b.mixed_encoding = 1;
        }

      read_encoded_value_with_base (encoding, base, this_fde->pc_begin,
                                    &pc_begin);

      /* Take care to ignore link-once functions that were removed.
         In these cases, the function address will be NULL, but if
         the encoding is smaller than a pointer a true NULL may not
         be representable.  Assume 0 in the representable bits is NULL.  */
      mask = size_of_encoded_value (encoding);
      if (mask < ptrsize)
        mask = (((_Unwind_Ptr) 1) << (mask << 3)) - 1;
      else
        mask = -1;

      if ((pc_begin & mask) == 0)
        continue;

      count += 1;
      if ((void *) pc_begin < ob->pc_begin)
        ob->pc_begin = (void *) pc_begin;
    }


	if(getenv("EH_VERBOSE"))
		cout<<"Count is "<<count<<endl;

  	return count;
}

void print_lsda_handlers(lsda_header_info* info, unsigned char* p)
{
	// change to lsda section;
	uintptr_t p_addr=((uintptr_t)p+(uintptr_t)eh_offset);

	// if the action table is size 0, avoid calling addr_to_ptr_to_data on it, because it may assert, 
	// and we wouldn't do anything (except 0 iterations of the loop below) with the results anyhow.
	if(p_addr == (uintptr_t)info->action_table)
		return;

	p=(unsigned char*)addr_to_ptr_to_data((uintptr_t)p_addr);
	// use -1 in case info->action table represents the end of a section.
	uintptr_t action_table_in_data=(uintptr_t)addr_to_ptr_to_data((uintptr_t)info->action_table-1);
//	intptr_t p_offset=(intptr_t)p-(intptr_t)ptr_to_addr_to_data_to_addr((uintptr_t)p);
  	// Search the call-site table for the action associated with this IP.
  	while (
			// use <= because we we used -1 3 lines above.
			((uintptr_t)p) <=  (uintptr_t)action_table_in_data
		)
    	{
      		_Unwind_Ptr cs_start=0, cs_len=0, cs_lp=0;
      		_uleb128_t cs_action=0;
		
      		// Note that all call-site encodings are "absolute" displacements.
      		p = read_encoded_value (0, info->call_site_encoding, p, &cs_start);
      		p = read_encoded_value (0, info->call_site_encoding, p, &cs_len);
      		p = read_encoded_value (0, info->call_site_encoding, p, &cs_lp);
      		p = read_uleb128 (p, &cs_action);

		if(cs_lp!=0)
		{

			if(getenv("EH_VERBOSE"))
			{
				cout 	<<"cs_start: "<< cs_start  << "\t"
					<<"cs_len: "<< cs_len << "\t"
					<<"cs_lp: "<< cs_lp << "\t"
					<<"cs_action: "<< cs_action << endl;
				cout 	<<"cs_start+info->Start: "<< cs_start+info->Start << "\t"
					<<"cs_len+cs_start+info->Start: "<< cs_len+cs_start+info->Start << "\t"
					<<"cs_lp+info->LPstart: "<< cs_lp +info->LPStart<< "\t"
					<<"cs_action: "<< cs_action << endl;
			}

#ifndef TEST

			/* the landing pad is a possible target if an exception is thrown */ 
			possible_target(cs_lp+info->Start, 0, ibt_provenance_t::ibtp_eh_frame);

			/* and the return address is a possible oddity if it's used for walking the stack */
			possible_target(cs_len+cs_start+info->Start, 0, ibt_provenance_t::ibtp_eh_frame);

#endif
		}
    	}
}

int get_fde_encoding (struct dwarf_fde *f)
{
  return get_cie_encoding (get_cie (f));
}

unsigned char * parse_lsda_header (unsigned char *p, lsda_header_info *info, struct object *ob, fde* f)
{
  	_uleb128_t tmp=0;
  	unsigned char lpstart_encoding=0;
	_Unwind_Ptr func=0;

  	info->Start = 0;
/*
fixme

	needs to be info->Start=context->bases->func


which is set here:
*/
      unsigned char encoding = ob->s.b.encoding;
      if (ob->s.b.mixed_encoding)
        encoding = get_fde_encoding (f);
      read_encoded_value_with_base (encoding, base_from_object (encoding, ob),
                                    f->pc_begin, (_Unwind_Ptr*)&func);

	info->Start=func;
	if(getenv("EH_VERBOSE"))
		cout<<"info->Start set to "<<std::hex << (uintptr_t)info->Start << endl;


  	// Find @LPStart, the base to which landing pad offsets are relative.
  	lpstart_encoding = *p++;
  	if (lpstart_encoding != DW_EH_PE_omit)
    		p = read_encoded_value_with_base (lpstart_encoding, 0, p, &info->LPStart);
  	else
    		info->LPStart = info->Start;

	if(getenv("EH_VERBOSE"))
		cout<<"LPStart : "<<std::hex<<info->LPStart<<endl;

  	// Find @TType, the base of the handler and exception spec type data.
  	info->ttype_encoding = *p++;
  	if (info->ttype_encoding != DW_EH_PE_omit)
    	{
      		p = read_uleb128 (p, &tmp);
		// this doesn't work
      		//info->TType = (unsigned char*)ptr_to_data_to_addr((uintptr_t)(p + tmp));
      		info->TType = p + tmp + eh_offset;
    	}
  	else
    		info->TType = 0;

	if(getenv("EH_VERBOSE"))
		cout<<"ttype : "<<std::hex<<((uintptr_t)(info->TType))<<endl;

  	// The encoding and length of the call-site table; the action table
  	// immediately follows.
  	info->call_site_encoding = *p++;

	if(getenv("EH_VERBOSE"))
		cout<<"Call site encoding   " << std::hex << (uintptr_t)info->call_site_encoding << endl;

  	p = read_uleb128 (p, &tmp);
  	info->action_table = p + tmp + eh_offset;
  	//info->action_table = (unsigned char*)ptr_to_data_to_addr((uintptr_t)(p + tmp));
	if(getenv("EH_VERBOSE"))
		cout<<"Action table: "<<std::hex<<(uintptr_t)info->action_table<<endl;

  	return p;
}





void linear_search_fdes (struct object *ob, fde *this_fde, int offset)
{
  	struct dwarf_cie *last_cie = 0;
  	int encoding = ob->s.b.encoding;
	cout<<"encoding="<<encoding<<endl;
  	_Unwind_Ptr base = base_from_object (ob->s.b.encoding, ob);
	int saw_z=0; 
	int lsda_encoding=DW_EH_PE_omit;
	int fde_encoding=0;
	int count=0;

  	for (; ! last_fde (ob, this_fde); this_fde = next_fde (this_fde))
    	{
		count++;
		if(getenv("EH_VERBOSE"))
		{
			cout<<"Examining FDE #"<<std::dec<<count<<" at offset "<<hex<<(uintptr_t)this_fde-(uintptr_t)eh_frame_data<<endl;
			cout<<"FDE (in memory) addr is: "<<std::hex<<this_fde<<endl;
			cout<<"FDE (in file) addr is: "<<std::hex<<(uintptr_t)this_fde-(uintptr_t)eh_frame_data+(uintptr_t)eh_frame_addr<<endl;
		}
      		struct dwarf_cie *this_cie=NULL;
      		_Unwind_Ptr pc_begin=0, pc_range=0;
	
      		/* Skip CIEs.  */
      		if (this_fde->CIE_delta == 0)
		{
			if(getenv("EH_VERBOSE"))
				cout<<"Skipping FDE because it's a CIE"<<endl;
        		continue;
		}

		this_cie = get_cie (this_fde);

		saw_z=0; 
		lsda_encoding=DW_EH_PE_omit;
		fde_encoding=0;
		extract_cie_info(this_cie, &saw_z, &lsda_encoding, &fde_encoding);

  		/* Locate augmentation for the fde.  */
  		unsigned char* aug = (unsigned char *) this_fde + 8; /* 4-bytes for FDE len, 4-bytes for FDE id. */

		// 2x size of fde_encoding for pc_begin and pc_range */
  		aug += 2 * size_of_encoded_value (fde_encoding);
  		if (saw_z)
    		{
      			_uleb128_t i=0;
      			aug = read_uleb128 (aug, &i);
    		}
  		if (lsda_encoding != DW_EH_PE_omit)
    		{
      			_Unwind_Ptr lsda;
		
        		aug = read_encoded_value_with_base (lsda_encoding, 0, aug, &lsda);
			cout<<"lsda at "<<std::hex << lsda <<endl;
			lsda_header_info info;
			memset(&info, 0, sizeof(info));
			cout.flush();
	
			// lsda might be 0, which means we shouldn't load it!
			if(lsda != 0 ) 
			{
				// change sections;
				uintptr_t save_eh_offset=eh_offset;
				eh_offset=addr_to_p_offset(lsda);
				unsigned char* lsda_p=parse_lsda_header (
					(unsigned char*)addr_to_ptr_to_data(lsda)
					, &info, ob, this_fde);
				print_lsda_handlers(&info, lsda_p); 
				eh_offset=save_eh_offset;
				cout.flush();
			}
    		}

	

      		if (ob->s.b.mixed_encoding)
        	{
          		/* Determine the encoding for this FDE.  Note mixed encoded
             		objects for later.  */
           		this_cie = get_cie (this_fde);
          		if (this_cie != last_cie)
            		{
              			last_cie = this_cie;
              			encoding = get_cie_encoding (this_cie);
              			base = base_from_object (encoding, ob);
				if(getenv("EH_VERBOSE"))
					cout<<"Base from object (mixed encoding?)="<<std::hex<<base<<endl;
            		}
        	}
		
      		if (encoding == DW_EH_PE_absptr)
        	{
          		//pc_begin = ((_Unwind_Ptr *) this_fde->pc_begin)[0];
			auto my_pc_begin=(_Unwind_Ptr*)(this_fde->pc_begin);
			memcpy(&pc_begin, my_pc_begin, sizeof(_Unwind_Ptr));
          		//pc_range = ((_Unwind_Ptr *) this_fde->pc_begin)[1];
			memcpy(&pc_range, my_pc_begin+1, sizeof(_Unwind_Ptr));

			if(getenv("EH_VERBOSE"))
			{
				cout<<"absptr pc_begin 0x"<<std::hex<<(pc_begin+offset)<<"\t";
				cout<<"absptr pc_end 0x"<<std::hex<<(pc_begin+pc_range+offset)<<endl;
			}
#ifndef TEST
			extern void range(VirtualOffset_t, VirtualOffset_t);
			range(pc_begin,pc_begin+pc_range);
#endif
          		if (pc_begin == 0)
            			continue;
        	}
      		else
        	{
          		_Unwind_Ptr mask=0;
          		unsigned char *p=0;
		
          		p = read_encoded_value_with_base (encoding, base,
                                            		this_fde->pc_begin, &pc_begin);
          		read_encoded_value_with_base (encoding & 0x0F, 0, p, &pc_range);

			if(getenv("EH_VERBOSE"))
			{
				cout<<"!absptr pc_begin 0x"<<std::hex<<(pc_begin)<<"\t";
				cout<<"!absptr pc_end 0x"<<std::hex<<(pc_begin+pc_range)<<endl;
			}

#ifndef TEST
			extern void range(VirtualOffset_t, VirtualOffset_t);
			range(pc_begin,pc_begin+pc_range);
#endif

          		/* Take care to ignore link-once functions that were removed.
             		In these cases, the function address will be NULL, but if
             		the encoding is smaller than a pointer a true NULL may not
             		be representable.  Assume 0 in the representable bits is NULL.  */
          		mask = size_of_encoded_value (encoding);
          		if (mask < ptrsize)
            			mask = (((_Unwind_Ptr) 1) << (mask << 3)) - 1;
          		else
            			mask = -1;
		
          		if ((pc_begin & mask) == 0)
            			continue;
        	}
    	}

	if(getenv("EH_VERBOSE"))
		cout<<"Found "<<std::dec<<count<< " FDE's"<<endl;

  	return;
}

void read_ehframe(FileIR_t* virp, EXEIO::exeio* exeiop)
{

	ptrsize=virp->getArchitectureBitWidth()/(8*sizeof(char));
	// 64/8 = 8
	// 32/8 = 4

	assert(exeiop);
	elfiop=reinterpret_cast<ELFIO::elfio*>(exeiop->get_elfio());
	if(!elfiop)
		return;	// skip entire analysis for non-elf files as eh-frame is way different.

	const auto eh_frame_it=find_if(virp->getDataScoops().begin(), virp->getDataScoops().end(),
		[](const DataScoop_t* scoop) { return scoop->getName()==".eh_frame"; });

	// either no eh_frame in the elf file, or fill_in_indtargs removed it because
	// it was asked to import the EH IR. 
	if(eh_frame_it==virp->getDataScoops().end())
		return;

	int secndx=0;
	int secnum=elfiop->sections.size(); 

       	/* Locate desired section */
       	bool found=false;
	int eh_frame_index;
       	for (secndx=1; secndx<secnum; secndx++)
       	{
               	if (elfiop->sections[secndx]->get_name() == ".eh_frame")
               	{
                       	found = true;
			eh_frame_index=secndx;
                       	break;
               	}
       	}

	if(!found)
	{
		cout<<"Cannot find .eh_frame section\n";
		return;
	}
	cout<<"Found .eh_frame is section "<<std::dec<<eh_frame_index<<endl;

	eh_frame_addr=(void*)elfiop->sections[eh_frame_index]->get_address();
	cout<<"Found .eh_frame section addr is "<<std::dec<<eh_frame_addr<<endl;
	int total_size=0;

        if (elfiop->sections[secndx+1]->get_name() != ".gcc_except_table")
	{
		cout<<"Did not find .gcc_except_table immediately after .eh_frame\n";
		total_size=elfiop->sections[eh_frame_index]->get_size()+1;
	}
	else
	{
		total_size=
			(elfiop->sections[eh_frame_index+1]->get_address()+
			 elfiop->sections[eh_frame_index+1]->get_size()   ) - (uintptr_t)eh_frame_addr;
	}
	eh_frame_data_total_size=total_size;
	
	
	// calc the size needed to safely walk the EH frame data.  apparently walking assumes a null value in memory
	// after the section is loaded (or properly using eh_frame_hdr, which we aren't doing)
	//eh_frame_data=(char*)elfiop->sections[eh_frame_index]->get_data();
	int newsize=elfiop->sections[eh_frame_index]->get_size()+4;
	eh_frame_data=(char*)calloc(1,newsize);
	memcpy(eh_frame_data, (void*)elfiop->sections[eh_frame_index]->get_data(), elfiop->sections[eh_frame_index]->get_size());

	uintptr_t offset;
//	careful with offset and eh_offset as it can only be used for eh_frame_data offsetting, not offsetting into other addrs.
	eh_offset=offset=(uintptr_t)eh_frame_addr-(uintptr_t)eh_frame_data;


	struct object ob;
	register_frame_info( eh_frame_data, &ob);
	classify_object_over_fdes(&ob,ob.u.single);
	linear_search_fdes (&ob,ob.u.single,offset); 

	/* clean up memory */

}

#ifdef TEST
//
// main rountine; convert calls into push/jump statements 
//
main(int argc, char* argv[])
{

	bool fix_all=false;

	if(argc!=2 && argc !=3)
	{
		cerr<<"Usage: fix_calls <id> (--fix-all) "<<endl;
		exit(-1);
	}

	if(argc==3)
	{
		if(strcmp("--fix-all", argv[2])!=0)
		{
			cerr<<"Unrecognized option: "<<argv[2]<<endl;
			exit(-1);
		}
		else
			fix_all=true;
	}

	VariantID_t *pidp=NULL;
	FileIR_t *virp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		// read the db  
		virp=new FileIR_t(*pidp);


	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(virp && pidp);
	
	cout<<"Reading EH frame and gcc except table in variant "<<*pidp<< "." <<endl;

	read_ehframe(virp,pqxx_interface);

	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
//	virp->WriteToDB();

//	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete virp;
	delete pidp;
}
#endif
