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

using namespace std;
using namespace EXEIO;
using namespace IRDB_SDK;

void PopulateCFG::populate_instruction_map
	(
		map< pair<DatabaseID_t,VirtualOffset_t>, Instruction_t*> &insnMap,
		FileIR_t *firp
	)
{
	/* start from scratch each time */
	insnMap.clear();


	/* for each instruction in the IR */
	for(auto insn : firp->getInstructions())
	{
		auto fileID=insn->getAddress()->getFileID();
		auto vo=insn->getAddress()->getVirtualOffset();

		auto p=pair<DatabaseID_t,VirtualOffset_t>(fileID,vo);

		assert(insnMap[p]==NULL);
		insnMap[p]=insn;
	}

}

void PopulateCFG::set_fallthrough
	(
	map< pair<DatabaseID_t,VirtualOffset_t>, Instruction_t*> &insnMap,
	DecodedInstruction_t *disasm, Instruction_t *insn, FileIR_t *firp
	)
{
	assert(disasm);
	assert(insn);

	if(insn->getFallthrough())
		return;
	
	// check for branches with targets 
	if(
		(disasm->isUnconditionalBranch() ) ||	// it is a unconditional branch 
		(disasm->isReturn())			// or a return
	  )
	{
		// this is a branch with no fallthrough instruction
		return;
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


void PopulateCFG::set_target
	(
	map< pair<DatabaseID_t,VirtualOffset_t>, Instruction_t*> &insnMap,
	DecodedInstruction_t *disasm, Instruction_t *insn, FileIR_t *firp
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
		
        		VirtualOffset_t first=elfiop->sections[secndx]->get_address();
        		VirtualOffset_t second=elfiop->sections[secndx]->get_address()+elfiop->sections[secndx]->get_size();

			/* is the missed instruction in this section */
			if(first<=missed_address && missed_address<second)
			{
				const char* data=elfiop->sections[secndx]->get_data();
				// second=data?
				VirtualOffset_t offset_into_section=missed_address-elfiop->sections[secndx]->get_address();
	
				/* disassemble the instruction */
				auto disasm_p=DecodedInstruction_t::factory(missed_address, (void*)&data[offset_into_section], elfiop->sections[secndx]->get_size()-offset_into_section );
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
				/*
				auto newaddr=new AddressID_t();
				assert(newaddr);
				newaddr->setVirtualOffset(missed_address);
				newaddr->setFileID(missed_fileid);
				firp->getAddresses().insert(newaddr);
				*/
				auto newaddr=firp->addNewAddress(missed_fileid,missed_address);

				/* create a new instruction */
				/*
				auto newinsn=new Instruction_t();
				assert(newinsn);
				newinsn->setAddress(newaddr);
				newinsn->setDataBits(newinsnbits);
				newinsn->setComment(disasm.getDisassembly()+string(" from fill_in_cfg "));
				firp->getInstructions().insert(newinsn);
				newinsn->setAddress(newaddr);
				*/
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

		map< pair<DatabaseID_t,VirtualOffset_t>, Instruction_t*> insnMap;
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
	ELFIO::elfio *real_elfiop = reinterpret_cast<ELFIO::elfio*>(elfiop->get_elfio()); 
	if(!real_elfiop)
		return false;

	int segnum = real_elfiop->segments.size();

	VirtualOffset_t sec_start=(VirtualOffset_t)(elfiop->sections[secndx]->get_address());
	VirtualOffset_t sec_end=(VirtualOffset_t)(elfiop->sections[secndx]->get_address() + elfiop->sections[secndx]->get_size() - 1 );

	/* look through each section */
	for (int segndx=1; segndx<segnum; segndx++)
	{
		ELFIO::Elf_Word type=real_elfiop->segments[segndx]->get_type();
#ifndef PT_GNU_RELRO
#define PT_GNU_RELRO    0x6474e552      /* Read-only after relocation */
#endif

		if(type==PT_GNU_RELRO)
		{
			VirtualOffset_t seg_start=(VirtualOffset_t)(real_elfiop->segments[segndx]->get_virtual_address());
			VirtualOffset_t seg_end=(VirtualOffset_t)(real_elfiop->segments[segndx]->get_virtual_address() + real_elfiop->segments[segndx]->get_memory_size() - 1 );

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
		/*
		auto startaddr=new AddressID_t();
		assert(startaddr);
		startaddr->setVirtualOffset( elfiop->sections[secndx]->get_address());
		startaddr->setFileID(firp->getFile()->getBaseID());
		firp->getAddresses().insert(startaddr);
		*/
		auto startaddr=firp->addNewAddress(firp->getFile()->getBaseID(), elfiop->sections[secndx]->get_address());

		/* end */
		/*
		auto endaddr=new AddressID_t();
		assert(endaddr);
		endaddr->setVirtualOffset( elfiop->sections[secndx]->get_address() + elfiop->sections[secndx]->get_size()-1);
		endaddr->setFileID(firp->getFile()->getBaseID());
		firp->getAddresses().insert(endaddr);
		*/
		auto endaddr=firp->addNewAddress(firp->getFile()->getBaseID(), elfiop->sections[secndx]->get_address() + elfiop->sections[secndx]->get_size()-1);


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
		scoops_detected++;
		/*
		DataScoop_t *newscoop=new DataScoop_t(max_base_id++, name, startaddr, endaddr, NULL, permissions, is_relro, the_contents);
		assert(newscoop);
		firp->getDataScoops().insert(newscoop);
		*/
		auto newscoop=firp->addNewDataScoop( name, startaddr, endaddr, NULL, permissions, is_relro, the_contents, max_base_id++ );
		(void)newscoop; // just give it to the IR

		cout<<"Allocated new scoop for section "<<name
		    <<"("<<hex<<startaddr->getVirtualOffset()<<"-"
		    <<hex<<endaddr->getVirtualOffset()<<")"
		    <<" perms="<<permissions<<" relro="<<boolalpha<<is_relro<<endl;

	}

}

void PopulateCFG::detect_scoops_in_code(FileIR_t *firp)
{
	// data for this function
	auto already_scoopified=set<VirtualOffset_t>();

	// only valid for arm64
	if(firp->getArchitecture()->getMachineType() != admtAarch64) return;

	// check each insn for an ldr with a pcrel operand.
	for(auto insn : firp->getInstructions())
	{
		// look for ldr's with a pcrel operand
		const auto d=DecodedInstruction_t::factory(insn);
		if(d->getMnemonic()!="ldr") continue;	 // only valid on arm.
		const auto op0=d->getOperand(0);
		const auto op1=d->getOperand(1);
	       	if( !op1->isPcrel()) continue;

		// sanity check that it's a memory operation, and extract fields
		assert(op1->isMemory());
		const auto referenced_address=op1->getMemoryDisplacement();
		const auto op0_str=op0->getString();
		const auto referenced_size=  // could use API call?
			op0_str[0]=='w' ? 4  : 
			op0_str[0]=='x' ? 8  : 
			op0_str[0]=='s' ? 4  : 
			op0_str[0]=='d' ? 8  : 
			op0_str[0]=='q' ? 16 : 
			throw domain_error("Cannot decode instruction size");
			;

		// check if we've seen this address already
		const auto already_seen_it=already_scoopified.find(referenced_address);
		if(already_seen_it!=end(already_scoopified)) continue;

		// not seen, add it
		already_scoopified.insert(referenced_address);


		// find section and sanity check.
		const auto sec=elfiop->sections.findByAddress(referenced_address);
		if(sec==nullptr) continue;

		// only trying to do this for executable chunks, other code deals with
		// scoops not in the .text section.
		if(!sec->isExecutable()) continue;

		const auto sec_data=sec->get_data();
		const auto sec_start=sec->get_address();
		const auto the_contents=string(&sec_data[referenced_address-sec_start],referenced_size);
		const auto fileid=firp->getFile()->getBaseID();


		/*
		auto start_addr=new AddressID_t(BaseObj_t::NOT_IN_DATABASE,fileid,referenced_address);
		assert(start_addr);
		firp->getAddresses().insert(start_addr);

		auto end_addr=new AddressID_t(BaseObj_t::NOT_IN_DATABASE,fileid,referenced_address+referenced_size-1);
		assert(end_addr);
		firp->getAddresses().insert(end_addr);
		*/
		auto start_addr=firp->addNewAddress(fileid,referenced_address);
		auto end_addr  =firp->addNewAddress(fileid,referenced_address+referenced_size-1);

		const auto name="data_in_text_"+to_string(referenced_address);
		const auto permissions=0x4; /* R-- */
		const auto is_relro=false;
		/*
		auto newscoop=new DataScoop_t(BaseObj_t::NOT_IN_DATABASE, name, start_addr, end_addr, NULL, permissions, is_relro, the_contents);
		firp->getDataScoops().insert(newscoop);
		*/
		auto newscoop=firp->addNewDataScoop(name, start_addr, end_addr, NULL, permissions, is_relro, the_contents);
		(void)newscoop;

		cout<< "Allocated data in text segment "<<name<<"=("<<start_addr->getVirtualOffset()<<"-"
		    << end_addr->getVirtualOffset()<<")"<<endl;
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

	// for_each(firp->getInstructions().begin(), firp->getInstructions().end(), [&](Instruction_t* t)
	for(const auto t : firp->getInstructions())
	{
		if(t->getFunction()==NULL)
			continue;
		auto lp=eh_frame_rep_ptr->find_lp(t);
		if(lp && lp->getFunction()==NULL)
			insns_to_add_to_funcs[t->getFunction()].insert(lp);
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
    if(step_args.size()<1)
    {
            cerr<<"Usage: <id> [--fix-landing-pads | --no-fix-landing-pads]"<<endl;
            return -1;
    }

    variant_id = stoi(step_args[0]);
    
    for (unsigned int i = 1; i < step_args.size(); ++i)
    {
            if (step_args[i]=="--fix-landing-pads")
            {
                    fix_landing_pads = true;
            }
            else if (step_args[i]=="--no-fix-landing-pads")
            {
                    fix_landing_pads = false;
            }
    }

    cout<<"fix_landing_pads="<<fix_landing_pads<<endl;
    
    return 0;
}

int PopulateCFG::executeStep(IRDBObjects_t *const irdb_objects)
{
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

			/* get the OID of the file */
			const int elfoid=firp->getFile()->getELFOID();

			pqxx::largeobject lo(elfoid);
                	lo.to_file(pqxx_interface->getTransaction(),"readeh_tmp_file.exe");

			elfiop.reset(new exeio());
			elfiop->load(string("readeh_tmp_file.exe"));

			fill_in_cfg(firp);
			fill_in_scoops(firp);
			detect_scoops_in_code(firp);

			if (fix_landing_pads)
			{
				fill_in_landing_pads(firp);
			}
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
