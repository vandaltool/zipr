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
#include "back_search.hpp"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <pe_bliss.h>
#include <pe_structures.h>
#pragma GCC diagnostic pop



using namespace std;
using namespace EXEIO;
using namespace IRDB_SDK;
using namespace PopCFG;

template < typename T > 
static inline std::string to_hex_string( const T& n )
{
        std::ostringstream stm ;
        stm << std::hex<< "0x"<< n ;
        return stm.str() ;
}


void PopulateCFG::populate_instruction_map
	(
		InstructionMap_t &insnMap,
		FileIR_t *firp
	)
{
	/* start from scratch each time */
	insnMap.clear();

	/* for each instruction in the IR */
	for(auto insn : firp->getInstructions())
	{
		const auto fileID= insn->getAddress()->getFileID();
		const auto vo    = insn->getAddress()->getVirtualOffset();
		const auto p     = pair<DatabaseID_t,VirtualOffset_t>(fileID,vo);

		assert(insnMap[p]==NULL);
		insnMap[p]=insn;
	}

}

void PopulateCFG::set_fallthrough
	(
	InstructionMap_t &insnMap,
	DecodedInstruction_t *disasm, 
	Instruction_t *insn, 
	FileIR_t *firp
	)
{
	assert(disasm);
	assert(insn);

	if(insn->getFallthrough())
		return;
	
	const auto is_mips = firp->getArchitecture()->getMachineType() == admtMips32;	 
	// always set fallthrough for mips
	// other platforms can end fallthroughs ofr uncond branches and returns
	if(!is_mips) 
	{
		// check for branches with targets 
		if(
			(disasm->isUnconditionalBranch() ) ||	// it is a unconditional branch 
			(disasm->isReturn())			// or a return
		  )
		{
			// this is a branch with no fallthrough instruction
			return;
		}
	}

	/* get the address of the next instrution */
	
	auto virtual_offset=insn->getAddress()->getVirtualOffset() + insn->getDataBits().size();

	/* create a pair of offset/file */
	auto p=pair<DatabaseID_t,VirtualOffset_t>(insn->getAddress()->getFileID(),virtual_offset);
	
	/* lookup the target insn from the map */
	auto fallthrough_insn=insnMap[p];

	/* sanity, note we may see odd control transfers to 0x0 */
	if(fallthrough_insn==NULL &&   virtual_offset!=0)
	{
		cout<<"Cannot set fallthrough for "<<std::hex<<insn->getAddress()->getVirtualOffset();
		cout<< " : "<<insn->getDisassembly()<<endl;
		bad_fallthrough_count++;
	}

	/* set the target for this insn */
	if(fallthrough_insn!=0)
	{
		fallthroughs_set++;
		insn->setFallthrough(fallthrough_insn);
	}
	else
		missed_instructions.insert(pair<DatabaseID_t,VirtualOffset_t>(insn->getAddress()->getFileID(),virtual_offset));
}

void PopulateCFG::set_delay_slots
	(
	InstructionMap_t &insnMap,
	DecodedInstruction_t *disasm, 
	Instruction_t *insn, 
	FileIR_t *firp
	)
{
	const auto is_mips = firp->getArchitecture()->getMachineType() == admtMips32;
	if(!is_mips)
		return;

	// using df=DecodedInstruction_t::factory;
	const auto d=DecodedInstruction_t::factory(insn);
	if(d->isBranch())
	{
		const auto branch_addr     = insn->getAddress();
		const auto delay_slot_insn = insnMap[ {branch_addr->getFileID(), branch_addr->getVirtualOffset() + 4}];
		assert(delay_slot_insn);
		(void)firp->addNewRelocation(insn,0,"delay_slot1", delay_slot_insn, 0);
	}

}


void PopulateCFG::set_target
	(
	InstructionMap_t &insnMap,
	DecodedInstruction_t *disasm, 
	Instruction_t *insn, 
	FileIR_t *firp
	)
{

	assert(insn);
	assert(disasm);

	if(insn->getTarget())
		return;

	const auto &operands = disasm->getOperands();
	for(auto operand : operands)
	{	
		// check for branches with targets 
		if(
			disasm->isBranch() &&		// it is a branch 
			!disasm->isReturn() && 		// and not a return
			operand->isConstant()		// and has a constant argument 
		  )
		{
	//		cout<<"Found direct jump with addr=" << insn->getAddress()->getVirtualOffset() <<
	//			" disasm="<<disasm->CompleteInstr<<" ArgMnemonic="<<
	//			disasm->Argument1.ArgMnemonic<<"."<<endl;

			/* get the offset */
			auto virtual_offset=disasm->getAddress();

			/* create a pair of offset/file */
			auto p=pair<DatabaseID_t,VirtualOffset_t>(insn->getAddress()->getFileID(),virtual_offset);
		
			/* lookup the target insn from the map */
			auto target_insn=insnMap[p];

			/* sanity, note we may see odd control transfers to 0x0 */
			if(target_insn==NULL)
			{
				unsigned char first_byte=0;
				if(insn->getFallthrough())
					first_byte=(insn->getFallthrough()->getDataBits().c_str())[0];
				VirtualOffset_t jump_dist=virtual_offset-(insn->getAddress()->getVirtualOffset()+(insn->getDataBits()).size());
				if(	
					// jump 1 byte forward
					jump_dist == 1 &&

					// and we calculated the fallthrough
					insn->getFallthrough()!=NULL &&

					// and the fallthrough starts with a lock prefix
					first_byte==0xf0
				  )
				{
					odd_target_count++;
					target_insn=insn->getFallthrough();
				}
				else
				{
					if(virtual_offset!=0)
						cout<<"Cannot set target (target="<< std::hex << virtual_offset << ") for "<<std::hex<<insn->getAddress()->getVirtualOffset()<<"."<<endl;
					bad_target_count++;
				}
			}

			/* set the target for this insn */
			if(target_insn!=0)
			{
				targets_set++;
				insn->setTarget(target_insn);
			}
			else
				missed_instructions.insert( pair<DatabaseID_t,VirtualOffset_t>(insn->getAddress()->getFileID(),virtual_offset));

		}
	}
}

File_t* PopulateCFG::find_file(FileIR_t* firp, DatabaseID_t fileid)
{
	assert(firp->getFile()->getBaseID()==fileid);
	return firp->getFile();
}


void PopulateCFG::add_new_instructions(FileIR_t *firp)
{
	int found_instructions=0;
	for(auto p : missed_instructions)
	{
		/* get the address we've missed */
		auto missed_address=p.second;

		/* get the address ID of the instruction that's missing the missed addressed */
		auto missed_fileid=p.first;
		
		/* figure out which file we're looking at */
		auto filep=find_file(firp,missed_fileid);
		assert(filep);



        	int secnum = exeiop->sections.size(); 
		int secndx=0;

		bool found=false;
	
        	/* look through each section and find the missing target*/
        	for (secndx=1; secndx<secnum; secndx++)
		{
        		/* not a loaded section */
        		if( !exeiop->sections[secndx]->isLoadable()) 
                		continue;
		
        		/* loaded, and contains instruction, record the bounds */
        		if( !exeiop->sections[secndx]->isExecutable()) 
                		continue;
		
        		VirtualOffset_t first=exeiop->sections[secndx]->get_address();
        		VirtualOffset_t second=exeiop->sections[secndx]->get_address()+exeiop->sections[secndx]->get_size();

			/* is the missed instruction in this section */
			if(first<=missed_address && missed_address<second)
			{
				const char* data=exeiop->sections[secndx]->get_data();
				// second=data?
				VirtualOffset_t offset_into_section=missed_address-exeiop->sections[secndx]->get_address();
	
				/* disassemble the instruction */
				auto disasm_p=DecodedInstruction_t::factory(missed_address, (void*)&data[offset_into_section], exeiop->sections[secndx]->get_size()-offset_into_section );
				auto &disasm=*disasm_p;



				/* if we found the instruction, but can't disassemble it, then we skip out for now */
				if(!disasm.valid()) 
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
				auto newaddr=firp->addNewAddress(missed_fileid,missed_address);

				/* create a new instruction */
				auto newinsn=firp->addNewInstruction(newaddr, nullptr, newinsnbits, disasm.getDisassembly()+string(" from fill_in_cfg "), nullptr);
				(void)newinsn;// just add to IR

				/* fallthrough/target/is indirect will be set later */

				/* insert into the IR */


				cout<<"Found new instruction, "<<newinsn->getComment()<<", at "<<std::hex<<newinsn->getAddress()->getVirtualOffset()<<" in file "<<"<no name yet>"<<"."<<endl; 
				found_instructions++;
			}
		
		}
		if(!found)
		{
			failed_target_count++;
	
			cout<<"Cannot find address "<<std::hex<<missed_address<<endl;
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

		auto insnMap = InstructionMap_t();
		populate_instruction_map(insnMap, firp);

		cout << "Found "<<firp->getInstructions().size()<<" instructions." <<endl;

		/* for each instruction, disassemble it and set the target/fallthrough */
		for(auto insn : firp->getInstructions())
		{
      			auto disasm=DecodedInstruction_t::factory(insn);
	
      			const auto instr_len = disasm->length();
	
			assert(instr_len==insn->getDataBits().size());
	
			set_fallthrough(insnMap, disasm.get(), insn, firp);
			set_target(insnMap, disasm.get(), insn, firp);
			set_delay_slots(insnMap, disasm.get(), insn, firp);
			
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
	for(auto p : missed_instructions)
	{
		/* get the address we've missed */
		VirtualOffset_t missed_address=p.second;
		cout << missed_address << ", ";
	}
	cout<<dec<<endl;


	/* set the base IDs for all instructions */
	firp->setBaseIDS();

	/* for each instruction, set the original address id to be that of the address id, as fill_in_cfg is 
	 * designed to work on only original programs.
	 */
	for(auto insn : firp->getInstructions())
		insn->setOriginalAddressID(insn->getAddress()->getBaseID());


}

bool PopulateCFG::is_in_relro_segment(const int secndx)
{
	ELFIO::elfio *real_exeiop = reinterpret_cast<ELFIO::elfio*>(exeiop->get_elfio()); 
	if(!real_exeiop)
		return false;

	int segnum = real_exeiop->segments.size();

	VirtualOffset_t sec_start=(VirtualOffset_t)(exeiop->sections[secndx]->get_address());
	VirtualOffset_t sec_end=(VirtualOffset_t)(exeiop->sections[secndx]->get_address() + exeiop->sections[secndx]->get_size() - 1 );

	/* look through each section */
	for (int segndx=1; segndx<segnum; segndx++)
	{
		ELFIO::Elf_Word type=real_exeiop->segments[segndx]->get_type();
#ifndef PT_GNU_RELRO
#define PT_GNU_RELRO    0x6474e552      /* Read-only after relocation */
#endif

		if(type==PT_GNU_RELRO)
		{
			VirtualOffset_t seg_start=(VirtualOffset_t)(real_exeiop->segments[segndx]->get_virtual_address());
			VirtualOffset_t seg_end=(VirtualOffset_t)(real_exeiop->segments[segndx]->get_virtual_address() + real_exeiop->segments[segndx]->get_memory_size() - 1 );

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

	auto max_base_id=firp->getMaxBaseID();
	auto secnum = exeiop->sections.size();

	/* look through each section */
	for (auto secndx=1; secndx<secnum; secndx++)
	{
		/* not a loaded section, try next section */
		if(!exeiop->sections[secndx]->isLoadable()) 
		{
			cout << "Skipping scoop for section (not loadable) " << exeiop->sections[secndx]->get_name() << endl;
			continue;
		}

        	if(exeiop->sections[secndx]->isWriteable() && exeiop->sections[secndx]->isExecutable()) 
		{
			ofstream fout("warning.txt");
			fout << "Found that section " << exeiop->sections[secndx]->get_name() << " is both writeable and executable.  Program is inherently unsafe!" << endl;
		}

		/* executable sections handled by zipr/spri. */
        	if(exeiop->sections[secndx]->isExecutable()) 
		{
			cout << "Skipping scoop for section (executable) " << exeiop->sections[secndx]->get_name() << endl;
                	continue;
		}
		/* name */
		const auto name=string(exeiop->sections[secndx]->get_name());

		/* start and end address */
		auto startaddr = firp->addNewAddress(firp->getFile()->getBaseID(), exeiop->sections[secndx]->get_address());
		auto endaddr   = firp->addNewAddress(firp->getFile()->getBaseID(), exeiop->sections[secndx]->get_address() + exeiop->sections[secndx]->get_size()-1);


		auto the_contents=string(exeiop->sections[secndx]->get_size(), '\0'); 
		// deal with .bss segments that are 0 init'd.
		if (exeiop->sections[secndx]->get_data()) 
			the_contents.assign(exeiop->sections[secndx]->get_data(),exeiop->sections[secndx]->get_size());

		/* permissions */
		int permissions= 
			( exeiop->sections[secndx]->isReadable()   << 2 ) | 
			( exeiop->sections[secndx]->isWriteable()  << 1 ) | 
			( exeiop->sections[secndx]->isExecutable() << 0 ) ;

		bool is_relro=is_in_relro_segment(secndx);
		scoops_detected++;
		auto newscoop=firp->addNewDataScoop( name, startaddr, endaddr, NULL, permissions, is_relro, the_contents, max_base_id++ );
		(void)newscoop; // just give it to the IR

		cout << "Allocated new scoop for section " << name
		     << "(" << hex << startaddr->getVirtualOffset() << "-"
		     << hex << endaddr->getVirtualOffset() << ")"
		     << " perms=" << permissions << " relro=" << boolalpha << is_relro << endl;

	}
	for(auto extra_scoop : extra_scoops)
	{
		const auto start_vo        = extra_scoop.first;                                           // start and end offsets in this file 
		const auto end_vo          = extra_scoop.second;
		auto startaddr             = firp->addNewAddress(firp->getFile()->getBaseID(), start_vo); // start and end address 
		auto endaddr               = firp->addNewAddress(firp->getFile()->getBaseID(), end_vo);
		const auto sec             = exeiop->sections.findByAddress(start_vo);                    // section that contains the data 
		assert(sec);
		const auto sec_data        = sec->get_data();                                             // data
		const auto scoop_start_ptr = sec_data+(start_vo-sec->get_address());                      // relevant start of data
		const auto the_contents    = string(scoop_start_ptr, end_vo-start_vo+1); 
		const auto name            = string("user_added_")+to_hex_string(start_vo);               // name of new segment
		const auto permissions     =                                                              // permissions and relro bit.
			( sec->isReadable()   << 2 ) |  
			( sec->isWriteable()  << 1 ) | 
			( sec->isExecutable() << 0 ) ;
		const auto is_relro       = false;

		// finally, create the new scoop
		firp->addNewDataScoop( name, startaddr, endaddr, NULL, permissions, is_relro, the_contents, max_base_id++ );
		
	}

}

void PopulateCFG::detect_scoops_in_code(FileIR_t *firp)
{
	// make sure preds are up to date for this
	calc_preds(firp);

	// data for this function
	auto already_scoopified=map<VirtualOffset_t,DataScoop_t*>();

	const auto is_arm64       = firp->getArchitecture()->getMachineType() == admtAarch64;
	const auto is_arm32       = firp->getArchitecture()->getMachineType() == admtArm32;
	const auto is_arm_variant = is_arm32 || is_arm64;

	// only valid for arm64
	if(!is_arm_variant) return;

	// check each insn for an ldr with a pcrel operand.
	for(auto insn : firp->getInstructions())
	{
		// look for ldr's with a pcrel operand
		const auto d               = DecodedInstruction_t::factory(insn);
		const auto mnemonic        = d->getMnemonic();
		const auto is_ldrd_variant  = mnemonic.substr(0,4) == "ldrd";
		const auto is_ldr_variant  = mnemonic.substr(0,3) == "ldr";
		const auto is_vldr_variant = mnemonic.substr(0,4) == "vldr";
		const auto is_relevant_ldr = is_ldr_variant || is_vldr_variant || is_ldrd_variant;
		if(!is_relevant_ldr) continue;	 


		// extract op0
		const auto op0             = d->getOperand(0);

		// the address we detect as referenced by this instruction.
		auto referenced_address = VirtualOffset_t(0);
		auto do_unpin       = is_arm32;

		if(is_ldrd_variant && !d->getOperand(2)->isPcrel())
		{
			const auto mem_op = d->getOperand(2);
			if( mem_op->hasIndexRegister() ) 	continue;
			if( mem_op->getMemoryDisplacement() != 0 ) continue;

			const auto mem_str         = mem_op->getString();
			const auto end_of_base_reg = mem_str.find(" ");
			const auto find_reg        = mem_str.substr(0,end_of_base_reg);

			// find the instruction that sets the base reg.
			auto add_pc_insn = (Instruction_t*)nullptr;
			if(!backup_until( string()+"add.* "+find_reg+", pc, #",  /* look for this pattern. */
						add_pc_insn,        /* strong instruction in add_pc_insn */
						insn,               /* insn I10 */
						"^"+find_reg+"$"    /* stop if find_reg set */
						))
			{
				continue;
			}

			const auto add_pc_insn_d = DecodedInstruction_t::factory(add_pc_insn);

			// record to unpin something.
			referenced_address = add_pc_insn_d -> getOperand(2)->getConstant() + (is_arm32 ? add_pc_insn->getAddress()->getVirtualOffset() + 8 : 0); 
			do_unpin           = false;

		}
		else
		{

			// capstone reports ldrd instructions as having 2 "dest" operands.
			// so we skip to the 3rd operand to get the memory op.  
			// todo:  fix libirdb-core to fix this and skip the odd operand.
			// todo:  report to capstone that they are broken.
			const auto mem_op = d->getOperand(1);
			if( !mem_op->isPcrel()) continue;

			// if there is an indexing operation, skip this instruction.
			if( mem_op->hasIndexRegister()) continue;

			// sanity check that it's a memory operation, and extract fields
			assert(mem_op->isMemory());

			
			referenced_address = mem_op->getMemoryDisplacement() + (is_arm32 ? insn->getAddress()->getVirtualOffset() + 8 : 0); 
		}

		// no address found.
		if(referenced_address == 0) continue;

		const auto name           = "data_in_text_"+to_hex_string(referenced_address);
		const auto op0_str            = op0->getString();

		// if op0 is the PC, the instruction is some switch dispatch that we have to detect in more depth.  skip here.  see check_arm32_switch...
		if(is_arm32 && op0_str == "pc" ) continue;

		const auto referenced_size    =  // could use API call?
			is_ldrd_variant             ? 8  : // special case for load int reg pair.
			is_arm64 && op0_str[0]=='w' ? 4  : // arm64 regs
			is_arm64 && op0_str[0]=='x' ? 8  : 
			is_arm64 && op0_str[0]=='s' ? 4  : 
			is_arm64 && op0_str[0]=='d' ? 8  : 
			is_arm64 && op0_str[0]=='q' ? 16 : 
			is_arm32 && op0_str[0]=='s' ? 4  : // arm32 regs
			is_arm32 && op0_str[0]=='r' ? 4  : // arm32 regs
			is_arm32 && op0_str   =="sb"? 4  : // special arm32 regs
			is_arm32 && op0_str   =="sl"? 4  : 
			is_arm32 && op0_str   =="lr"? 4  : 
			is_arm32 && op0_str   =="fp"? 4  : 
			is_arm32 && op0_str   =="sp"? 4  : 
			is_arm32 && op0_str   =="ip"? 4  : 
			is_arm32 && op0_str[0]=='d' ? 8  : // arm32 regs
			throw domain_error("Cannot decode instruction size");
			;

		// check if we've seen this address already
		const auto already_seen_it = already_scoopified.find(referenced_address);
		if(already_seen_it == end(already_scoopified)) 
		{

			// find section and sanity check.
			const auto sec=exeiop->sections.findByAddress(referenced_address);
			if(sec==nullptr) continue; 

			// only trying to do this for executable chunks, other code deals with
			// scoops not in the .text section.
			if(!sec->isExecutable()) continue;

			const auto new_scoop_addr = do_unpin ?  VirtualOffset_t(0u) : referenced_address;
			const auto sec_data       = sec->get_data();
			const auto sec_start      = sec->get_address();
			const auto the_contents   = string(&sec_data[referenced_address-sec_start],referenced_size);
			const auto fileid         = firp->getFile()->getBaseID();
			auto new_start_addr       = firp->addNewAddress(fileid,new_scoop_addr);
			auto new_end_addr         = firp->addNewAddress(fileid,new_scoop_addr+referenced_size-1);
			const auto permissions    = 0x4; /* R-- */
			const auto is_relro       = false;

			// create the new scoop
			already_scoopified[referenced_address] = firp->addNewDataScoop(name, new_start_addr, 
					new_end_addr, NULL, permissions, is_relro, the_contents);
		}

		auto newscoop = already_scoopified[referenced_address] ;
		assert(newscoop);
		const auto start_addr = newscoop -> getStart();
		const auto end_addr   = newscoop -> getEnd();

		if(do_unpin)
		{
			const auto new_addend = -uint32_t(insn->getAddress()->getVirtualOffset());
			(void)firp->addNewRelocation(insn,0,"pcrel",newscoop, new_addend);
		}

		cout << "Allocated data in text segment " << name << "=(" << start_addr->getVirtualOffset() << "-"
		     << end_addr->getVirtualOffset() << ")" 
		     << " for " << d->getDisassembly() << "@" << hex << insn->getAddress()->getVirtualOffset() << endl;
	}
}

void PopulateCFG::ctor_detection(FileIR_t *firp)
{
	const auto create_text_scoop=[&](const string& name, const VirtualOffset_t& start_vo, const VirtualOffset_t& end_vo)
		{
			auto startaddr             = firp->addNewAddress(firp->getFile()->getBaseID(), start_vo); // start and end address 
			auto endaddr               = firp->addNewAddress(firp->getFile()->getBaseID(), end_vo);
			const auto sec             = exeiop->sections.findByAddress(start_vo);                    // section that contains the data 
			assert(sec);
			const auto sec_data        = sec->get_data();                                             // data
			const auto scoop_start_ptr = sec_data+(start_vo-sec->get_address());                      // relevant start of data
			const auto the_contents    = string(scoop_start_ptr, end_vo-start_vo+1); 
			const auto permissions     =                                                              // permissions and relro bit.
				( sec->isReadable()   << 2 ) |  
				( sec->isWriteable()  << 1 ) | 
				( sec->isExecutable() << 0 ) ;
			const auto is_relro       = false;

			// finally, create the new scoop
			firp->addNewDataScoop( name, startaddr, endaddr, NULL, permissions, is_relro, the_contents);
			cout << "Added ctor/dtor scoop called " << name << " at " << hex << start_vo << "-" << end_vo << endl;
		};	

	const auto gnustyle = [&]() -> bool
	{
		assert(firp);
		if(do_ctor_detection == cdt_NO) return false;

		const auto is_pe = firp->getArchitecture()->getFileType() == adftPE ;
		if(do_ctor_detection == cdt_PE32AUTO && !is_pe) return false;

		// is either a PE file and we are in auto detect mode
		// or ctor_detection is yes.
		assert(is_pe || do_ctor_detection == cdt_YES);

		auto find_ctor_start=[&](const exeio_section_t* sec, const VirtualOffset_t end_of_ctor) -> VirtualOffset_t
			{
				// values needed later for various things
				const auto ptrsize   = firp->getArchitectureBitWidth() / 8 ;
				const auto sec_data  = sec->get_data();
				const auto sec_start = sec->get_address();

				// check for a null terminator at the stated end of table
				const auto null_term_addr=end_of_ctor-ptrsize;
				const auto null_term_value =
					ptrsize == 8 ?  *(uint64_t*)(sec_data+null_term_addr-sec_start) :
					ptrsize == 4 ?  *(uint32_t*)(sec_data+null_term_addr-sec_start) :
					throw invalid_argument("Unknown ptrsize");

				// not found, return this isn't sane.
				if(null_term_value!=0) return 0;

				// now scan the table in reverse order for 1) valid entries, or 2) a -1 terminator.
				auto next_addr=null_term_addr-ptrsize;
				while(true)
				{
					// check for flowing
					if(next_addr<sec_start) return 0;

					// get the table entry
					const auto ctor_entry_value =
						ptrsize == 8 ?  *(uint64_t*)(sec_data+next_addr-sec_start) :
						ptrsize == 4 ?  *(uint32_t*)(sec_data+next_addr-sec_start) :
						throw invalid_argument("Unknown ptrsize");

					// check for the -1 terminator
					if(ptrsize == 8 && (int64_t)ctor_entry_value==int64_t(-1)) return next_addr;
					if(ptrsize == 4 && (int32_t)ctor_entry_value==int32_t(-1)) return next_addr;

					// check if the table entry isn't a valid address
					const auto is_before_start = ctor_entry_value <  sec->get_address() ;
					const auto is_after_end    = ctor_entry_value > (sec->get_address()+sec->get_size());
					if(is_before_start || is_after_end) return 0;

					next_addr -= ptrsize;
				}
			};


		const auto text_sec = exeiop->sections[".text"];
		if(text_sec == nullptr) return false;
		const auto text_end_addr = text_sec->get_address()+text_sec->get_size();
		const auto dtor_end      = text_end_addr-1;
		const auto dtor_start    = find_ctor_start(text_sec,text_end_addr);
		if(dtor_start==0) return false;
		create_text_scoop(".dtor", dtor_start, dtor_end);

		const auto ctor_end   = dtor_start-1;
		const auto ctor_start = find_ctor_start(text_sec,dtor_start);
		if(ctor_start==0) return false;
		create_text_scoop(".ctor", ctor_start, ctor_end);
		return true;
	};

#if 0
	const auto winstyle = [&]() -> bool
	{
		const auto  reloc_sec = exeiop->sections[".reloc"];
		if(reloc_sec == nullptr) return false;
		const auto reloc_data = string(reloc_sec->get_data(), reloc_sec->get_size());
		const auto peblissp = static_cast<pe_bliss::pe_base*>(exeiop->get_pebliss());
		if(peblissp == nullptr) return false;
		const auto pe_image_base = peblissp->get_image_base_64();

		// for each block in the reloc section
		using reloc_block_header_t = 
			struct 
			{
				uint32_t page;
				uint32_t block_size;
			};
		using reloc_block_t = 
			struct
			{
				uint16_t offset:12; 	// lower bits 
				uint16_t type:4;	// higher bits than offset?
			};
		static_assert(sizeof(reloc_block_header_t)==8, "size of reloc_block_header is wrong");
		static_assert(sizeof(reloc_block_t)==2, "size of reloc_block is wrong");
		// start at block 0
		auto current_offset = VirtualOffset_t(0);
		while(true)
		{

			// stop if we fall off the end of the section.
			if(current_offset + sizeof(reloc_block_header_t) > static_cast<size_t>(reloc_sec->get_size()))
				break;

			// read the header
			auto header = reloc_block_header_t();
			memcpy(&header, &reloc_data.c_str()[current_offset], sizeof(header));

			// start counting the length after we've read the header.
			auto current_length = size_t(sizeof(header));

			// for each entry in the block
			while(true)
			{
				// stop if we exceed the block
				if(current_length + sizeof(reloc_block_t) > header.block_size)
					break;
				// stop if we exceed the section
				if(current_offset + current_length + sizeof(reloc_block_t) > static_cast<size_t>(reloc_sec->get_size()))
					break;
				
				// read the block
				auto block  = reloc_block_t();
				memcpy(&block, &reloc_data.c_str()[current_offset+current_length], sizeof(block));
				current_length += sizeof(block);

				// for now, just log
				const auto rva = pe_image_base + header.page + block.offset;
				cout << "reloc rva = " << hex << rva << " type = " << block.type << '\n';

				switch(block.type)
				{
					case pe_bliss::pe_win::image_rel_based_absolute: /* ignored */
						break;
					case pe_bliss::pe_win::image_rel_based_dir64: /* dir64 */
					{
						const auto relocd_sec = exeiop->sections.findByAddress(rva);
						if(!relocd_sec) throw runtime_error("Unexpected relocation address?");

						const auto relocd_sec_name = relocd_sec->get_name();
						if(relocd_sec_name != ".text") // no need to pin anything besides the text seg.
							continue;   // try next block

						const auto new_name = "winstyle_scoop_"+to_string(rva);
						create_text_scoop(new_name, rva, rva+8);

						break;
					}
					default:
						throw runtime_error("Cannot handle PE relocation.");

				}
			}
			// update to the next block
			current_offset += current_length;
		}
		return true;


	};


	const auto scoopify_data_dir_sections = [&]() 
	{
		const auto peblissp = static_cast<pe_bliss::pe_base*>(exeiop->get_pebliss());

		for(auto dir=0u; dir<15; dir++)
		{
			if(!peblissp->directory_exists(dir))
				continue;

			const auto dir_rva  = peblissp->get_directory_rva (dir);
			const auto dir_size = peblissp->get_directory_size(dir);

			cout << "Directory " << dec << dir << " is at " << hex << dir_rva << " - " << dir_rva+dir_size << '\n';

			
		}
	};
#endif


	
//	scoopify_data_dir_sections();

	if(gnustyle()) return;
//	if(winstyle()) return;

	cerr << "Caution!  Could not find any reloc handling for ctor/dtor and/or plts." << endl;

}

void PopulateCFG::fill_in_landing_pads(FileIR_t *firp)
{
	const auto eh_frame_rep_ptr = split_eh_frame_t::factory(firp);
	if(getenv("EHIR_VERBOSE"))
		eh_frame_rep_ptr->print();
	cout<<"Completed eh-frame parsing"<<endl;

	map<Function_t*,set<Instruction_t*> > insns_to_add_to_funcs;

	for(const auto t : firp->getInstructions())
	{
		if(t->getFunction()==NULL)
			continue;
		auto lp=eh_frame_rep_ptr->find_lp(t);
		if(lp && lp->getFunction()==NULL)
			insns_to_add_to_funcs[t->getFunction()].insert(lp);
	};


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
			if(insn->getFunction()!=NULL)
				continue;

			auto lp=eh_frame_rep_ptr->find_lp(insn);
			if(lp && lp->getFunction()==NULL)
				insns.insert(lp);

			insn->setFunction(func);
			cout<<"	Adding "<<insn->getBaseID()<<":"<<insn->getDisassembly()<<"@"<<hex<<insn->getAddress()->getVirtualOffset()<<dec<<endl;
			insn_count++;
			

			auto target=insn->getTarget();
			auto fallthru=insn->getFallthrough();

			if(target) insns.insert(target);
			if(fallthru) insns.insert(fallthru);
		}
		cout<<"Found LP outside of function "<<func->getName()<<" added "<<insn_count<<" instructions"<<endl;
	};
	
}

int PopulateCFG::parseArgs(const vector<string> step_args)
{   
    
    for (auto i = 0u; i < step_args.size(); ++i)
    {
            if (step_args[i]=="--fix-landing-pads")
            {
                    fix_landing_pads = true;
            }
            else if (step_args[i]=="--no-fix-landing-pads")
            {
                    fix_landing_pads = false;
            }
            else if (step_args[i]=="--extra-scoop")
	    {
		    i++;
		    const auto extra_scoops_str=step_args[i];
		    stringstream ss(extra_scoops_str);
		    auto start_addr = VirtualOffset_t(0);
		    auto end_addr   = VirtualOffset_t(0);
		    auto c          = uint8_t(0);
		    ss>>hex>>start_addr>>c>>end_addr;
		    assert(c=='-');
		    extra_scoops.insert({start_addr,end_addr});
		    cout << "Recognizing request to add " << hex << start_addr << "-" << end_addr << " as a scoop."<<endl;
	    }
            else if (step_args[i]=="--do-ctor-detection")
	    {
		    do_ctor_detection=cdt_YES;
	    }
            else if (step_args[i]=="--no-ctor-detection")
	    {
		    do_ctor_detection=cdt_NO;
	    }
    }

    cout<<"fix_landing_pads="<<fix_landing_pads<<endl;
    
    return 0;
}

void PopulateCFG::rename_start(FileIR_t *firp)
{
	for(auto f : firp->getFunctions())
	{
		const auto entry_point_insn = f->getEntryPoint();
		if(!entry_point_insn) continue;

		const auto entry_point_vo = entry_point_insn->getAddress()->getVirtualOffset();
		if(entry_point_vo==exeiop->get_entry())
			f->setName("_start");
	}
}

int PopulateCFG::executeStep()
{
	variant_id=getVariantID();
	auto irdb_objects=getIRDBObjects();
	try 
	{
		const auto pqxx_interface = irdb_objects->getDBInterface();
		// now set the DB interface for THIS PLUGIN LIBRARY -- VERY IMPORTANT
		BaseObj_t::setInterface(pqxx_interface);	

		const auto variant = irdb_objects->addVariant(variant_id);
		for(File_t* file : variant->getFiles())
		{
			const auto firp = irdb_objects->addFileIR(variant_id, file->getBaseID());
			assert(firp);
                        cout<<"Filling in cfg for "<<firp->getFile()->getURL()<<endl;

			exeiop.reset(new exeio());
			exeiop->load(string("a.ncexe"));

			rename_start(firp);
			fill_in_cfg(firp);
			fill_in_scoops(firp);
			detect_scoops_in_code(firp);
			ctor_detection(firp);

			if (fix_landing_pads)
			{
				fill_in_landing_pads(firp);
			}
		}
	}
	catch (const DatabaseError_t &pnide)
	{
		cerr<<"Unexpected database error: "<<pnide<<endl;
		return -1;
        }
	catch(...)
	{
		cerr<<"Unexpected error"<<endl;
		return -1;
	}

        cout<<"#ATTRIBUTE targets_set="<<targets_set<<endl;
        cout<<"#ATTRIBUTE fallthroughs_set="<<fallthroughs_set<<endl;
        cout<<"#ATTRIBUTE scoops_detected="<<scoops_detected<<endl;

	if(getenv("SELF_VALIDATE"))
	{
		assert(targets_set > 10);
		assert(fallthroughs_set > 100);
		assert(scoops_detected > 5 );
	}

	return 0;
}


extern "C"
shared_ptr<TransformStep_t> getTransformStep(void)
{
	const shared_ptr<TransformStep_t> the_step(new PopulateCFG());
	return the_step;
}
