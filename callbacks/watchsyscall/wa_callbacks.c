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

#include "malloc.h"
#include "itox.h"
#include "strlen.h"
#include "null.h"


#ifdef DEBUG
	#define print_str_debug print_str
	#define print_int_debug print_int
	#define assert(a) {if(!a) print_str("assert failure!"), cgc_terminate(254);}
#else
	#define print_str_debug(a) 
	#define print_int_debug(a) 
	#define assert(a)
#endif


#ifdef DEBUG
#define write_fd 2
void print_str(char *s)
{
	write(write_fd,s,strlen(s));
}
void print_int(int x)
{
	char buf[100];
	itox(x,buf);
	write(write_fd,buf,strlen(buf));
}
#endif



typedef struct 
{
        int edi;
        int esi;
        int ebp;
        int esp_dummy;
        int ebx;
        int edx;
        int ecx;
        int eax;
        int flags;
} reg_values_t;

#define PAGESIZE 4096
#define MAXADDRSPERPAGE ((PAGESIZE/(2*sizeof(int)))-(8*sizeof(int)))

typedef struct addr_list addr_list_t;
struct addr_list
{
	int pairs;
	addr_list_t *next;
	struct
	{
		int start;
		int end;
	} addrs[MAXADDRSPERPAGE];
};

addr_list_t *OK_list=NULL;


void add_to_list(int start, int end)
{
#ifdef DEBUG_ADD
	print_str_debug("starting search for space.\n");
	print_str_debug("OK list = ");
	print_int((int)OK_list);
	print_str_debug("\n");

	print_str_debug("adding to OK list, start = ");
	print_int((int)start);
	print_str_debug(", end = ");
	print_int((int)end);
	print_str_debug("\n");
#endif


	addr_list_t* al=OK_list;	
	int page_no=0;
	while(al)
	{
		page_no++;
		//print_str_debug("checking for space.\n");
		if(al->pairs<MAXADDRSPERPAGE)
		{
#ifdef DEBUG_ADD
			print_str_debug("Found space in this list_entry.  page=");
			print_int_debug(page_no);
			print_str_debug(", slot=");
			print_int_debug(al->pairs);
			print_str_debug("\n");
#endif
			al->addrs[al->pairs].start=start;
			al->addrs[al->pairs].end  =end;
			al->pairs++;
			return;
		}
		al=al->next;
	}

#ifdef DEBUG_ADD
	print_str_debug("No space in any page.  Adding page=");
	print_int_debug(page_no);
	print_str_debug(", slot=");
	print_int_debug(0);
	print_str_debug("\n");
#endif
	// no space in any list item.
	assert(sizeof(addr_list_t));
	al=malloc(sizeof(addr_list_t));
	al->next=OK_list;
	al->pairs=1;
	al->addrs[0].start=start;
	al->addrs[0].end=end;
	OK_list=al;
}




void zipr_init_addrs( int ret, int start, int end)
{
#ifdef DEBUG
//	char *str=" in init:  start=";
//	print_str("in init: start=");
//	print_int(start);
//	print_str(" end=");
//	print_int(end);
//	print_str("\n");
#endif
	add_to_list(start,end);
	
}


/* 
 * zipr_post_allocate_watcher - this function is called from a CGC binary after the allocate() system
 * call is executed.  eax=syscall return value, ebx=size, ecx=is_executable and edx=(void**)pointer to new memory .
 */
void zipr_post_allocate_watcher(int ret, reg_values_t rv)
{
#ifdef DEBUG
//	print_str("eax=");
//	print_int(rv.eax);
//	print_str(" ebx=");
//	print_int(rv.ebx);
//	print_str(" ecx=");
//	print_int(rv.ecx);
//	print_str(" edx=");
//	print_int(rv.edx);
//	print_str(" *edx=");
//	print_int(*(int*)rv.edx);
//	print_str("\n");
#endif
	int start=*(int*)(rv.edx);
	int end=start+rv.ebx;
	add_to_list(start,end);
	
}


/*
 * zipr_is_addr_ok - called when a potentially dangerous memory operation is called.
 * return 1 (true) if OK, 0 (false) if not.
 */
int zipr_is_addr_ok(int ret, unsigned int to_check)
{
#ifdef DEBUG_ADD
	print_str_debug("In zipr_is_addr_ok, addr=");
	print_int_debug(to_check);
	print_str_debug(", ret=");
	print_int_debug(ret);
	print_str_debug("\n");
#endif

	// CGC binaries define initial stack pointer as 0xbaaaaffc.  not sure why, but that's it.
	if( (uintptr_t)&to_check < (uintptr_t)to_check && (uintptr_t)to_check < (uintptr_t)0xbaaaaffc )
	{
#ifdef DEBUG_ADD
		print_str_debug("found stack addr.\n");
#endif
		return 1;
	}
#ifdef DEBUG_ADD
	else
	{
		print_str_debug("didn't find stack addr to_check=  ");
		print_int_debug((int)&to_check);
		print_str_debug("\n");
	}
#endif 

        addr_list_t* al=OK_list;
        int page_no=0;
        while(al)
        {
                page_no++;
		int i;
		for(i=0;i<al->pairs;i++)
		{
                        if (al->addrs[al->pairs].start<to_check && to_check <= al->addrs[al->pairs].end)
			{
				print_str_debug("Detected addr as OK\n");
				return 1;
			}
		
		}
                al=al->next;
        }

#ifdef DEBUG
	print_str("Detected about-to-segfault\n");
#endif
	return 0;
}
