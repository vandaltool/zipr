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
#include <sys/mman.h>
#include <ctype.h>

#include <exeio.h>

#include "beaengine/BeaEngine.h"

int odd_target_count=0;
int bad_target_count=0;
int bad_fallthrough_count=0;

using namespace libIRDB;
using namespace std;
using namespace EXEIO;

set< pair<db_id_t,virtual_offset_t> > missed_instructions;
int failed_target_count=0;

pqxxDB_t pqxx_interface;

void populate_instruction_map
	(
		map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
		FileIR_t *firp
	)
{
	/* start from scratch each time */
	insnMap.clear();


	/* for each instruction in the IR */
	for(
		set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t *insn=*it;
		db_id_t fileID=insn->GetAddress()->GetFileID();
		virtual_offset_t vo=insn->GetAddress()->GetVirtualOffset();

		pair<db_id_t,virtual_offset_t> p(fileID,vo);

		assert(insnMap[p]==NULL);
		insnMap[p]=insn;
	}

}

void set_fallthrough
	(
	map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
	DISASM *disasm, Instruction_t *insn, FileIR_t *firp
	)
{
	assert(disasm);
	assert(insn);

	if(insn->GetFallthrough())
		return;
	
	// check for branches with targets 
	if(
		(disasm->Instruction.BranchType==JmpType) ||			// it is a unconditional branch 
		(disasm->Instruction.BranchType==RetType)			// or a return
	  )
	{
		// this is a branch with no fallthrough instruction
		return;
	}

	/* get the address of the next instrution */
	
	virtual_offset_t virtual_offset=insn->GetAddress()->GetVirtualOffset() + insn->GetDataBits().size();

	/* create a pair of offset/file */
	pair<db_id_t,virtual_offset_t> p(insn->GetAddress()->GetFileID(),virtual_offset);
	
	/* lookup the target insn from the map */
	Instruction_t *fallthrough_insn=insnMap[p];

	/* sanity, note we may see odd control transfers to 0x0 */
	if(fallthrough_insn==NULL &&   virtual_offset!=0)
	{
		cout<<"Cannot set fallthrough for "<<std::hex<<insn->GetAddress()->GetVirtualOffset();
		cout<< " : "<<insn->getDisassembly()<<endl;
		bad_fallthrough_count++;
	}

	/* set the target for this insn */
	if(fallthrough_insn!=0)
		insn->SetFallthrough(fallthrough_insn);
	else
		missed_instructions.insert(pair<db_id_t,virtual_offset_t>(insn->GetAddress()->GetFileID(),virtual_offset));
}


void set_target
	(
	map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
	DISASM *disasm, Instruction_t *insn, FileIR_t *firp
	)
{

	assert(insn);
	assert(disasm);

	if(insn->GetTarget())
		return;
	
	// check for branches with targets 
	if(
		(disasm->Instruction.BranchType!=0) &&			// it is a branch 
		(disasm->Instruction.BranchType!=RetType) && 		// and not a return
		(disasm->Argument1.ArgType & CONSTANT_TYPE)!=0		// and has a constant argument type 1
	  )
	{
//		cout<<"Found direct jump with addr=" << insn->GetAddress()->GetVirtualOffset() <<
//			" disasm="<<disasm->CompleteInstr<<" ArgMnemonic="<<
//			disasm->Argument1.ArgMnemonic<<"."<<endl;

		/* get the offset */
		virtual_offset_t virtual_offset=strtoul(disasm->Argument1.ArgMnemonic, NULL, 16);

		/* create a pair of offset/file */
		pair<db_id_t,virtual_offset_t> p(insn->GetAddress()->GetFileID(),virtual_offset);
	
		/* lookup the target insn from the map */
		Instruction_t *target_insn=insnMap[p];

		/* sanity, note we may see odd control transfers to 0x0 */
		if(target_insn==NULL)
		{
			unsigned char first_byte=0;
			if(insn->GetFallthrough())
				first_byte=(insn->GetFallthrough()->GetDataBits().c_str())[0];
			virtual_offset_t jump_dist=virtual_offset-(insn->GetAddress()->GetVirtualOffset()+(insn->GetDataBits()).size());
			if(	
				// jump 1 byte forward
				jump_dist == 1 &&

				// and we calculated the fallthrough
				insn->GetFallthrough()!=NULL &&

				// and the fallthrough starts with a lock prefix
				first_byte==0xf0
			  )
			{
				odd_target_count++;
				target_insn=insn->GetFallthrough();
			}
			else
			{
				if(virtual_offset!=0)
					cout<<"Cannot set target (target="<< std::hex << virtual_offset << ") for "<<std::hex<<insn->GetAddress()->GetVirtualOffset()<<"."<<endl;
				bad_target_count++;
			}
		}

		/* set the target for this insn */
		if(target_insn!=0)
			insn->SetTarget(target_insn);
		else
			missed_instructions.insert( pair<db_id_t,virtual_offset_t>(insn->GetAddress()->GetFileID(),virtual_offset));

	}
}

static File_t* find_file(FileIR_t* firp, db_id_t fileid)
{
#if 0
	set<File_t*> &files=firp->GetFiles();

	for(
		set<File_t*>::iterator it=files.begin();
		it!=files.end();
		++it
	   )
	{
		File_t* thefile=*it;
		if(thefile->GetBaseID()==fileid)
			return thefile;
	}
	return NULL;
#endif
	assert(firp->GetFile()->GetBaseID()==fileid);
	return firp->GetFile();

}

EXEIO::exeio    elfiop;

void add_new_instructions(FileIR_t *firp)
{
	int found_instructions=0;
	for(
		set< pair<db_id_t,virtual_offset_t> >::const_iterator it=missed_instructions.begin();
		it!=missed_instructions.end(); 
		++it
   	   )
	{
		/* get the address we've missed */
		virtual_offset_t missed_address=(*it).second;

		/* get the address ID of the instruction that's missing the missed addressed */
		db_id_t missed_fileid=(*it).first;
		
		/* figure out which file we're looking at */
		File_t* filep=find_file(firp,missed_fileid);
		assert(filep);



#if 0
        	::Elf64_Off sec_hdr_off, sec_off;
        	::Elf_Half secnum, strndx, secndx;
        	::Elf_Word secsize;
	
#endif
        	int secnum = elfiop.sections.size(); 
		int secndx=0;

		bool found=false;
	
        	/* look through each section and find the missing target*/
        	for (secndx=1; secndx<secnum; secndx++)
		{
//        		int flags = elfiop.sections[secndx]->get_flags();

        		/* not a loaded section */
        		if( !elfiop.sections[secndx]->isLoadable()) // (flags & SHF_ALLOC) != SHF_ALLOC)
                		continue;
		
        		/* loaded, and contains instruction, record the bounds */
        		// if( (flags & SHF_EXECINSTR) != SHF_EXECINSTR)
        		if( !elfiop.sections[secndx]->isExecutable()) 
                		continue;
		
        		virtual_offset_t first=elfiop.sections[secndx]->get_address();
        		virtual_offset_t second=elfiop.sections[secndx]->get_address()+elfiop.sections[secndx]->get_size();

			/* is the missed instruction in this section */
			if(first<=missed_address && missed_address<second)
			{
				const char* data=elfiop.sections[secndx]->get_data();
				// second=data?
				virtual_offset_t offset_into_section=missed_address-elfiop.sections[secndx]->get_address();
	
				/* disassemble the instruction */
				DISASM disasm;
                		memset(&disasm, 0, sizeof(DISASM));

                		disasm.Options = NasmSyntax + PrefixedNumeral;
                		disasm.Archi = firp->GetArchitectureBitWidth();
                		disasm.EIP = (UIntPtr) &data[offset_into_section];
				disasm.SecurityBlock=elfiop.sections[secndx]->get_size()-offset_into_section;
                		disasm.VirtualAddr = missed_address;
                		int instr_len = Disasm(&disasm);


/* bea docs say OUT_OF_RANGE and UNKNOWN_OPCODE are defined, but they aren't */
#define OUT_OF_RANGE (0)
#define UNKNOWN_OPCODE (-1) 

				/* if we found the instruction, but can't disassemble it, then we skip out for now */
				if(instr_len==OUT_OF_RANGE || instr_len==UNKNOWN_OPCODE)
				{
					if(getenv("VERBOSE_CFG"))
						cout<<"Found invalid insn at "<<missed_address<<endl;
					break;
				}
				else if(getenv("VERBOSE_CFG"))
					cout<<"Found valid insn at "<<missed_address<<": "<<disasm.CompleteInstr<<endl;

				/* intel instructions have a max size of 16 */
				assert(1<=instr_len && instr_len<=16);


				/* here we are certain we found the instruction  */
				found=true;

				/* get the new bits for an instruction */
				string newinsnbits;
				newinsnbits.resize(instr_len);
				for(int i=0;i<instr_len;i++)
					newinsnbits[i]=data[offset_into_section+i];

				/* create a new address */
				AddressID_t *newaddr=new AddressID_t();
				assert(newaddr);
				newaddr->SetVirtualOffset(missed_address);
				newaddr->SetFileID(missed_fileid);

				/* create a new instruction */
				Instruction_t *newinsn=new Instruction_t();
				assert(newinsn);
				newinsn->SetAddress(newaddr);
				newinsn->SetDataBits(newinsnbits);
				newinsn->SetComment(string(disasm.CompleteInstr)+string(" from fill_in_cfg "));
				newinsn->SetAddress(newaddr);
				/* fallthrough/target/is indirect will be set later */

				/* insert into the IR */
				firp->GetInstructions().insert(newinsn);
				firp->GetAddresses().insert(newaddr);


				cout<<"Found new instruction, "<<newinsn->GetComment()<<", at "<<std::hex<<newinsn->GetAddress()->GetVirtualOffset()<<" in file "<<"<no name yet>"<<"."<<endl; 
				found_instructions++;
			}
		
		}
		if(!found)
		{
			failed_target_count++;
	
			cout<<"Cannot find address "<<std::hex<<missed_address<<" in file "<<"<no name yet>"<<"."<<endl; 
		} 
	}	
	cout<<"Found a total of "<<std::dec<<found_instructions<<" new instructions."<<endl;

}

void fill_in_cfg(FileIR_t *firp)
{
	int round=0;
	
	do
	{
		bad_target_count=0;
		bad_fallthrough_count=0;
		failed_target_count=0;
		missed_instructions.clear();

		map< pair<db_id_t,virtual_offset_t>, Instruction_t*> insnMap;
		populate_instruction_map(insnMap, firp);

		cout << "Found "<<firp->GetInstructions().size()<<" instructions." <<endl;

		/* for each instruction, disassemble it and set the target/fallthrough */
		for(
			set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
			it!=firp->GetInstructions().end(); 
			++it
	   	   )
		{
			Instruction_t *insn=*it;
      			DISASM disasm;
      			memset(&disasm, 0, sizeof(DISASM));
	
      			int instr_len = insn->Disassemble(disasm);
	
			assert(instr_len==insn->GetDataBits().size());
	
			set_fallthrough(insnMap, &disasm, insn, firp);
			set_target(insnMap, &disasm, insn, firp);
			
		}
		if(bad_target_count>0)
			cout<<std::dec<<"Found "<<bad_target_count<<" bad targets at round "<<round<<endl;
		if(bad_fallthrough_count>0)
			cout<<"Found "<<bad_fallthrough_count<<" bad fallthroughs at round "<<round<<endl;
		cout<<"Missed instruction count="<<missed_instructions.size()<<endl;

		add_new_instructions(firp);

		round++;

	/* keep trying this while we're resolving targets.  if at any point we fail to resolve a new target/fallthrough address, then we give up */
	} while(missed_instructions.size()>failed_target_count);

	cout<<"Caution: Was unable to find instructions for these addresses:"<<hex<<endl;
	for(
		set< pair<db_id_t,virtual_offset_t> >::const_iterator it=missed_instructions.begin();
		it!=missed_instructions.end(); 
		++it
   	   )
	{
		/* get the address we've missed */
		virtual_offset_t missed_address=(*it).second;
		cout << missed_address << ", ";
	}
	cout<<dec<<endl;


	/* set the base IDs for all instructions */
	firp->SetBaseIDS();

	/* for each instruction, set the original address id to be that of the address id, as fill_in_cfg is 
	 * designed to work on only original programs.
	 */
	for(
		std::set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); 
		++it
   	   )
	{
		Instruction_t* insn=*it;

		insn->SetOriginalAddressID(insn->GetAddress()->GetBaseID());
	}


}



main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: fill_in_cfg <id>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t * firp=NULL;

	try 
	{
		/* setup the interface to the sql server */
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

		for(set<File_t*>::iterator it=pidp->GetFiles().begin();
			it!=pidp->GetFiles().end();
			++it
		    )
		{
			File_t* this_file=*it;
			assert(this_file);
			cout<<"Filling in cfg for "<<this_file->GetURL()<<endl;


			// read the db  
			firp=new FileIR_t(*pidp, this_file);

			/* get the OID of the file */
			int elfoid=this_file->GetELFOID();

			pqxx::largeobject lo(elfoid);
                	lo.to_file(pqxx_interface.GetTransaction(),"readeh_tmp_file.exe");

			elfiop.load("readeh_tmp_file.exe");
			EXEIO::dump::header(cout,elfiop);
			EXEIO::dump::section_headers(cout,elfiop);

			fill_in_cfg(firp);

			// write the DB back and commit our changes 
			firp->WriteToDB();
			delete firp;

		}


		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(firp && pidp);


	delete pidp;
}
