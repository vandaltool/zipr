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
#include <irdb-cfg>
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <elf.h>
#include <set>
#include <exeio.h>

#include "fill_in_indtargs.hpp"


using namespace std;
using namespace EXEIO;
using namespace IRDB_SDK;

// macros
#define ALLOF(a) begin(a),end(a)


// externs
extern void read_ehframe(FileIR_t* firp, EXEIO::exeio* );

class FixCalls_t : public TransformStep_t
{
	private:
		const bool opt_fix_no_func_target = false;
		const bool opt_fix_no_target      = false;


		class Range_t
		{
			public:
				Range_t(VirtualOffset_t p_s, VirtualOffset_t p_e) : m_start(p_s), m_end(p_e) { }
				Range_t() : m_start(0), m_end(0) { }

				virtual VirtualOffset_t getStart() const { return m_start; }
				virtual VirtualOffset_t getEnd() const { return m_end; }
				virtual void setStart(VirtualOffset_t s) { m_start=s; }
				virtual void setEnd(VirtualOffset_t e) { m_end=e; }

			protected:

				VirtualOffset_t m_start, m_end;
		};

		struct Range_tCompare
		{
			bool operator() (const Range_t &first, const Range_t &second) const
			{
				return first.getEnd() < second.getStart();
			}
		};

		using Rangeset_t = set<Range_t, Range_tCompare>;

		Rangeset_t eh_frame_ranges;
		size_t no_target_insn=0;
		size_t no_fallthrough_insn=0;
		size_t target_not_in_function=0;
		size_t call_to_not_entry=0;
		size_t thunk_check=0;
		size_t found_pattern=0;
		size_t in_ehframe=0;
		size_t no_fix_for_ib=0;
		size_t no_fix_for_safefn=0;
		size_t other_fixes=0;
		size_t fixed_calls=0;
		size_t not_fixed_calls=0;
		size_t not_calls=0;

		bool opt_fix_icalls = false;
		bool opt_fix_safefn = true;

		bool check_entry(bool &found, ControlFlowGraph_t* cfg)
		{

			auto entry=cfg->getEntry();
			found=false;

			for(auto insn : entry->getInstructions())
			{
				auto disasmp = DecodedInstruction_t::factory(insn);
				auto &disasm = *disasmp;
				if(disasm.setsStackPointer()) {
					return false;
				} else {
					if(getenv("VERBOSE_FIX_CALLS"))
					{
						VirtualOffset_t addr = 0;
						if (insn->getAddress())
							addr = insn->getAddress()->getVirtualOffset();
						cout << "check_entry: does not set stack pointer?" << " address="
						     << hex << addr << ": " << insn->getDisassembly() << endl;
					}
				}

				if(strstr(disasm.getDisassembly().c_str(), "[esp]"))
				{
					found=true;
					if(getenv("VERBOSE_FIX_CALLS"))
					{
						VirtualOffset_t addr = 0;
						if (insn->getAddress())
							addr = insn->getAddress()->getVirtualOffset();
						cout << "Needs fix (check_entry): [esp]" << " address="
						     << hex << addr << ": " << insn->getDisassembly() << endl;
					}
					return true;
				}
			}

			return false;
		}

		using ControlFlowGraphMap_t = map<Function_t*, shared_ptr<ControlFlowGraph_t> >;
		ControlFlowGraphMap_t cfg_optimizer;

		bool call_needs_fix(Instruction_t* insn)
		{

			for(auto reloc : insn->getRelocations())
			{
				if(string("safefr") == reloc->getType())
					return false;
			}

			const auto target=insn->getTarget();
			const auto fallthru=insn->getFallthrough();

		// 	string pattern;

		// this used to work because fill_in_indirects would mark IBTs 
		// while reading the ehframe, which perfectly corresponds to when
		// we need to fix calls due to eh_frame.  However, now STARS also marks
		// return points as IBTs, so we need to re-parse the ehframe and use that instead.

			/* no fallthrough instruction, something is odd here */
			if(!fallthru)
			{
				if(getenv("VERBOSE_FIX_CALLS"))
				{
					VirtualOffset_t addr = 0;
					if (insn->getAddress())
						addr = insn->getAddress()->getVirtualOffset();
					cout << "Needs fix: No fallthrough" << " address="
					     << hex << addr << ": " << insn->getDisassembly() << endl;
				}
				no_fallthrough_insn++;
				return true;
			}

			const auto addr=fallthru->getAddress()->getVirtualOffset();
			const auto rangeiter=eh_frame_ranges.find(Range_t(addr,addr));
			if(rangeiter != eh_frame_ranges.end())	// found an eh_frame addr entry for this call
			{
				in_ehframe++;
				return true;
			}

			if (!opt_fix_icalls && insn->getIBTargets() && insn->getIBTargets()->size() > 0) 
			{
				/* do not fix indirect calls */
				no_fix_for_ib++;
				return false;
			}

			/* if the target isn't in the IR */
			if(!target)
			{
				/* call 0's aren't to real locations */
				const auto disasm=DecodedInstruction_t::factory(insn);
				if(disasm->getOperand(0)->isConstant() && disasm->getAddress()==0)
				{
					return false;
				}
				no_target_insn++;

				if(getenv("VERBOSE_FIX_CALLS"))
				{
					VirtualOffset_t addr = 0;
					if (insn->getAddress())
						addr = insn->getAddress()->getVirtualOffset();
					cout << "Needs fix: No target instruction" << " address="
					     << hex << addr << ": " << insn->getDisassembly() << endl;
				}
				// then we might need to fix it 
				// but typically, we don't fix it because it's not really a valid isntruction. 
				return opt_fix_no_target;
			}


			/* 
			 * if the location after the call is marked as an IBT, then 
			 * this location might be used for walking the stack 
			 */


			const auto func=target->getFunction();

			/* if there's no function for this instruction */
			if(!func)
			{
				if(getenv("VERBOSE_FIX_CALLS"))
				{
					VirtualOffset_t addr = 0;
					if (insn->getAddress())
						addr = insn->getAddress()->getVirtualOffset();
					cout<<"Needs fix: Target not in a function"<< " address="
					    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
				}
				target_not_in_function++;
				/* we need to fix it */
				return opt_fix_no_func_target;
			}



			const auto is_found_it=cfg_optimizer.find(func);
			const auto is_found=(is_found_it!=end(cfg_optimizer));

			if(!is_found)
				/* build a cfg for this function */
				cfg_optimizer[func]=shared_ptr<ControlFlowGraph_t>(move(ControlFlowGraph_t::factory(func)));

			auto cfg=cfg_optimizer[func].get();
			


			assert(cfg->getEntry());
			
			/* if the call instruction isn't to a function entry point */
			if(cfg->getEntry()->getInstructions()[0]!=target)
			{
				call_to_not_entry++;
				/* then we need to fix it */
				return true;
			}


			/* check the entry block for thunks, etc. */
			auto found=false;
			bool ret=check_entry(found,cfg);
			// delete cfg;
			if(found)
			{
				if(ret)
				{
					if(getenv("VERBOSE_FIX_CALLS"))
					{
						VirtualOffset_t addr = 0;
						if (insn->getAddress())
							addr = insn->getAddress()->getVirtualOffset();
						cout<<"Needs fix: (via check_entry) Thunk detected"<< " address="
						    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
					}
					thunk_check++;
				}

				return ret;
			}


			/* otherwise, we think it's safe */
			return false;

		}

		/*
		 * - adjust_esp_offset - take newbits, and determine if it has an esp+K type offset in a memory address.
		 * if so, adjust k by offset, and return the new string.
		 */
		string adjust_esp_offset(string newbits, int offset)
		{

			/*
			 * call has opcodes of:
			 *              E8 cw   call near, relative, displacement relative to next instruction
			 *              E8 cd   call near, relative, displacement relative to next instruction
			 *              FF /2   call near, absolute indirect, address given in r/m16
			 *              FF /2   call near, absolute indirect, address given in r/m32
			 *              9a cd   call far, absolute, address given in operand
			 *              9a cp   call far, absolute, address given in operand
			 *              FF /3   call far, absolute indirect, address given in m16:16
			 *              FF /3   call far, absolute indirect, address given in m16:32
			 *
			 *              FF /4   jmp near, absolute indirect, address given in r/m16
			 *              FF /4   jmp near, absolute indirect, address given in r/m32
			 *              FF /5   jmp near, absolute indirect, address given in r/m16
			 *              FF /5   jmp near, absolute indirect, address given in r/m32
			 *
			 *              /digit indicates that the Reg/Opcode field (bits 3-5) of the ModR/M byte (opcode[1])
			 *              contains that value as an opcode instead of a register operand indicator.  The instruction only
			 *              uses the R/M field (bits 0-2) as an operand.
			 *
			 *              cb,cw,cd,cp indicates a 1,2,4,6-byte following the opcode is used to specify a
			 *                      code offset and possibly a new code segment
			 *
			 *      I believe we only care about these this version:
			 *              FF /3   call far, absolute indirect, address given in m16:32
			 *      as we're looking for an address on the stack.
			 *
			 *      The ModR/M byte must be Mod=10, Reg/Op=010, R/M=100 aka 0x94 or possibly
			 *                              Mod=01, Reg/Op=100, R/M=100 aka 0x64 or possibly
			 *                              Mod=10, Reg/Op=100, R/M=100 aka 0xa4 or possibly
			 *                              Mod=01, Reg/Op=010, R/M=100 aka 0x54 or possibly
			 *      R/M=100 indicates to read the SIB (see below)
			 *      Mod=10 indicates an 32-bit displacement after the SIB byte
			 *      Mod=01 indicates an  8-bit displacement after the SIB byte
			 *      Reg/Op=010 indicates an opcode that has a /3 after it
			 *      Reg/Op=100 indicates an opcode that has a /5 after it
			 *
			 *      The SIB byte (opcode[2]) needs to be scale=00, index=100, base=100, or 0x24
			 *              indicating that the addresing mode is 0*%esp+%esp.
			 *
			 *      I believe that the SIB byte could also be 0x64, 0xA4, or 0xE4, but that
			 *      these are very rarely used as they are redundant.
			 *
			 *
			 *
			 *
			 */
			/* non-negative offsets please */
			assert(offset>=0);

			int sib_byte=(unsigned char)newbits[2];
			int sib_base=(unsigned char)sib_byte&0x7;
			//int sib_ss=(sib_byte>>3)&0x7;
			//int sib_index=(sib_byte>>3)&0x7;


			/* 32-bit offset */
			if ( (unsigned char)newbits[0] == 0xff &&                                	/* ff */
			     ((unsigned char)newbits[1] == 0x94 || (unsigned char)newbits[1] == 0x64 )    && 		/* /2  or /4*/
			     sib_base == 0x4 )                                          /* base==esp */
			{
				// reconstruct the old 32-bit value
				int oldval=((unsigned char)newbits[6]<<24)+((unsigned char)newbits[5]<<16)+((unsigned char)newbits[4]<<8)+((unsigned char)newbits[3]);

				// add the offset 
				int newval=oldval+offset;

				// break it back apart to store in the string.
				newbits[3]=(char)(newval>>0)&0xff;
				newbits[4]=(char)(newval>>8)&0xff;
				newbits[5]=(char)(newval>>16)&0xff;
				newbits[6]=(char)(newval>>24)&0xff;
			}

			/* 8-bit offset */
			else if ( (unsigned char)newbits[0] == 0xff &&                                  //   ff 
			     ((unsigned char)newbits[1] == 0x54 || (unsigned char)newbits[1]==0xa4) &&  //   /3 or /5 
			     sib_base == 0x4 )                                                          //   base==esp 
			{
				/* We need to add 4 to the offset, but this may overflow an 8-bit quantity
				 * (note:  there's no 16-bit offset for this insn.  Only 8-bit or 32-bit offsets exist for this instr)
				 * if the const offset is over (0x7f-offset), adding offset will overflow it to be negative instead
				 * of positive
				 */
				if((unsigned char)newbits[3]>=(0x7f-offset))
				{
					/* Careful, adding 4 to this will cause an overflow.
					 * We need to convert it to a 32-bit displacement
					 */
					/* first, change addressing mode from 8-bit to 32-bit. */
					newbits[1]&=0x3f;         /* remove upper 2 bits */
					newbits[1]|=0x80;         /* make them 10 to indicate a 32-bit offset */

					// sign-extend to 32-bit int
					int oldval=(char)newbits[3];

					// add the offset 
					int newval=oldval+offset;
			
					// break it back apart to store in the string.
					assert(newbits.length() == 4);
					newbits[3]=(char)(newval>>0)&0xff;
					// 3 most significant bytes extend the instruction
					newbits+=(char)(newval>>8)&0xff;
					newbits+=(char)(newval>>16)&0xff;
					newbits+=(char)(newval>>24)&0xff;

				}
				else

					newbits[3] += (char)offset;
			}
			return newbits;
		}

			

		/* 
		 * convert_to_jump - assume newbits is a call insn, convert newbits to a jump, and return it.
		 * Also: if newbits is a call [esp+k], add "offset" to k.
		 */ 
		void convert_to_jump(Instruction_t* insn, int offset)
		{
			string newbits=insn->getDataBits();
			auto dp=DecodedInstruction_t::factory(insn);
			auto &d=*dp;

			/* this case is odd, handle it specially (and more easily to understand) */
			if(strcmp(d.getDisassembly().c_str(), "call qword [rsp]")==0)
			{
				char buf[100];
				sprintf(buf,"jmp qword [rsp+%d]", offset);
				insn->assemble(buf);
				return;
			}


			/* skip over any prefixes */
			int op_index=d.getPrefixCount(); // d.Prefix.Number;

			// on the opcode.  assume no prefix here 	
			switch((unsigned char)newbits[op_index])
			{
				// opcodes: ff /2 and ff /3
				case 0xff:	
				{
					// opcode: ff /2
					// call r/m32, call near, absolute indirect, address given in r/m32
					if(((((unsigned char)newbits[op_index+1])&0x38)>>3) == 2)
					{
						newbits=adjust_esp_offset(newbits,offset);
						// convert to jmp r/m32
						// opcode: FF /4
						newbits[op_index+1]&=0xC7;	// remove old bits
						newbits[op_index+1]|=(0x4<<3);	// set r/m field to 4
					}
					// opcode: ff /3
					// call m16:32, call far, absolute indirect, address given in m16:32	
					else if(((((unsigned char)newbits[op_index+1])&0x38)>>3) == 3)
					{
						// convert to jmp m16:32
						// opcode: FF /5
						newbits[op_index+1]&=0xC7;	// remove old bits
						newbits[op_index+1]|=(0x5<<3);	// set r/m field to 5
					}
					else
						assert(0);
					break;
				}
				// opcode: 9A cp
				// call ptr16:32, call far, absolute, address given in operand
				case 0x9A:	
				{
					// convert to jmp ptr16:32
					// opcode: EA cp
					newbits[op_index+0]=0xEA;
					break;
				}

				// opcode: E8 cd
				// call rel32, call near, relative, displacement relative to next insn.
				case 0xE8:
				{
					// convert to jmp rel32
					// opcode: E9 cd
					newbits[op_index+0]=0xE9;
					break;
				}

				// not a call
				default:
					assert(0);
			}

			/* set the instruction's bits */
			insn->setDataBits(newbits);
			return;
		}


		/* 
		 * fix_call - convert call to push/jump.
		 */
		void fix_call(Instruction_t* insn, FileIR_t *firp, bool can_unpin)
		{
			// doesn't work for ARM64 yet.
			if(firp->getArchitecture()->getMachineType()==admtAarch64)
				return;

			/* record the possibly new indirect branch target if this call gets fixed */
			Instruction_t* newindirtarg=insn->getFallthrough();

			/* Disassemble the instruction */
			auto disasmp=DecodedInstruction_t::factory (insn);
			auto &disasm=*disasmp;

			/* if this instruction is an inserted call instruction than we don't need to 
			 * convert it for correctness' sake.
			 */
			if(insn->getAddress()->getVirtualOffset()==0)
				return;

			/* if the first byte isn't a call opcode, there's some odd prefixing and we aren't handling it.
			 * this comes up most frequently in a call gs:0x10 instruction where an override prefix specifes the gs: part.
			 */
			if((insn->getDataBits()[0]&0x40)==0x40)
			{
				// ignore rex prefixes
			}
			else if( (insn->getDataBits()[0]!=(char)0xff) && 
				 (insn->getDataBits()[0]!=(char)0xe8) && 
				 (insn->getDataBits()[0]!=(char)0x9a) )
			{
				cout<<"Found odd prefixing.\n  Not handling **********************************************"<<endl;
				assert(0);
				return;
			}

			if(getenv("VERBOSE_FIX_CALLS"))
			{
				cout<<"Doing a fix_call on "<<hex<<insn->getAddress()->getVirtualOffset()<< " which is "<<disasm.getDisassembly() /*.CompleteInstr*/<<endl;
			}

			/* calculate the call's fallthrough addr with and without the file base */  
			const auto next_addr_with_fb = insn->getAddress()->getVirtualOffset() + insn->getDataBits().length();
			const auto next_addr_no_fb   = uint32_t(next_addr_with_fb - firp->getArchitecture()->getFileBase());

			/* create a new instruction and a new addresss for it that do not correspond to any original program address */
			const auto callinsn          = firp->addNewInstruction();
			const auto calladdr          = firp->addNewAddress(insn->getAddress()->getFileID(),0);

			/* set the fields in the new instruction */
			callinsn->setAddress(calladdr);
			callinsn->setTarget(insn->getTarget());
			callinsn->setFallthrough(NULL);
			callinsn->setFunction(insn->getFunction());
			callinsn->setComment(insn->getComment()+" Jump part");

			/* handle ib targets */
			callinsn->setIBTargets(insn->getIBTargets());
			insn->setIBTargets(NULL);

			// We need the control transfer instruction to be from the orig program because 
			// if for some reason it's fallthrough/target isn't in the DB, we need to correctly 
			// emit fallthrough/target rules
			callinsn->setOriginalAddressID(insn->getOriginalAddressID());
			insn->setOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);

			/* set the new instruction's data bits to be a jmp instead of a call */
			string newbits="";

			/* add 4 (8) if it's an esp(rsp) indirect branch for x86-32 (-64) */ 
			callinsn->setDataBits(insn->getDataBits());
			convert_to_jump(callinsn,firp->getArchitectureBitWidth()/8);		

			/* the jump instruction should NOT be indirectly reachable.  We should
			 * land at the push
			 */
			fix_other_pcrel(firp, callinsn, insn->getAddress()->getVirtualOffset());
			callinsn->setIndirectBranchTargetAddress(NULL);

			/* add the new insn and new address into the list of valid calls and addresses */


			/* Convert the old call instruction into a push return_address instruction */
			insn->setFallthrough(callinsn);
			insn->setTarget(NULL);
			newbits=string("");
			newbits.resize(5);
			newbits[0]=0x68;	/* assemble an instruction push next_addr */
			newbits[1]=(next_addr_no_fb>>0) & 0xff;
			newbits[2]=(next_addr_no_fb>>8) & 0xff;
			newbits[3]=(next_addr_no_fb>>16) & 0xff;
			newbits[4]=(next_addr_no_fb>>24) & 0xff;
			insn->setDataBits(newbits);
			insn->setComment(insn->getComment()+" Push part");

			/* create a relocation for this instruction */
			auto reloc= firp->getArchitectureBitWidth()==32 ? firp->addNewRelocation(insn, 1, "32-bit") :
				    firp->getArchitectureBitWidth()==64 ? firp->addNewRelocation(insn, 0, "push64") :
				    throw invalid_argument("odd bit width?");



			/* If the fallthrough is not marked as indirectly branchable-to, then mark it so */
			if(newindirtarg && !newindirtarg->getIndirectBranchTargetAddress())
			{
				/* create a new address for the IBTA */
				auto newaddr=firp->addNewAddress(newindirtarg->getAddress()->getFileID(), newindirtarg->getAddress()->getVirtualOffset());

				/* set the instruction and include this address in the list of addrs */
				newindirtarg->setIndirectBranchTargetAddress(newaddr);
			
				// if we're marking this as an IBTA, determine whether we can unpin it or not 
				if(can_unpin)
				{
					if(getenv("VERBOSE_FIX_CALLS"))
					{
						cout<<"setting unpin for type="<< reloc->getType()<< " address="
						    <<hex<<insn->getBaseID()<<":"<<insn->getDisassembly()<<endl;
					}
					// set newindirtarg as unpinned
					newindirtarg->getIndirectBranchTargetAddress()->setVirtualOffset(0);
					reloc->setWRT(newindirtarg);
				}
			}


			// mark in the IR what the fallthrough of this insn is and give the reloc to the IR.
			(void)firp->addNewRelocation(callinsn, 0, "fix_call_fallthrough", newindirtarg);
		}


		//
		// return true if insn is a call
		//
		bool is_call(Instruction_t* insn)
		{
			/* Disassemble the instruction */
			auto disasm=DecodedInstruction_t::factory (insn);
			return disasm->isCall(); 
		}

		bool can_skip_safe_function(Instruction_t *call_insn) 
		{
			if (!call_insn)
				return false;
			if (!is_call(call_insn))
				return false;
			Instruction_t *target=call_insn->getTarget();
			if (!target)
				return false;
			auto func=target->getFunction();
			if (!func)
				return false;

			/* if the call instruction isn't to a function entry point */
			if(func->getEntryPoint()!=target)
			{
				return false;
			}

			if (func->isSafe())
			{
				cout << "Function " << func->getName() << " is safe" << endl;
			}

			return func->isSafe();
		}


		template <class T> struct insn_less : binary_function <T,T,bool> 
		{
			bool operator() (const T& x, const T& y) const 
			{
				return  make_tuple(x->getBaseID(),x) < make_tuple(y->getBaseID(),y);
			}
		};


		// 
		// Mark ret_point as an unpinned IBT.
		//
		void mark_as_unpinned_ibt(FileIR_t* firp, Instruction_t* ret_point)
		{
			if( ret_point == NULL ) return;
			if( ret_point->getIndirectBranchTargetAddress() != NULL ) return;
			
			auto newaddr=firp->addNewAddress(ret_point->getAddress()->getFileID(),0);
			ret_point->setIndirectBranchTargetAddress(newaddr);
			
		}

		//
		// fix_all_calls - convert calls to push/jump pairs in the IR.  if fix_all is true, all calls are converted, 
		// else we attempt to detect the calls it is safe to convert.
		//
		void fix_all_calls(FileIR_t* firp, bool fix_all)
		{

			const auto sorted_insns = set<Instruction_t*,insn_less<Instruction_t*> >(ALLOF(firp->getInstructions()));
			const auto fcl          = getenv("FIX_CALL_LIMIT");
			const auto sfca         = getenv("STOP_FIX_CALLS_AT");

			for(auto insn : sorted_insns)
			{
				if(getenv("STOP_FIX_CALLS_AT") && fixed_calls>=(size_t)atoi(
						sfca
							))
					break;

				if(is_call(insn)) 
				{
					if( call_needs_fix(insn) )	// fixing is necessary + unpinning not possible.
					{
						fixed_calls++;
						fix_call(insn, firp, false);
					}
					// we've been asked to fix all calls for funsies/cfi
					// (and a bit about debugging fix-calls that's not important for anyone but jdh.
					else if ( fix_all || (fcl && not_fixed_calls>=(size_t)atoi(fcl)))
					{
						auto fix_me = true;
						if (!opt_fix_safefn && can_skip_safe_function(insn))
						{
							fix_me = false;
							no_fix_for_safefn++;
						}

						if(fix_me)
						{
							// if we make it here, we know that it was not 100% necessary to fix the call
							// but we've been asked to anyhow.	
							fixed_calls++;
							fix_call(insn, firp, pin_fixed);
						}
						else
						{
							not_fixed_calls++;
						}
					}
					else
					{
						if(getenv("VERBOSE_FIX_CALLS"))
							cout<<"No fix needed, marking ret site IBT, for "<<insn->getAddress()->getVirtualOffset()<<":"<<insn->getDisassembly()<<endl;
						mark_as_unpinned_ibt(firp, insn->getFallthrough());
						not_fixed_calls++;
					}
				}
				
				else
				{
					if(getenv("VERBOSE_FIX_CALLS"))
						cout<<"Not a call "<<insn->getAddress()->getVirtualOffset()<<":"<<insn->getDisassembly()<<endl;
					not_calls++;
				}
			}

			cout << "# ATTRIBUTE fix_calls::fixed_calls="<<dec<<fixed_calls<<endl;
			cout << "# ATTRIBUTE fix_calls::no_fix_needed_calls="<<dec<<not_fixed_calls<<endl;
			cout << "# ATTRIBUTE fix_calls::other_instructions="<<dec<<not_calls<<endl;
			cout << "# ATTRIBUTE fix_calls::fixed_pct="<<fixed<<(((float)fixed_calls)/((float)(not_fixed_calls+fixed_calls+not_calls)))*100.00<<"%"<<endl;
			cout << "# ATTRIBUTE fix_calls::remaining_ratio="<<fixed<<((float)not_fixed_calls/((float)(not_fixed_calls+fixed_calls+not_calls)))*100.00<<"%"<<endl;
			cout << "# ATTRIBUTE fix_calls::other_insts_ratio="<<fixed<<((float)not_calls/((float)(not_fixed_calls+fixed_calls+not_calls)))*100.00<<"%"<<endl;
			cout << "# ATTRIBUTE fix_calls::no_target_insn="<<dec<< no_target_insn << endl;
			cout << "# ATTRIBUTE fix_calls::no_fallthrough_insn="<<dec<< no_fallthrough_insn << endl;
			cout << "# ATTRIBUTE fix_calls::target_not_in_function="<<dec<< target_not_in_function << endl;
			cout << "# ATTRIBUTE fix_calls::call_to_not_entry="<<dec<< call_to_not_entry << endl;
			cout << "# ATTRIBUTE fix_calls::thunk_check="<<dec<< thunk_check << endl;
			cout << "# ATTRIBUTE fix_calls::found_pattern="<<dec<< found_pattern << endl;
			cout << "# ATTRIBUTE fix_calls::in_ehframe="<<dec<< in_ehframe << endl;
			cout << "# ATTRIBUTE fix_calls::no_fix_for_ib="<<dec<< no_fix_for_ib << endl;
			cout << "# ATTRIBUTE fix_calls::no_fix_for_safefn="<<dec<< no_fix_for_safefn << endl;
		}


		//
		//  fix_other_pcrel - add relocations to other instructions that have pcrel bits
		//
		void fix_other_pcrel(FileIR_t* firp, Instruction_t *insn, uintptr_t virt_offset)
		{
			/* if this has already been fixed, we can skip it */
			if(virt_offset == 0 || virt_offset == (uintptr_t)-1)
				return;

			const auto disasm    = DecodedInstruction_t::factory(insn);
			const auto &operands = disasm->getOperands();
			for(const auto &op : operands)
			{
				const auto &the_arg = *op;
				const auto  is_rel  = the_arg.isPcrel(); 
				const auto  is_read = the_arg.isRead();

				if(is_rel && is_read)
				{
					const auto mt       = firp->getArchitecture()->getMachineType();
					if(mt==admtAarch64 || mt==admtArm32)
					{
						// figure out how to rewrite pcrel arm insns, then change the virt addr
						// insn->getAddress()->setVirtualOffset(0);	
						// for now, we aren't doing this... we may need to for doing xforms.
						if(getenv("VERBOSE_FIX_CALLS"))
							cout << "Detected arm32/64 pc-rel operand in " << disasm->getDisassembly()  << endl;
					}
					else if(mt==admtX86_64 ||  mt==admtI386)
					{
						assert(the_arg.isMemory());
						auto offset=disasm->getMemoryDisplacementOffset(&the_arg, insn); 
						assert(offset>=0 && offset <=15);
						auto size=the_arg.getMemoryDisplacementEncodingSize(); 
						assert(size==1 || size==2 || size==4 || size==8);

						if(getenv("VERBOSE_FIX_CALLS"))
						{
							cout<<"Found insn with pcrel memory operand: "<<disasm->getDisassembly()
							    <<" Displacement="<<hex<<the_arg.getMemoryDisplacement() << dec
							    <<" size="<<the_arg.getMemoryDisplacementEncodingSize() <<" Offset="<<offset;
						}

						/* convert [rip_pc+displacement] addresssing mode into [rip_0+displacement] where rip_pc is the actual PC of the insn, 
						 * and rip_0 is means that the PC=0. AKA, we are relocating this instruction to PC=0. Later we add a relocation to undo this transform at runtime 
						 * when we know the actual address.
						 */

						/* get the data */
						string data=insn->getDataBits();
						char cstr[20]={}; 
						memcpy(cstr,data.c_str(), data.length());
						void *offsetptr=&cstr[offset];

						auto disp=the_arg.getMemoryDisplacement(); 
						auto oldpc=virt_offset;
						auto newdisp=disp+oldpc-firp->getArchitecture()->getFileBase();

						assert((uintptr_t)(offset+size)<=(uintptr_t)(data.length()));
						
						switch(size)
						{
							case 4:
								assert( (uintptr_t)(int)newdisp == (uintptr_t)newdisp);
								*(int*)offsetptr=newdisp;
								break;
							case 1:
							case 2:
							case 8:
							default:
								assert(0);
								//assert(("Cannot handle offset of given size", 0));
						}

						/* put the data back into the insn */
						data.replace(0, data.length(), cstr, data.length());
						insn->setDataBits(data);

						other_fixes++;

						if(getenv("VERBOSE_FIX_CALLS"))
							cout << " Converted to: " << insn->getDisassembly() << endl;

						// and it's important to set the VO to 0, so that the pcrel-ness is calculated correctly.
						insn->getAddress()->setVirtualOffset(0);	
					}
					else
						throw std::invalid_argument("Unknown architecture in fix_other_pcrel");

					// now that we've done the rewriting, go ahead and add the reloc.
					auto reloc=firp->addNewRelocation(insn,0,"pcrel");
					(void)reloc; // not used, only given to the IR

				}
			}
		}

		void fix_safefr(FileIR_t* firp, Instruction_t *insn, uintptr_t virt_offset)
		{
			/* if this has already been fixed, we can skip it */
			if(virt_offset==0 || virt_offset==(uintptr_t)-1)
				return;

			for(auto reloc : insn->getRelocations())
			{
				assert(reloc);
				if( reloc->getType() == "safefr" )
				{
					auto addr=firp->addNewAddress(insn->getAddress()->getFileID(), 0);
					insn->setAddress(addr);
				}
			}
		}


		void fix_other_pcrel(FileIR_t* firp)
		{
			for(auto insn : firp->getInstructions())
			{
				fix_other_pcrel(firp,insn, insn->getAddress()->getVirtualOffset());
				fix_safefr(firp,insn, insn->getAddress()->getVirtualOffset());
			}
			cout << "# ATTRIBUTE fix_calls::other_fixes="<<dec<<other_fixes<<endl;
		}

		//
		// main rountine; convert calls into push/jump statements 
		//
		// int main(int argc, char* argv[])


		bool fix_all=false;
		bool pin_fixed=false;
		bool do_eh_frame=true;

		DatabaseID_t variant_id=BaseObj_t::NOT_IN_DATABASE;

	public:
		int parseArgs(const vector<string> step_args)
		{


			for(auto argc_iter=0u; argc_iter<step_args.size(); argc_iter++)
			{
				if("--pin-fixed"==step_args[argc_iter])
				{
					pin_fixed=true;

				}
				else if("--no-pin-fixed"==step_args[argc_iter])
				{
					pin_fixed=false;

				}
				else if("--fix-all"==step_args[argc_iter])
				{
					fix_all=true;
				}
				else if("--no-fix-all"==step_args[argc_iter])
				{
					fix_all=false;
				}
				else if("--eh-frame"==step_args[argc_iter])
				{
					do_eh_frame=true;
				}
				else if("--no-eh-frame"==step_args[argc_iter])
				{
					do_eh_frame=false;
				}
				else if("--fix-icalls"==step_args[argc_iter])
				{
					opt_fix_icalls = true;
				}
				else if("--no-fix-icalls"==step_args[argc_iter])
				{
					opt_fix_icalls = false;
				}
				else if("--fix-safefn"==step_args[argc_iter])
				{
					opt_fix_safefn = true;
				}
				else if("--no-fix-safefn"==step_args[argc_iter])
				{
					opt_fix_safefn = false;
				}
				else
				{
					cerr<<"Unrecognized option: "<<step_args[argc_iter]<<endl;
					return -1;
				}
			}
			if(getenv("FIX_CALLS_FIX_ALL_CALLS"))
				fix_all=true;

			return 0;
		}

		int executeStep()
		{
			variant_id=getVariantID();
			auto irdb_objects=getIRDBObjects();

			cout<<"Reading variant "<<variant_id<<" from database." << endl;
			try 
			{
				/* setup the interface to the sql server */
				const auto pqxx_interface=irdb_objects->getDBInterface();
				BaseObj_t::setInterface(pqxx_interface);

				auto  pidp = irdb_objects->addVariant(variant_id);
				cout<<"Fixing calls->push/jmp in variant "<<*pidp<< "." <<endl;

				assert(pidp->isRegistered()==true);

				for(const auto &this_file : pidp->getFiles())
				{
					assert(this_file);

					// read the db  
					auto firp = irdb_objects->addFileIR(variant_id, this_file->getBaseID());
			
					assert(firp && pidp);
			
					eh_frame_ranges.clear();
					int elfoid=firp->getFile()->getELFOID();
					pqxx::largeobject lo(elfoid);
					lo.to_file(pqxx_interface->getTransaction(),"readeh_tmp_file.exe");
					EXEIO::exeio*    elfiop=new EXEIO::exeio;
					elfiop->load(string("readeh_tmp_file.exe"));
					EXEIO::dump::header(cout,*elfiop);
					EXEIO::dump::section_headers(cout,*elfiop);
					// do eh_frame reading as required. 
					if(do_eh_frame)
						read_ehframe(firp, elfiop);
					setFrameSizes(firp);

					fix_all_calls(firp,fix_all);
					fix_other_pcrel(firp);

					cout<<"Done!"<<endl;

					if(firp->getArchitecture()->getMachineType() != admtAarch64)
						assert(getenv("SELF_VALIDATE")==nullptr || (fixed_calls + other_fixes) > 5);


				}
			}
			catch (DatabaseError_t pnide)
			{
				cout<<"Unexpected database error: "<<pnide<<endl;
				return -1;
			}
			catch(...)
			{
				cerr<<"Unexpected error"<<endl;
				return -1;
			}

			assert(getenv("SELF_VALIDATE")==nullptr || fix_all || not_fixed_calls > 5);

			return 0;
		}

		void range(VirtualOffset_t a, VirtualOffset_t b)
		{
			// we've found examples of ranges being 0 sized, and it's a bit weird what that means.
			// it applies to 0 instructions?
			// skip it, it's likely an invalid FDE.
			if(a==b)
				return; 
			// non-zero sized fde
			assert(a<b);

			const auto rangeiter=eh_frame_ranges.find(Range_t(a+1,a+1));
			assert(rangeiter==eh_frame_ranges.end());

			eh_frame_ranges.insert(Range_t(a+1,b));	// ranges are interpreted as (a,b]
		}

		bool possible_target(uintptr_t p, uintptr_t at, ibt_provenance_t prov)
		{
			// used for LDSA
			return false;
		}


		string getStepName(void) const override
		{
			return string("fix_calls");
		}

	private:
		void setFrameSizes(FileIR_t* firp)
		{
			for(auto func : firp->getFunctions())
			{
				if(func->getEntryPoint()==nullptr) continue;

				const auto is_found_it=cfg_optimizer.find(func);
				const auto is_found=(is_found_it!=end(cfg_optimizer));

				if(!is_found)
					/* build a cfg for this function */
					cfg_optimizer[func]=shared_ptr<ControlFlowGraph_t>(move(ControlFlowGraph_t::factory(func)));

				const auto cfg=cfg_optimizer[func].get();
				const auto entry_block=cfg->getEntry();
				auto pushes=0;
				for(auto insn : entry_block->getInstructions())
				{
					const auto di=DecodedInstruction_t::factory(insn);
					const auto mnemonic=di->getMnemonic();
					if(mnemonic=="push")
						pushes++;
					if(mnemonic=="sub")
					{
						const auto hasop0    = di->hasOperand(0);
						const auto op0_sp    = hasop0 && (di->getOperand(0)->getString()=="rsp" || di->getOperand(0)->getString()=="esp");
						const auto hasop1    = di->hasOperand(1);
						const auto op1_const = hasop1 && di->getOperand(1)->isConstant();
						if(op0_sp && op1_const)
						{
							func->setStackFrameSize(di->getOperand(1)->getConstant());
						}
						break;
					}

				}
			}
		}

}; // end class FixCalls_t

shared_ptr<TransformStep_t> curInvocation;

bool possible_target(VirtualOffset_t p, VirtualOffset_t from_addr, ibt_provenance_t prov)
{
        assert(curInvocation);
        return (dynamic_cast<FixCalls_t*>(curInvocation.get()))->possible_target(p,from_addr,prov);
}

void range(VirtualOffset_t start, VirtualOffset_t end)
{
        assert(curInvocation);
        return (dynamic_cast<FixCalls_t*>(curInvocation.get()))->range(start,end);
}

extern "C"
shared_ptr<TransformStep_t> getTransformStep(void)
{
        curInvocation.reset(new FixCalls_t());
        return curInvocation;
}


