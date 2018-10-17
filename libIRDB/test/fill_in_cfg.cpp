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

#include "fill_in_cfg.hpp"
#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include "elfio/elfio.hpp"
#include "split_eh_frame.hpp"

using namespace libIRDB;
using namespace std;
using namespace EXEIO;

void PopulateCFG::populate_instruction_map
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

void PopulateCFG::set_fallthrough
	(
	map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
	DecodedInstruction_t *disasm, Instruction_t *insn, FileIR_t *firp
	)
{
	assert(disasm);
	assert(insn);

	if(insn->GetFallthrough())
		return;
	
	// check for branches with targets 
	if(
		//(disasm->Instruction.BranchType==JmpType) ||			// it is a unconditional branch 
		//(disasm->Instruction.BranchType==RetType)			// or a return

		(disasm->isUnconditionalBranch() ) ||	// it is a unconditional branch 
		(disasm->isReturn())			// or a return
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


void PopulateCFG::set_target
	(
	map< pair<db_id_t,virtual_offset_t>, Instruction_t*> &insnMap,
	DecodedInstruction_t *disasm, Instruction_t *insn, FileIR_t *firp
	)
{

	assert(insn);
	assert(disasm);

	if(insn->GetTarget())
		return;
	
	// check for branches with targets 
	if(
		//(disasm->Instruction.BranchType!=0) &&			// it is a branch 
		//(disasm->Instruction.BranchType!=RetType) && 		// and not a return
		//(disasm->Argument1.ArgType & CONSTANT_TYPE)!=0		// and has a constant argument type 1
		(disasm->isBranch()) &&			// it is a branch 
		(!disasm->isReturn()) && 		// and not a return
		(disasm->getOperand(0).isConstant())		// and has a constant argument type 1
	  )
	{
//		cout<<"Found direct jump with addr=" << insn->GetAddress()->GetVirtualOffset() <<
//			" disasm="<<disasm->CompleteInstr<<" ArgMnemonic="<<
//			disasm->Argument1.ArgMnemonic<<"."<<endl;

		/* get the offset */
		// virtual_offset_t virtual_offset=strtoul(disasm->Argument1.ArgMnemonic, NULL, 16);
		virtual_offset_t virtual_offset=disasm->getAddress();

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

File_t* PopulateCFG::find_file(FileIR_t* firp, db_id_t fileid)
{
	assert(firp->GetFile()->GetBaseID()==fileid);
	return firp->GetFile();
}


void PopulateCFG::add_new_instructions(FileIR_t *firp)
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



        	int secnum = elfiop->sections.size(); 
		int secndx=0;

		bool found=false;
	
        	/* look through each section and find the missing target*/
        	for (secndx=1; secndx<secnum; secndx++)
		{
        		/* not a loaded section */
        		if( !elfiop->sections[secndx]->isLoadable()) 
                		continue;
		
        		/* loaded, and contains instruction, record the bounds */
        		if( !elfiop->sections[secndx]->isExecutable()) 
                		continue;
		
        		virtual_offset_t first=elfiop->sections[secndx]->get_address();
        		virtual_offset_t second=elfiop->sections[secndx]->get_address()+elfiop->sections[secndx]->get_size();

			/* is the missed instruction in this section */
			if(first<=missed_address && missed_address<second)
			{
				const char* data=elfiop->sections[secndx]->get_data();
				// second=data?
				virtual_offset_t offset_into_section=missed_address-elfiop->sections[secndx]->get_address();
	
				/* disassemble the instruction */
				DecodedInstruction_t disasm(missed_address, (void*)&data[offset_into_section], elfiop->sections[secndx]->get_size()-offset_into_section );

				/*
                		memset(&disasm, 0, sizeof(DecodeInstruction_t));

                		disasm.Options = NasmSyntax + PrefixedNumeral;
                		disasm.Archi = firp->GetArchitectureBitWidth();
                		disasm.EIP = (uintptr_t) &data[offset_into_section];
				disasm.SecurityBlock=elfiop->sections[secndx]->get_size()-offset_into_section;
                		disasm.VirtualAddr = missed_address;
				*/


				



/* bea docs say OUT_OF_RANGE and UNKNOWN_OPCODE are defined, but they aren't */
// #define OUT_OF_RANGE (0)
// #define UNKNOWN_OPCODE (-1) 

				/* if we found the instruction, but can't disassemble it, then we skip out for now */
				if(!disasm.valid()) // instr_len==OUT_OF_RANGE || instr_len==UNKNOWN_OPCODE)
				{
					if(getenv("VERBOSE_CFG"))
						cout<<"Found invalid insn at "<<missed_address<<endl;
					break;
				}
				else if(getenv("VERBOSE_CFG"))
					cout<<"Found valid insn at "<<missed_address<<": "<<disasm.getDisassembly()<<endl;

                		const auto instr_len = disasm.length();

				/* intel instructions have a max size of 16 */
				assert(1<=instr_len && instr_len<=16);


				/* here we are certain we found the instruction  */
				found=true;

				/* get the new bits for an instruction */
				string newinsnbits;
				newinsnbits.resize(instr_len);
				for(auto i=0U;i<instr_len;i++)
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
				newinsn->SetComment(disasm.getDisassembly()+string(" from fill_in_cfg "));
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

void PopulateCFG::fill_in_cfg(FileIR_t *firp)
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
      			DecodedInstruction_t disasm(insn);
      			//memset(&disasm, 0, sizeof(DISASM));
	
      			const auto instr_len = disasm.length();
	
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

bool PopulateCFG::is_in_relro_segment(const int secndx)
{
	ELFIO::elfio *real_elfiop = reinterpret_cast<ELFIO::elfio*>(elfiop->get_elfio()); 
	if(!real_elfiop)
		return false;

	int segnum = real_elfiop->segments.size();
//	int segndx=0;

	virtual_offset_t sec_start=(virtual_offset_t)(elfiop->sections[secndx]->get_address());
	virtual_offset_t sec_end=(virtual_offset_t)(elfiop->sections[secndx]->get_address() + elfiop->sections[secndx]->get_size() - 1 );

	/* look through each section */
	for (int segndx=1; segndx<segnum; segndx++)
	{
		ELFIO::Elf_Word type=real_elfiop->segments[segndx]->get_type();
#ifndef PT_GNU_RELRO
#define PT_GNU_RELRO    0x6474e552      /* Read-only after relocation */
#endif

		if(type==PT_GNU_RELRO)
		{
			virtual_offset_t seg_start=(virtual_offset_t)(real_elfiop->segments[segndx]->get_virtual_address());
			virtual_offset_t seg_end=(virtual_offset_t)(real_elfiop->segments[segndx]->get_virtual_address() + real_elfiop->segments[segndx]->get_memory_size() - 1 );

			// check if start lies within 
			if(seg_start <= sec_start  && sec_start <= seg_end)
				return true;

			// check if end lies within 
			if(seg_start <= sec_end  && sec_end <= seg_end)
				return true;
			
			// check if crosses
			if(sec_start < seg_start  && seg_end < sec_end)
				return true;
		}
	}

	return false;
}

void PopulateCFG::fill_in_scoops(FileIR_t *firp)
{

	auto max_base_id=firp->GetMaxBaseID();
	auto secnum = elfiop->sections.size();
	auto secndx=0;

	/* look through each section */
	for (secndx=1; secndx<secnum; secndx++)
	{
		/* not a loaded section, try next section */
		if(!elfiop->sections[secndx]->isLoadable()) 
		{
			cout<<"Skipping scoop for section (not loadable) "<<elfiop->sections[secndx]->get_name()<<endl;
			continue;
		}

        	if(elfiop->sections[secndx]->isWriteable() && elfiop->sections[secndx]->isExecutable()) 
		{
			ofstream fout("warning.txt");
			fout<<"Found that section "<<elfiop->sections[secndx]->get_name()<<" is both writeable and executable.  Program is inherently unsafe!"<<endl;
		}

		/* executable sections handled by zipr/spri. */
        	if(elfiop->sections[secndx]->isExecutable()) 
		{
			cout<<"Skipping scoop for section (executable) "<<elfiop->sections[secndx]->get_name()<<endl;
                	continue;
		}
		/* name */
		string name=elfiop->sections[secndx]->get_name();

		/* start address */
		AddressID_t *startaddr=new AddressID_t();
		assert(startaddr);
		startaddr->SetVirtualOffset( elfiop->sections[secndx]->get_address());
		startaddr->SetFileID(firp->GetFile()->GetBaseID());
		firp->GetAddresses().insert(startaddr);

		/* end */
		AddressID_t *endaddr=new AddressID_t();
		assert(endaddr);
		endaddr->SetVirtualOffset( elfiop->sections[secndx]->get_address() + elfiop->sections[secndx]->get_size()-1);
		endaddr->SetFileID(firp->GetFile()->GetBaseID());
		firp->GetAddresses().insert(endaddr);

		string the_contents;
		the_contents.resize(elfiop->sections[secndx]->get_size()); 
		// deal with .bss segments that are 0 init'd.
		if (elfiop->sections[secndx]->get_data()) 
			the_contents.assign(elfiop->sections[secndx]->get_data(),elfiop->sections[secndx]->get_size());

//		Type_t *chunk_type=NULL; /* FIXME -- need to figure out the type system for scoops, but NULL should remain valid */

		/* permissions */
		int permissions= 
			( elfiop->sections[secndx]->isReadable() << 2 ) | 
			( elfiop->sections[secndx]->isWriteable() << 1 ) | 
			( elfiop->sections[secndx]->isExecutable() << 0 ) ;

		bool is_relro=is_in_relro_segment(secndx);
		DataScoop_t *newscoop=new DataScoop_t(max_base_id++, name, startaddr, endaddr, NULL, permissions, is_relro, the_contents);
		assert(newscoop);
		firp->GetDataScoops().insert(newscoop);

		cout<<"Allocated new scoop for section "<<name
		    <<"("<<hex<<startaddr->GetVirtualOffset()<<"-"
		    <<hex<<endaddr->GetVirtualOffset()<<")"
		    <<" perms="<<permissions<<" relro="<<boolalpha<<is_relro<<endl;

	}

}

void PopulateCFG::fill_in_landing_pads(FileIR_t *firp)
{
	const auto eh_frame_rep_ptr = split_eh_frame_t::factory(firp);
	// eh_frame_rep_ptr->parse(); already parsed now.
	if(getenv("EHIR_VERBOSE"))
		eh_frame_rep_ptr->print();
	cout<<"Completed eh-frame parsing"<<endl;

	map<Function_t*,set<Instruction_t*> > insns_to_add_to_funcs;

	// for_each(firp->GetInstructions().begin(), firp->GetInstructions().end(), [&](Instruction_t* t)
	for(const auto t : firp->GetInstructions())
	{
		if(t->GetFunction()==NULL)
			continue;
		auto lp=eh_frame_rep_ptr->find_lp(t);
		if(lp && lp->GetFunction()==NULL)
			insns_to_add_to_funcs[t->GetFunction()].insert(lp);
	};


	// for_each(insns_to_add_to_funcs.begin(), insns_to_add_to_funcs.end(), [&](pair<Function_t* const,set<Instruction_t*> > & p)
	for(const auto & p : insns_to_add_to_funcs)
	{
		auto & func=p.first; 	
		auto insns=p.second; 	/* copy */
		auto insn_count=0;

		while(insns.size()>0 )
		{
			auto it=insns.begin();
			auto insn=*it;
			insns.erase(it);

			assert(insn);
			if(insn->GetFunction()!=NULL)
				continue;

			auto lp=eh_frame_rep_ptr->find_lp(insn);
			if(lp && lp->GetFunction()==NULL)
				insns.insert(lp);

			insn->SetFunction(func);
			cout<<"	Adding "<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<"@"<<hex<<insn->GetAddress()->GetVirtualOffset()<<dec<<endl;
			insn_count++;
			

			auto target=insn->GetTarget();
			auto fallthru=insn->GetFallthrough();

			if(target) insns.insert(target);
			if(fallthru) insns.insert(fallthru);
		}
		cout<<"Found LP outside of function "<<func->GetName()<<" added "<<insn_count<<" instructions"<<endl;
	};
	
}

int PopulateCFG::ParseArgs
        (
        int argc, 
        char* argv[]
        )
{   
    if(argc<2)
    {
            cerr<<"Usage: fill_in_cfg <id> [--fix-landing-pads | --no-fix-landing-pads]"<<endl;
            return -1;
    }

    variant_id = atoi(argv[1]);
    
    for (int i = 2; i < argc; ++i)
    {
            if (strcmp("--fix-landing-pads", argv[i]) == 0)
            {
                    fix_landing_pads = true;
            }
            else if (strcmp("--no-fix-landing-pads", argv[i]) == 0)
            {
                    fix_landing_pads = false;
            }
    }
    
    return 0;
}

int PopulateCFG::ExecuteStep(IRDBObjects_t *irdb_objects)
{
    try 
	{
		pqxx_interface = irdb_objects->GetDBInterface();
		// now set the DB interface for THIS PLUGIN LIBRARY -- VERY IMPORTANT
		BaseObj_t::SetInterface(pqxx_interface);	

		shared_ptr<VariantID_t> variant = irdb_objects->AddVariant(variant_id);
		for(File_t* file : variant->GetFiles())
		{
			shared_ptr<FileIR_t> firp = irdb_objects->AddFileIR(variant_id, file->GetBaseID());
			assert(firp);
                        cout<<"Filling in cfg for "<<firp->GetFile()->GetURL()<<endl;

			/* get the OID of the file */
			int elfoid=firp->GetFile()->GetELFOID();

			pqxx::largeobject lo(elfoid);
                	lo.to_file(pqxx_interface->GetTransaction(),"readeh_tmp_file.exe");

			elfiop=new EXEIO::exeio;
			assert(elfiop);
			elfiop->load(string("readeh_tmp_file.exe"));

			fill_in_cfg(firp.get());
			fill_in_scoops(firp.get());

			if (fix_landing_pads)
			{
				fill_in_landing_pads(firp.get());
			}

			delete elfiop;
			elfiop=NULL;
		}
	}
	catch (DatabaseError_t pnide)
	{
		cerr<<"Unexpected database error: "<<pnide<<endl;
		return -1;
        }
	catch(...)
	{
		cerr<<"Unexpected error"<<endl;
		return -1;
	}
    
    return 0;
}


extern "C"
Transform_SDK::TransformStep_t* GetTransformStep(void)
{
	return new PopulateCFG();
}
