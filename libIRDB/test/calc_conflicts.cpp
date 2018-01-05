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



#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <elf.h>
#include <sys/mman.h>
#include <ctype.h>
#include "targ-config.h"
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"



#include "beaengine/BeaEngine.h"

int odd_target_count=0;
int bad_target_count=0;
int bad_fallthrough_count=0;

using namespace libIRDB;
using namespace std;

set< pair<db_id_t,int> > missed_instructions;
int failed_target_count=0;

pqxxDB_t pqxx_interface;

long long num_addresses=0,num_conflicts=0,tot_conflicts=0;
long long total_bytes=0;

void count_conflicts (FileIR_t* virp)
{
	map<int,int> conflict_map;


	/* for each instruction in the IR */
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t *insn=*it;

		virtual_offset_t vo=insn->GetAddress()->GetVirtualOffset();

		for(int i=0;i<insn->GetDataBits().length();i++)
			conflict_map[(vo+i)]++;
	}


	for(map<int,int>::const_iterator it=conflict_map.begin();
		it!=conflict_map.end();
		++it)
	{
		int address=(*it).first;
		int conflicts=(*it).second;

		num_addresses++;
		if(conflicts>1)
		{
			num_conflicts++;
			tot_conflicts+=conflicts;
		}
			


	}

}




void get_total(char* filename)
{

	FILE* loa=fopen(filename, "r");
	
	assert(loa && "Cannot open input file");
		

       	IRDB_Elf_Off sec_hdr_off, sec_off;
       	IRDB_Elf_Half secnum, strndx, secndx;
       	IRDB_Elf_Word secsize;
	

       	/* allcoate memory  */
       	IRDB_Elf_Ehdr elfhdr;

       	/* Read ELF header */
       	fread((char*)&elfhdr, sizeof(IRDB_Elf_Ehdr), 1, loa);

       	sec_hdr_off = elfhdr.e_shoff;
       	secnum = elfhdr.e_shnum;
       	strndx = elfhdr.e_shstrndx;

       	/* Read Section headers */
       	IRDB_Elf_Shdr *sechdrs=(IRDB_Elf_Shdr*)malloc(sizeof(IRDB_Elf_Shdr)*secnum);
       	fseek(loa,sec_hdr_off, std::ios_base::beg);
       	fread((char*)sechdrs, sizeof(IRDB_Elf_Shdr), secnum, loa);


	
       	/* look through each section and find the missing target*/
       	for (secndx=1; secndx<secnum; secndx++)
	{
       		int flags = sechdrs[secndx].sh_flags;

       		/* not a loaded section */
       		if( (flags & SHF_ALLOC) != SHF_ALLOC)
               		continue;
		
       		/* loaded, and contains instruction, record the bounds */
       		if( (flags & SHF_EXECINSTR) != SHF_EXECINSTR)
               		continue;


        	total_bytes+=sechdrs[secndx].sh_size;
	}


}



main(int argc, char* argv[])
{

	if(argc!=3)
	{
		cerr<<"Usage: calc_conflicts <id> <a.out>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t * virp=NULL;

	try 
	{
		/* setup the interface to the sql server */
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

		// read the db  
		virp=new FileIR_t(*pidp);

		get_total(argv[2]);
		count_conflicts(virp);

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(virp && pidp);

	cout<<"# ATTRIBUTE calc_conflicts::num_addresses="<<std::dec<<num_addresses<<endl;
	cout<<"# ATTRIBUTE calc_conflicts::num_conflicts="<<std::dec<<num_conflicts<<endl;
	cout<<"# ATTRIBUTE calc_conflicts::total_conflicts="<<std::dec<<tot_conflicts<<endl;
	cout<<"# ATTRIBUTE calc_conflicts::pct_conflicts="<<std::fixed<<((float)num_conflicts/(float)tot_conflicts)*100.00<<"%"<<endl;
	cout<<"# ATTRIBUTE calc_conflicts::ave_conflicts="<<std::dec<<((double)tot_conflicts/num_addresses)<<endl;
	cout<<"# ATTRIBUTE calc_conflicts::ave_bytes_conflicted="<<std::dec<<((double)num_conflicts/num_addresses)<<endl;
	cout <<"# ATTRIBUTE calc_conflicts::total_executable_bytes = "<< std::dec << total_bytes<<endl;
	cout<<"# ATTRIBUTE calc_conflicts::ave_conflicts_within_exe="<<std::dec<<((double)tot_conflicts/total_bytes)<<endl;
	cout<<"# ATTRIBUTE calc_conflicts::ave_bytes_conflicted_within_exe="<<std::dec<<((double)num_conflicts/total_bytes)<<endl;


	delete virp;
	delete pidp;
}
