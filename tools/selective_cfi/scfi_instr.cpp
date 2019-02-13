/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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


#include <algorithm>
#include "scfi_instr.hpp"
#include "color_map.hpp"
#include <stdlib.h>
#include <memory>
#include <math.h>
#include <exeio.h>
#include <elf.h>


using namespace std;
using namespace IRDB_SDK;

string getRetDataBits()
{
        string dataBits;
        dataBits.resize(1);
        dataBits[0] = 0xc3;
        return dataBits;
}



Relocation_t* SCFI_Instrument::FindRelocation(Instruction_t* insn, string type)
{
        RelocationSet_t::iterator rit;
        for( rit=insn->getRelocations().begin(); rit!=insn->getRelocations().end(); ++rit)
        {
                Relocation_t& reloc=*(*rit);
                if(reloc.getType()==type)
                {
                        return &reloc;
                }
        }
        return NULL;
}

bool SCFI_Instrument::isSafeFunction(Instruction_t* insn)
{
	return (insn && insn->getFunction() && insn->getFunction()->isSafe());
}

bool SCFI_Instrument::isCallToSafeFunction(Instruction_t* insn)
{
	if (insn && insn->getTarget() && insn->getTarget()->getFunction())
	{
		if(getenv("SCFI_VERBOSE")!=NULL)
		{
			if (insn->getTarget()->getFunction()->isSafe())
			{
				cout << "Function " << insn->getTarget()->getFunction()->getName() << " is deemed safe" << endl;
			}
		}

		return insn->getTarget()->getFunction()->isSafe();
	}

	return false;
}

Relocation_t* SCFI_Instrument::create_reloc(Instruction_t* insn)
{
        /*
	 * Relocation_t* reloc=new Relocation_t;
	*/
	auto reloc=firp->addNewRelocation(insn,0,"error-if-seen"); // will update offset and type in caller

	return reloc;
}

bool SCFI_Instrument::add_scfi_instrumentation(Instruction_t* insn)
{
	bool success=true;

	if(getenv("SCFI_VERBOSE")!=NULL)
		;


	return success;
}

bool SCFI_Instrument::needs_scfi_instrumentation(Instruction_t* insn)
{
assert(0);
	return false;

}

unsigned int SCFI_Instrument::GetExeNonceOffset(Instruction_t* insn)
{
        if(exe_nonce_color_map)
	{
		assert(insn->getIBTargets());
                // We can't know the offset yet because this nonce may need to get
                // shoved into multiple exe nonces, so just return the position.
		cout << "EXE NONCE OFFSET IS: "<< exe_nonce_color_map->GetColorOfIB(insn).GetPosition() << endl;
		return exe_nonce_color_map->GetColorOfIB(insn).GetPosition();
	}
        // Otherwise, only one nonce per target and it's always in the first position
	return 0;
}

NonceValueType_t SCFI_Instrument::GetExeNonce(Instruction_t* insn)
{
	if(exe_nonce_color_map)
	{
		assert(insn->getIBTargets());
		return exe_nonce_color_map->GetColorOfIB(insn).GetNonceValue();
	}
        // Otherwise, all nonces are the same
        switch(exe_nonce_size)
        {
            case 1:
                return 0xcc;
            case 2:
                return 0xbbcc;
            case 4:
                return 0xaabbccdd;
            case 8:
                return 0xaabbccdd00112233; 
            default:
                exit(1); // wat?
        }
}

unsigned int SCFI_Instrument::GetExeNonceSize(Instruction_t* insn)
{
    return exe_nonce_size;
}

unsigned int SCFI_Instrument::GetNonceOffset(Instruction_t* insn)
{
	if(color_map)
	{
		assert(insn->getIBTargets());
		return (color_map->GetColorOfIB(insn).GetPosition()+1) * GetNonceSize(insn);
	}
	return GetNonceSize(insn);
}

NonceValueType_t SCFI_Instrument::GetNonce(Instruction_t* insn)
{
	/* in time we look up the nonce category for this insn */
	/* for now, it's just f4 as the nonce */
	if(color_map)
	{
		assert(insn->getIBTargets());
		return color_map->GetColorOfIB(insn).GetNonceValue();
	}
        // Otherwise, all nonces are the same
	switch(nonce_size)
        {
            case 1:
                return 0xcc;
            case 2:
                return 0xbbcc;
            case 4:
                return 0xaabbccdd;
            case 8:
		return 0xaabbccdd00112233;                
            default:
                exit(1); // wat?
        }
}

unsigned int SCFI_Instrument::GetNonceSize(Instruction_t* insn)
{
    return nonce_size;
}

bool SCFI_Instrument::mark_targets() 
{
	int targets=0, ind_targets=0, exe_nonce_targets=0;
	// Make sure no unresolved instructions are in the insn set
	firp->assembleRegistry();
	firp->setBaseIDS();	
	// create new preds (we've added instructions)
	auto newPredsp=InstructionPredecessors_t::factory(firp);
	auto &newPreds=*newPredsp;
	// Make sure the new insns added in this loop are not processed in this loop
	// (ok since none of the them should receive nonces)
	auto insn_set = firp->getInstructions();
	for(auto insn : insn_set) 
	{
		targets++;
		if(insn->getIndirectBranchTargetAddress())
		{

			// make sure there are no fallthroughs to nonces.
			for(auto the_pred : newPreds[insn])
			{
				if(the_pred->getFallthrough()==insn)
				{
					Instruction_t* jmp=addNewAssembly("jmp 0x0");
					the_pred->setFallthrough(jmp);
					jmp->setTarget(insn);
				}
			}

			ind_targets++;
			string type;
			if(do_coloring)
			{
				ColoredSlotValues_t v=color_map->GetColorsOfIBT(insn);
				int size=GetNonceSize(insn);
				for(auto i=0U;i<v.size();i++)
				{
					if(!v[i].IsValid())
						continue;
					int position=v[i].GetPosition();

					// convert the colored "slot" into a position in the code.
					position++;
					position*=size;
					position = - position;

					// cfi_nonce=(pos=-1,nv=0x33,sz=1)
					NonceValueType_t noncevalue=v[i].GetNonceValue();
					type=string("cfi_nonce=(pos=") +  to_string(position) + ",nv="
						+ to_string(noncevalue) + ",sz="+ to_string(size)+ ")";
					Relocation_t* reloc=create_reloc(insn);
					reloc->setOffset(-position*size);
					reloc->setType(type);
					cout<<"Created reloc='"+type+"' for "<<std::hex<<insn->getBaseID()<<":"<<insn->getDisassembly()<<endl;
				}
			}
			else
			{
				type=string("cfi_nonce=(pos=") +  to_string(-((int) GetNonceOffset(insn))) + ",nv="
                                    + to_string(GetNonce(insn)) + ",sz="+ to_string(GetNonceSize(insn))+ ")";
				Relocation_t* reloc=create_reloc(insn);
				reloc->setOffset(-((int) GetNonceOffset(insn)));
				reloc->setType(type);
				cout<<"Created reloc='"+type+"' for "<<std::hex<<insn->getBaseID()<<":"<<insn->getDisassembly()<<endl;
			}
		}

		if(do_exe_nonce_for_call && DecodedInstruction_t::factory(insn)->isCall()) 
                {
		    ++exe_nonce_targets;
                 
                    if(do_color_exe_nonces)
                    {
                        // The colors we want are not on the call, but on its return target.
                        ColoredSlotValues_t v=exe_nonce_color_map->GetColorsOfIBT(insn->getFallthrough());
			
			// The return target gets a regular nonce as well, so there may be an inserted jmp
			// in the way of the original IBT that has our colors.
			if(v.size() == 0)
			{
			    v=exe_nonce_color_map->GetColorsOfIBT(insn->getFallthrough()->getTarget());
			}

                        int nonceSz=GetExeNonceSize(insn); //Multiple sizes not yet supported	
    
                        for(auto i=0U;i<v.size();i++)
                        {
                            int noncePos;
                            NonceValueType_t nonceVal;
                            if(!v[i].IsValid())
                            {
                                // This position is not valid, but need to fill it with something anyway
                                // because nonces are executable
                                noncePos = i; //FIXME: BAD COUPLING. Relies on the fact that i == position number
                                nonceVal = 0xc;
                            }
                            else
                            {
                                noncePos=v[i].GetPosition();
                                nonceVal = v[i].GetNonceValue();
                            }
                            PlaceExeNonceReloc(insn, nonceVal, nonceSz, noncePos);
                        }
                        // Place exe nonce reloc to fill extra space (if any)
                        int isExtraSpace = (v.size()*nonceSz) % EXE_NONCE_ARG_SIZE;
                        if(isExtraSpace)
                        {
                            int extraSpace = EXE_NONCE_ARG_SIZE - ((v.size()*nonceSz) % EXE_NONCE_ARG_SIZE);
                            int extraSpacePos = v.size();
                            int extraSpaceBytePos = GetNonceBytePos(nonceSz, extraSpacePos-1)+nonceSz;
                            CreateExeNonceReloc(insn, 0, extraSpace, extraSpaceBytePos);
                        }
                    }
                    else
                    {
                    	int nonceSz = GetExeNonceSize(insn);
                        PlaceExeNonceReloc(insn, GetExeNonce(insn), nonceSz, GetExeNonceOffset(insn));
                        // Place exe nonce reloc to fill extra space (if any)
                        int isExtraSpace = nonceSz % EXE_NONCE_ARG_SIZE;
                        if(isExtraSpace)
                        {
                            int extraSpace = EXE_NONCE_ARG_SIZE - (nonceSz % EXE_NONCE_ARG_SIZE);
                            int extraSpacePos = 1;
                            int extraSpaceBytePos = GetNonceBytePos(nonceSz, extraSpacePos-1)+nonceSz;
                            CreateExeNonceReloc(insn, 0, extraSpace, extraSpaceBytePos);
                        } 
                    }
                } 
	}

	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::ind_targets_found="<<std::dec<<ind_targets<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::targets_found="<<std::dec<<targets<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::exe_nonce_targets_found="<<std::dec<<exe_nonce_targets<<endl;

	assert(getenv("SELF_VALIDATE")==nullptr || ind_targets > 5 );
	assert(getenv("SELF_VALIDATE")==nullptr || targets > 5 );
	assert(getenv("SELF_VALIDATE")==nullptr || !do_exe_nonce_for_call || exe_nonce_targets > 5 );

	return true;
}


void SCFI_Instrument::CreateExeNonceReloc(Instruction_t* insn, NonceValueType_t nonceVal, int nonceSz, int bytePos) 
{
    string type=string("cfi_exe_nonce=(pos=") +  to_string(bytePos) + ",nv="
        + to_string(nonceVal) + ",sz="+ to_string(nonceSz)+ ")";
    Relocation_t* reloc=create_reloc(insn);
    reloc->setOffset(bytePos);
    reloc->setType(type);
    cout<<"Created reloc='"+type+"' for "<<std::hex<<insn->getBaseID()<<":"<<insn->getDisassembly()<<endl;
}


int SCFI_Instrument::GetNonceBytePos(int nonceSz, int noncePos)
{
    // To get to the nonce, we must pass over noncePos other nonces, which may involve
    // jumping over some exe nonce opcodes, depending on the exe nonce argument size.
    int nonceBytePos =   (((noncePos*nonceSz)/EXE_NONCE_ARG_SIZE)+1)*EXE_NONCE_OPCODE_SIZE
                            + noncePos*nonceSz;
    return nonceBytePos;
}


/* Nonces are just bit strings. To fit them into exe
 * nonces that have a limited argument size, they may need to
 * be split over more than one exe nonce. To be stored as tightly 
 * as possible, more than one nonce may be fit into one exe nonce. 
 * 
 * To handle this, the GetExeNonceFit function returns a list
 * of one or more NonceParts that together specify where in memory 
 * the complete nonce string is located.
 * This function makes some CRITICAL ASSUMPTIONS. Look at the function definition in scfi_instr.hpp */
std::vector<SCFI_Instrument::NoncePart_t> SCFI_Instrument::GetExeNonceFit(NonceValueType_t nonceVal, int nonceSz, int noncePos)
{
    // Checking an assumption
    if(nonceSz > EXE_NONCE_ARG_SIZE)
        assert(nonceSz % EXE_NONCE_ARG_SIZE == 0);
    else
        assert(EXE_NONCE_ARG_SIZE % nonceSz == 0);
    
    // If our nonce will fit into a single exe nonce
    if(EXE_NONCE_ARG_SIZE >= nonceSz)
    {
        // To get to our nonce, we must pass over noncePos other nonces, which may involve
        // jumping over some exe nonce opcodes, depending on the exe nonce argument size.
        int nonceBytePos =   GetNonceBytePos(nonceSz, noncePos);
        std::vector<NoncePart_t> noncePartList;
        NoncePart_t nonce = {nonceVal, nonceBytePos, nonceSz};
        noncePartList.push_back(nonce);
        return noncePartList;
    } 
    else //Otherwise, our nonce will have to be split over multiple exe nonces
    {
        // We will be dividing the nonce into nonceSz/EXE_NONCE_ARG_SIZE nonce parts,
        // and each part corresponds to one entire exe nonce.
         
        std::vector<NoncePart_t> noncePartList;
        for(int exeNonceIndex=0; exeNonceIndex < nonceSz/EXE_NONCE_ARG_SIZE; exeNonceIndex++)
        {
            // Calculate our nonce part info. The idea is to treat
            // the nonce part as if it were a complete nonce in a scenario where
            // nonce size = exe nonce arg size. This requires recalculation of the
            // nonce position.
            
            int noncePartSz = EXE_NONCE_ARG_SIZE;
            int noncePartPos = exeNonceIndex + noncePos*(nonceSz/EXE_NONCE_ARG_SIZE);
            int noncePartBytePos = GetNonceBytePos(noncePartSz, noncePartPos);
    
            NonceValueType_t mask = 0;
	    mask = ~mask;
            NonceValueType_t noncePartVal =  (nonceVal >> exeNonceIndex*EXE_NONCE_ARG_SIZE*8)
                               & (mask >> (8-EXE_NONCE_ARG_SIZE)*8);
 	    cout << "CALCD NONCE BYTE POS AS: " << noncePartBytePos << endl;            
            // Store the calculated nonce part info
            NoncePart_t part = {noncePartVal, noncePartBytePos, noncePartSz};
            noncePartList.push_back(part);
        }
        return noncePartList;
    }
}


// This function supports marking targets for colored 1, 2, 4, or 8 byte exe nonces.
// The exe nonce argument sizes can be 1, 2, 4, or 8 bytes.
void SCFI_Instrument::PlaceExeNonceReloc(Instruction_t* insn, NonceValueType_t nonceVal, int nonceSz, int noncePos)
{   
    // Get the nonce parts our nonce will be divided into to fit into the exe nonces
    std::vector<NoncePart_t> nonceParts = GetExeNonceFit(nonceVal, nonceSz, noncePos);
    // Create exe nonce relocs for each part.
    for(std::vector<NoncePart_t>::iterator it = nonceParts.begin(); it != nonceParts.end(); ++it)
    {
        NoncePart_t theNoncePart = *it;
        // If this nonce part is next to an exe nonce opcode, create a reloc for the opcode too
        bool nextToExeNonceOpCode = !((noncePos*nonceSz) % EXE_NONCE_ARG_SIZE);
        if(nextToExeNonceOpCode)
        {
            CreateExeNonceReloc(insn, EXE_NONCE_OPCODE_VAL, EXE_NONCE_OPCODE_SIZE,
                                      theNoncePart.bytePos-EXE_NONCE_OPCODE_SIZE   );
        }
        // Create an exe nonce reloc for the nonce part.
        CreateExeNonceReloc(insn, theNoncePart.val, theNoncePart.size, theNoncePart.bytePos);
    }
}


/*
 * targ_change_to_push - use the mode in the insnp to create a new instruction that is a push instruction.
 */
static string change_to_push(Instruction_t *insn)
{
	string newbits=insn->getDataBits();

	const auto dp=DecodedInstruction_t::factory(insn);
	const auto &d=*dp;

	int opcode_offset=0;

	opcode_offset=d.getPrefixCount();

	unsigned char modregrm = (newbits[1+opcode_offset]);
	modregrm &= 0xc7;
	modregrm |= 0x30;
	newbits[0+opcode_offset] = 0xFF;
	newbits[1+opcode_offset] = modregrm;

        return newbits;
}


void SCFI_Instrument::mov_reloc(Instruction_t* from, Instruction_t* to, string type )
{
	// need to copy because moveReloc will destroy the iterator used for copying 
	auto copy_of_relocs=from->getRelocations();
	for(auto reloc : copy_of_relocs)
	{

		if(reloc->getType()==type)
		{
			firp->moveRelocation(reloc,from,to);
			// odd standards-conforming way to delete object while iterating.
			//to->getRelocations().insert(reloc);	
			//from->getRelocations().erase(it++);	
		}
	}
		
}


void SCFI_Instrument::move_relocs(Instruction_t* from, Instruction_t* to)
{
	auto copy_of_relocs=from->getRelocations();
	for(auto reloc : copy_of_relocs)
	{
		if(reloc->getType()!="fix_call_fallthrough")
		{
			firp->moveRelocation(reloc,from,to);
			// to->getRelocations().insert(reloc);
			// from->getRelocations().erase(current);
		}
	}
}

void SCFI_Instrument::AddJumpCFI(Instruction_t* insn)
{
	assert(do_rets);

	string pushbits=change_to_push(insn);
	cout<<"Converting ' "<<insn->getDisassembly()<<"' to '";
	Instruction_t* after=insertDataBitsBefore(insn,pushbits); 
	move_relocs(after,insn);
	
	after->setDataBits(getRetDataBits());
	cout <<insn->getDisassembly()<<" + ret' "<<endl ;

	// move any pc-rel relocation bits to the push, which will access memory now 
	mov_reloc(after,insn,"pcrel");
	
	if(do_exe_nonce_for_call)
        {
            // This may be inefficient b/c jumps will not normally be targeting 
            // return targets. However, that does happen in multimodule returns
            // so this is needed.
            AddReturnCFIForExeNonce(after);
        }
        else
        {
            AddReturnCFI(after);
        }
	// cout<<"Warning, JUMPS not CFI's yet"<<endl;
	return;
}


void SCFI_Instrument::AddCallCFIWithExeNonce(Instruction_t* insn)
{
	string reg="ecx";       // 32-bit reg 
        if(firp->getArchitectureBitWidth()==64)
                reg="r11";      // 64-bit reg.

        Instruction_t* call=NULL, *stub=NULL;


        // convert a call indtarg to:
	//      push indtarg  ;Calculate target if it has a memory operation
        //      pop reg       ;Calculate it before changing stack ptr with call stub
	//	call stub     ;Easy way to get correct return address on stack
	//
        //stub: cmp <nonce size> PTR [ecx-<nonce size>], Nonce
        //      jne slow
	//	jmp reg          
 

        // insert the pop/checking code.
	string pushbits=change_to_push(insn);
   call=insertDataBitsBefore(insn, pushbits);
        insertAssemblyAfter(insn,string("pop ")+reg);

	// keep any relocs on the push instruction, as those may need updating.
	insn->setRelocations(call->getRelocations());
	call->setRelocations({});

       	// Jump to non-exe nonce check code
	stub = addNewAssembly(string("push ")+reg);
    	Instruction_t* ret = insertDataBitsAfter(stub, getRetDataBits());
    	ret->setIBTargets(call->getIBTargets());
	
        if(do_exe_nonce_for_call)
        {
            // This may be inefficient b/c jumps will not normally be targeting 
            // return targets. However, that does happen in multimodule returns
            // so this is needed.
            AddReturnCFIForExeNonce(ret);
        }
        else
        {
            AddReturnCFI(ret);
        }	
 
	// convert the indirct call to a direct call to the stub.
        string call_bits=call->getDataBits();
        call_bits.resize(5);
        call_bits[0]=0xe8;
        call->setTarget(stub);
        call->setDataBits(call_bits);
        call->setComment("Direct call to cfi stub");
        call->setIBTargets(NULL);       // lose info about branch targets.

	return;
}

void SCFI_Instrument::AddExecutableNonce(Instruction_t* insn)
{
// this is now done by the nonce plugin's PlopDollopEntry routine.
//	insertDataBitsAfter(firp, insn, ExecutableNonceValue, NULL);
}

// Returns the insn starting a non-exe nonce check for an insn, in case the exe nonce check fails
Instruction_t* SCFI_Instrument::GetExeNonceSlowPath(Instruction_t* insn)
{
    // Jump to non-exe nonce check code
    Instruction_t* ret = addNewDataBits(getRetDataBits());
    ret->setIBTargets(insn->getIBTargets()); 
    AddReturnCFI(ret);
    return ret;
}


// This function supports nonce checks for 1, 2, 4, or 8 byte nonces
// with 1, 2, 4, or 4 byte exe nonce argument sizes.
// TODO: Support 8 byte exe nonce arg sizes
void SCFI_Instrument::InsertExeNonceComparisons(Instruction_t* insn, 
        NonceValueType_t nonceVal, int nonceSz, int noncePos, 
        Instruction_t* exeNonceSlowPath)
{
    
    int noncePartSz; 
    if(nonceSz > EXE_NONCE_ARG_SIZE)
        noncePartSz = EXE_NONCE_ARG_SIZE;
    else
        noncePartSz = nonceSz;
    
    string reg="ecx";	// 32-bit reg 
    if(firp->getArchitectureBitWidth()==64)
        reg="r11";	// 64-bit reg.
    
    string decoration="";
    switch(noncePartSz)
    {
        case 8:
            cerr<<"Cannot handle nonce part of size "<<std::dec<<nonceSz<<endl;
            assert(0);
	    break;
        case 4: 
            decoration="dword ";
            break;
        case 2:	// handle later
            decoration="word ";
            break;
        case 1: //handle later
            decoration="byte ";
            break;
	default:
            cerr<<"Cannot handle nonce of size "<<std::dec<<nonceSz<<endl;
            assert(0);		
    }
    
    // Get the nonce parts our nonce will be divided into to fit into the exe nonces
    std::vector<NoncePart_t> nonceParts = GetExeNonceFit(nonceVal, nonceSz, noncePos);
    // Create exe nonce comparisons for each part.
    Instruction_t* tmp = insn;
    Instruction_t* jne=NULL;
    for(std::vector<NoncePart_t>::iterator it = nonceParts.begin(); it != nonceParts.end(); ++it)
    {
        NoncePart_t theNoncePart = *it;
	cout << "ADDING CMP FOR BYTE POS: " << theNoncePart.bytePos << "FOR NONCE WITH POSITION: " << noncePos << "AND SIZE: " << nonceSz << endl;
        tmp=insertAssemblyAfter(tmp,string("cmp ")+decoration+
		" ["+reg+"+"+to_string(theNoncePart.bytePos)+"], "+to_string(theNoncePart.val));
        jne=tmp=insertAssemblyAfter(tmp,"jne 0");
        jne->setTarget(exeNonceSlowPath);
    }
}


void SCFI_Instrument::AddReturnCFIForExeNonce(Instruction_t* insn, ColoredSlotValue_t *v)
{

	if(!do_rets)
		return;

	string reg="ecx";	// 32-bit reg 
	if(firp->getArchitectureBitWidth()==64)
		reg="r11";	// 64-bit reg.

	string rspreg="esp";	// 32-bit reg 
	if(firp->getArchitectureBitWidth()==64)
		rspreg="rsp";	// 64-bit reg.

	string worddec="dword";	// 32-bit reg 
	if(firp->getArchitectureBitWidth()==64)
		worddec="qword";	// 64-bit reg.

	
	const auto dp=DecodedInstruction_t::factory(insn);
	const auto &d=*dp;

	if(d.hasOperand(0)) 
	{
		unsigned int sp_adjust=d.getImmediate() -firp->getArchitectureBitWidth()/8;
		cout<<"Found relatively rare ret_with_pop insn: "<<d.getDisassembly() <<endl;
		char buf[30];
		sprintf(buf, "pop %s [%s+%d]", worddec.c_str(), rspreg.c_str(), sp_adjust);
		Instruction_t* newafter=insertAssemblyBefore(insn,buf);

		if(sp_adjust>0)
		{
			sprintf(buf, "lea %s, [%s+%d]", rspreg.c_str(), rspreg.c_str(), sp_adjust);
		}

		// rewrite the "old" isntruction, as that's what insertAssemblyBefore returns
		insn=newafter;
		//Needed b/c AddReturnCFI may be called on this ret insn
                //But also clean up and change ret_with_pop to ret (as it should be now) to be safe
                setInstructionAssembly(insn, string("ret"), insn->getFallthrough(), insn->getTarget()); 
	}
        //TODO: Handle uncommon slow path
		
	//TODO: Fix possible TOCTOU race condition? (see Abadi CFI paper)
	//	Ret address can be changed between nonce check and ret insn execution (in theory)
	int nonce_size=GetExeNonceSize(insn);
	int nonce_pos=GetExeNonceOffset(insn);
	NonceValueType_t nonce=GetExeNonce(insn);
        Instruction_t* exeNonceSlowPath = GetExeNonceSlowPath(insn);
	

	// convert a return to:
	// 	mov ecx, [esp]
	// 	cmp <nonce size> PTR [ecx+<offset>], Nonce
	// 	jne exit_node	; no need for slow path here			
	// 	ret             ; ignore race condition for now

	// insert the mov/checking code.
	insertAssemblyBefore(insn,string("mov ")+reg+string(", [")+rspreg+string("]"));
        InsertExeNonceComparisons(insn, nonce, nonce_size, nonce_pos, exeNonceSlowPath);
       	// leave the ret instruction (risk of successful race condition exploit << performance benefit)

	return;
	
}

void SCFI_Instrument::AddReturnCFI(Instruction_t* insn, ColoredSlotValue_t *v)
{

	if(!do_rets)
		return;
	ColoredSlotValue_t v2; 
	if(v==NULL && color_map)
	{
		v2=color_map->GetColorOfIB(insn);
		v=&v2;
	}


	string reg="ecx";	// 32-bit reg 
	if(firp->getArchitectureBitWidth()==64)
		reg="r11";	// 64-bit reg.

	string rspreg="esp";	// 32-bit reg 
	if(firp->getArchitectureBitWidth()==64)
		rspreg="rsp";	// 64-bit reg.

	string worddec="dword";	// 32-bit reg 
	if(firp->getArchitectureBitWidth()==64)
		worddec="qword";	// 64-bit reg.

	const auto dp=DecodedInstruction_t::factory(insn);
	const auto &d=*dp;
	if(d.hasOperand(0)) // d.Argument1.ArgType!=NO_ARGUMENT)
	{
		unsigned int sp_adjust=d.getImmediate() /* Instruction.Immediat*/-firp->getArchitectureBitWidth()/8;
		cout<<"Found relatively rare ret_with_pop insn: "<<d.getDisassembly()<<endl;
		char buf[30];
		sprintf(buf, "pop %s [%s+%d]", worddec.c_str(), rspreg.c_str(), sp_adjust);
		Instruction_t* newafter=insertAssemblyBefore(insn,buf);

		if(sp_adjust>0)
		{
			sprintf(buf, "lea %s, [%s+%d]", rspreg.c_str(), rspreg.c_str(), sp_adjust);
		}

		// rewrite the "old" isntruction, as that's what insertAssemblyBefore returns
		insn=newafter;
		//Clean up and change ret_with_pop to ret (as it should be now) to be safe
                setInstructionAssembly(insn, string("ret"), insn->getFallthrough(), insn->getTarget()); 
	}
		
	int size=1;
	//int position=0;
	string slow_cfi_path_reloc_string;
	if(do_coloring && !do_common_slow_path)
	{
		slow_cfi_path_reloc_string="slow_cfi_path=(pos=-1,nv=244,sz=1)";
		if( v && v->IsValid())
		{
			slow_cfi_path_reloc_string="slow_cfi_path=(pos=-"+ to_string(v->GetPosition()+1) +",nv="
						  + to_string(v->GetNonceValue())+",sz="+ to_string(size) +")";
			size=v->GetPosition();
		}
	}
	else
	{
		slow_cfi_path_reloc_string="slow_cfi_path";
	}

	
	cout<<"Cal'd slow-path cfi reloc as: "<<slow_cfi_path_reloc_string<<endl;
// fixme:  would like to mark a slow path per nonce type using the variables calc'd above.
	
	


	string decoration="";
	int nonce_size=GetNonceSize(insn);
	int nonce_offset=GetNonceOffset(insn);
	NonceValueType_t nonce=GetNonce(insn);
	Instruction_t* jne=NULL, *tmp=NULL;
	

	// convert a return to:
	// 	pop ecx
	// 	cmp <nonce size> PTR [ecx-<nonce size>], Nonce
	// 	jne slow	; reloc such that strata/zipr can convert slow to new code
	//			; to handle places where nonce's can't be placed. 
	// 	jmp ecx

	switch(nonce_size)
	{
		case 1:
                    decoration="byte ";
                    break;
		case 2:	// handle later
                    decoration="word ";
                    break;
		case 4: // handle later
                    decoration="dword ";
                    break;
                case 8:
                    decoration="dword "; // 8 byte nonces will be split into 2 compares
                    break;
		default:
			cerr<<"Cannot handle nonce of size "<<std::dec<<nonce_size<<endl;
			assert(0);
		
	}

	// insert the pop/checking code.
	insertAssemblyBefore(insn,string("pop ")+reg);
        if(nonce_size != 8)
        {
            tmp=insertAssemblyAfter(insn,string("cmp ")+decoration+
		" ["+reg+"-"+to_string(nonce_offset)+"], "+to_string(nonce));
        }
        else
        {
            tmp=insertAssemblyAfter(insn,string("cmp ")+decoration+
		" ["+reg+"-"+to_string(nonce_offset)+"], "+to_string((uint32_t) nonce)); // upper 32 bits chopped off by cast
            jne=tmp=insertAssemblyAfter(tmp,"jne 0");
            tmp=insertAssemblyAfter(tmp,string("cmp ")+decoration+
		" ["+reg+"-"+to_string(nonce_offset - 4)+"], "+to_string((uint32_t) (nonce >> 32)));
            // set the jne's target to itself, and create a reloc that zipr/strata will have to resolve.
            jne->setTarget(jne);	// needed so spri/spasm/irdb don't freak out about missing target for new insn.
            Relocation_t* reloc=create_reloc(jne);
            reloc->setType(slow_cfi_path_reloc_string); 
            reloc->setOffset(0);
            cout<<"Setting slow path for: "<<slow_cfi_path_reloc_string<<endl;
        }
        
        jne=tmp=insertAssemblyAfter(tmp,"jne 0");
	// convert the ret instruction to a jmp ecx
	cout<<"Converting "<<hex<<tmp->getFallthrough()->getBaseID()<<":"<<tmp->getFallthrough()->getDisassembly()<<"to jmp+reg"<<endl;
	setInstructionAssembly(tmp->getFallthrough(), string("jmp ")+reg, NULL,NULL);

	// set the jne's target to itself, and create a reloc that zipr/strata will have to resolve.
	jne->setTarget(jne);	// needed so spri/spasm/irdb don't freak out about missing target for new insn.
	Relocation_t* reloc=create_reloc(jne);
	reloc->setType(slow_cfi_path_reloc_string); 
	reloc->setOffset(0);
	cout<<"Setting slow path for: "<<slow_cfi_path_reloc_string<<endl;
	
	return;
}

static void display_histogram(std::ostream& out, std::string attr_label, std::map<int,int> & p_map)
{
	if (p_map.size()) 
	{
		out<<"# ATTRIBUTE Selective_Control_Flow_Integrity::" << attr_label << "=";
		out<<"{ibt_size:count,";
		bool first_time=true;
		for (map<int,int>::iterator it = p_map.begin(); 
			it != p_map.end(); ++it)
		{
			if (!first_time)
				out << ",";
			out << it->first << ":" << it->second;
			first_time = false;
		}
		out<<"}"<<endl;
	}
}

bool SCFI_Instrument::is_plt_style_jmp(Instruction_t* insn) 
{
	const auto d=DecodedInstruction_t::factory(insn);
	if(d->getOperand(0)->isMemory()) 
	{
		if(!d->getOperand(0)->hasBaseRegister() && !d->getOperand(0)->hasIndexRegister() )  
			return true;
		return false;
	}
	return false;
}

bool SCFI_Instrument::is_jmp_a_fixed_call(Instruction_t* insn) 
{
	if(preds[insn].size()!=1)
		return false;

	Instruction_t* pred=*(preds[insn].begin());
	assert(pred);

	if(pred->getDataBits()[0]==0x68)
		return true;
	return false;
}

bool SCFI_Instrument::instrument_jumps() 
{
	int cfi_checks=0;
	int cfi_branch_jmp_checks=0;
	int cfi_branch_jmp_complete=0;
	int cfi_branch_call_checks=0;
	int cfi_branch_call_complete=0;
	int cfi_branch_ret_checks=0;
	int cfi_branch_ret_complete=0;
	int cfi_safefn_jmp_skipped=0;
	int cfi_safefn_ret_skipped=0;
	int cfi_safefn_call_skipped=0;
	int ibt_complete=0;
	double cfi_branch_jmp_complete_ratio = NAN;
	double cfi_branch_call_complete_ratio = NAN;
	double cfi_branch_ret_complete_ratio = NAN;

	std::map<int, int> calls;
	std::map<int, int> jmps;
	std::map<int, int> rets;

	// build histogram of target sizes

	// for each instruction
	// But make sure the new insns added in this loop are not processed in this loop
        // (ok since none of the them should need to be protected)
        auto insn_set = firp->getInstructions();
	for(auto insn : insn_set)
	{

		// we always have to protect the zestcfi dispatcher, that we just added.
		if(zestcfi_function_entry==insn)
		{
			cout<<"Protecting zestcfi function for external entrances"<<endl;
			cfi_checks++;
			AddJumpCFI(insn);
			continue;
		}

		if(insn->getBaseID()==BaseObj_t::NOT_IN_DATABASE)
			continue;

		const auto dp=DecodedInstruction_t::factory(insn);
		const auto &d=*dp;

		if (insn->getFunction())
			cerr<<"Looking at: "<<insn->getDisassembly()<< " from func: " << insn->getFunction()->getName() << endl;
		else
			cerr<<"Looking at: "<<insn->getDisassembly()<< " but no associated function" << endl;

		if(d.isCall()  && (protect_safefn && !do_exe_nonce_for_call))
		{
                	cerr<<"Fatal Error: Found call instruction!"<<endl;
                	cerr<<"FIX_CALLS_FIX_ALL_CALLS=1 should be set in the environment, or"<<endl;
                	cerr<<"--step-option fix_calls:--fix-all should be passed to ps_analyze."<<endl;
			exit(1);
		}

		// if marked safe
		if(FindRelocation(insn,"cf::safe"))
			continue;

		bool safefn = isSafeFunction(insn);

		if (safefn) {
			if (insn->getFunction())
				cerr << insn->getFunction()->getName() << " is safe" << endl;
		}

	
		
		if(d.isUnconditionalBranch())
		{
			if(!d.getOperand(0)->isConstant()) 
			{
				bool is_fixed_call=is_jmp_a_fixed_call(insn);
				bool is_plt_style=is_plt_style_jmp(insn);
				bool is_any_call_style = (is_fixed_call || is_plt_style);
				if(do_jumps && !is_any_call_style)
				{
					if (insn->getIBTargets() && insn->getIBTargets()->isComplete())
					{
						cfi_branch_jmp_complete++;
						jmps[insn->getIBTargets()->size()]++;
					}

					cfi_checks++;
					cfi_branch_jmp_checks++;

					AddJumpCFI(insn);
				}
				else if(do_calls && is_any_call_style)
				{
					if (insn->getIBTargets() && insn->getIBTargets()->isComplete())
					{
						cfi_branch_call_complete++;
						calls[insn->getIBTargets()->size()]++;
					}

					cfi_checks++;
					cfi_branch_call_checks++;
					
					AddJumpCFI(insn);
				}
				else 
				{	
					cout<<"Eliding protection for "<<insn->getDisassembly()<<std::boolalpha
						<<" is_fixed_call="<<is_fixed_call
						<<" is_plt_style="<<is_plt_style
						<<" is_any_call_style="<<is_any_call_style
						<<" do_jumps="<<do_jumps
						<<" do_calls="<<do_calls<<endl;
				}
			}
		}
		else if(d.isCall())
		{

			// should only see calls if we are not CFI'ing safe functions
			// be sure to use with: --no-fix-safefn in fixcalls
			//    (1) --no-fix-safefn in fixcalls leaves call as call (instead of push/jmp)
			//    (2) and here, we don't plop down a nonce
			//    see (3) below where we don't instrument returns for safe functions
			bool isDirectCall = d.getOperand(0)->isConstant();
			if (!protect_safefn)
			{

				if (safefn || (isDirectCall && isCallToSafeFunction(insn)))
				{
					cfi_safefn_call_skipped++;
					continue;
				}
			}

			AddExecutableNonce(insn);	// for all calls
			if(!isDirectCall) 
			{
				// for indirect calls.
				AddCallCFIWithExeNonce(insn);
			}
		}
		else if (d.isReturn()) 
		{
			if (insn->getFunction())
				cerr << "found ret type  protect_safefn: " << protect_safefn << "  safefn: " << safefn <<  " function: " << insn->getFunction()->getName() << endl;
			else
				cerr << "found ret type  protect_safefn: " << protect_safefn << "  safefn: " << safefn << " no functions associated with instruction!! wtf???" << endl;
			if (insn->getIBTargets() && insn->getIBTargets()->isComplete())
			{
				cfi_branch_ret_complete++;
				rets[insn->getIBTargets()->size()]++;
			}

			// (3) and here, we don't instrument returns for safe function
			if (!protect_safefn && safefn)
			{
				cfi_safefn_ret_skipped++;
				continue;
			}

			cfi_checks++;
			cfi_branch_ret_checks++;

			if(do_exe_nonce_for_call) {
				AddReturnCFIForExeNonce(insn);
			}
			else {
				AddReturnCFI(insn);
			}
		}
		else 
		{
		}
	}
	
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_jmp_checks="<<std::dec<<cfi_branch_jmp_checks<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_jmp_complete="<<cfi_branch_jmp_complete<<endl;
	display_histogram(cout, "cfi_jmp_complete_histogram", jmps);

	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_branch_call_checks="<<std::dec<<cfi_branch_call_checks<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_branch_call_complete="<<std::dec<<cfi_branch_call_complete<<endl;
	display_histogram(cout, "cfi_call_complete_histogram", calls);

	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_ret_checks="<<std::dec<<cfi_branch_ret_checks<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_ret_complete="<<std::dec<<cfi_branch_ret_complete<<endl;
	display_histogram(cout, "cfi_ret_complete_histogram", rets);

	assert(getenv("SELF_VALIDATE")==nullptr || cfi_branch_call_checks> 2);
	assert(getenv("SELF_VALIDATE")==nullptr || cfi_branch_call_checks> 2);

	// 0 or 1 checks.
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::multimodule_checks="<< (unsigned int)(zestcfi_function_entry!=NULL) <<endl;

	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_checks="<<std::dec<<cfi_checks<<endl;
	ibt_complete = cfi_branch_jmp_complete + cfi_branch_call_complete + cfi_branch_ret_complete;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::ibt_complete="<<std::dec<<ibt_complete<<endl;

	if (cfi_branch_jmp_checks > 0) 
		cfi_branch_jmp_complete_ratio = (double)cfi_branch_jmp_complete / cfi_branch_jmp_checks;

	if (cfi_branch_call_checks > 0) 
		cfi_branch_call_complete_ratio = (double)cfi_branch_call_complete / cfi_branch_call_checks;

	if (cfi_branch_ret_checks > 0) 
		cfi_branch_ret_complete_ratio = (double)cfi_branch_ret_complete / cfi_branch_ret_checks;

	//double cfi_branch_complete_ratio = NAN;
	//if (ibt_complete > 0)
	//	cfi_branch_complete_ratio = (double) cfi_checks / ibt_complete;

	

	cout << "# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_jmp_complete_ratio=" << cfi_branch_jmp_complete_ratio*100.00<<"%" << endl;
	cout << "# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_call_complete_ratio=" << cfi_branch_call_complete_ratio*100.00<<"%" << endl;
	cout << "# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_ret_complete_ratio=" << cfi_branch_ret_complete_ratio*100.00<<"%" << endl;
	cout << "# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_complete_ratio=" << cfi_branch_ret_complete_ratio*100.00<<"%" << endl;

	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_safefn_jmp_skipped="<<cfi_safefn_jmp_skipped<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_safefn_ret_skipped="<<cfi_safefn_ret_skipped<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::cfi_safefn_call_skipped="<<cfi_safefn_call_skipped<<endl;

	return true;
}

// use this to determine whether a scoop has a given name.
static struct ScoopFinder : binary_function<const DataScoop_t*,const string,bool>
{
	// declare a simple scoop finder function that finds scoops by name
	bool operator()(const DataScoop_t* scoop, const string& name) const
	{
		return (scoop->getName() == name);
	};
} finder;

static DataScoop_t* find_scoop(FileIR_t *firp,const string &name)
{
	auto it=find_if(firp->getDataScoops().begin(), firp->getDataScoops().end(), bind2nd(finder, name)) ;
	if( it != firp->getDataScoops().end() )
		return *it;
	return NULL;
};

static unsigned int  add_to_scoop(const string &str, DataScoop_t* scoop) 
{
	// assert that this scoop is unpinned.  may need to enable --step move_globals --step-option move_globals:--elftables-only
	assert(scoop->getStart()->getVirtualOffset()==0);
	int len=str.length();
	scoop->setContents(scoop->getContents()+str);
	auto oldend=scoop->getEnd()->getVirtualOffset();
	auto newend=oldend+len;
	scoop->getEnd()->setVirtualOffset(newend);
	return oldend+1;
};

template<int ptrsize>
static void insert_into_scoop_at(const string &str, DataScoop_t* scoop, FileIR_t* firp, const unsigned int at) 
{
	// assert that this scoop is unpinned.  may need to enable --step move_globals --step-option move_globals:--cfi
	assert(scoop->getStart()->getVirtualOffset()==0);
	int len=str.length();
	string new_scoop_contents=scoop->getContents();
	new_scoop_contents.insert(at,str);
	scoop->setContents(new_scoop_contents);

	auto oldend=scoop->getEnd()->getVirtualOffset();
	auto newend=oldend+len;
	scoop->getEnd()->setVirtualOffset(newend);

	// update each reloc to point to the new location.
	for(auto reloc : scoop->getRelocations())
	{
		if((unsigned int)reloc->getOffset()>=at)
			reloc->setOffset(reloc->getOffset()+str.size());
		
	};

	// check relocations for pointers to this object.
	// we'll update dataptr_to_scoop relocs, but nothing else
	// so assert if we find something else
	for(auto reloc : firp->getRelocations())
	{
		auto wrt=dynamic_cast<DataScoop_t*>(reloc->getWRT());
		assert(wrt != scoop || reloc->getType()=="dataptr_to_scoop");
	};

	// for each scoop
	for(auto scoop_to_update : firp->getDataScoops())
	{
		// for each relocation for that scoop
		for(auto reloc : scoop_to_update->getRelocations())
		{
			// if it's a reloc that's wrt scoop
			auto wrt=dynamic_cast<DataScoop_t*>(reloc->getWRT());
			if(wrt==scoop)
			{
				// then we need to update the scoop
				if(reloc->getType()=="dataptr_to_scoop")
				{
					string contents=scoop_to_update->getContents();
					// subtract the stringsize from the (implicitly stored) addend
					// taking pointer size into account.
					switch(ptrsize)
					{
						case 4:
						{
							unsigned int val=*((unsigned int*)&contents.c_str()[reloc->getOffset()]); 
							if(val>=at)
								val +=str.size();
							contents.replace(reloc->getOffset(), ptrsize, (const char*)&val, ptrsize);
							break;
						
						}
						case 8:
						{
							unsigned long long val=*((long long*)&contents.c_str()[reloc->getOffset()]); 
							if(val>=at)
								val +=str.size();
							contents.replace(reloc->getOffset(), ptrsize, (const char*)&val, ptrsize);
							break;

						}
						default: 
							assert(0);
					}
					scoop_to_update->setContents(contents);
				}
			}	

		};
		
	};
};

template<int ptrsize>
static void prefix_scoop(const string &str, DataScoop_t* scoop, FileIR_t* firp) 
{
	insert_into_scoop_at<ptrsize>(str,scoop,firp,0);
};


template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
bool SCFI_Instrument::add_dl_support()
{
	bool success=true;
	success = success &&  add_libdl_as_needed_support<T_Elf_Sym,T_Elf_Rela, T_Elf_Dyn, rela_shift, reloc_type, ptrsize>();
	success = success &&  add_got_entries<T_Elf_Sym,T_Elf_Rela, T_Elf_Dyn, reloc_type, rela_shift, ptrsize>();

	return success;
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
Instruction_t* SCFI_Instrument::find_runtime_resolve(DataScoop_t* gotplt_scoop)
{
	// find any data_to_insn_ptr reloc for the gotplt scoop
	auto it=find_if(gotplt_scoop->getRelocations().begin(), gotplt_scoop->getRelocations().end(), [](Relocation_t* reloc)
	{
		return reloc->getType()=="data_to_insn_ptr";
	});
	// there _should_ be one.
	assert(it!=gotplt_scoop->getRelocations().end());

	Relocation_t* reloc=*it;
	Instruction_t* wrt=dynamic_cast<Instruction_t*>(reloc->getWRT());
	assert(wrt);	// should be a WRT
	assert(wrt->getDisassembly().find("push ") != string::npos);	// should be push K insn
	return wrt->getFallthrough();	// jump to the jump, or not.. doesn't matter.  zopt will fix
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
void SCFI_Instrument::add_got_entry(const std::string& name)
{
	// find relevant scoops
	auto dynamic_scoop=find_scoop(firp,".dynamic");
	auto dynstr_scoop=find_scoop(firp,".dynstr");
	auto dynsym_scoop=find_scoop(firp,".dynsym");
	auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");
	auto relscoop=relaplt_scoop!=NULL ?  relaplt_scoop : relplt_scoop;
	auto gnu_version_scoop=find_scoop(firp,".gnu.version");

	// if versioning info is avail in this binary, assert that we unpinned it.
	if(gnu_version_scoop)
		assert(gnu_version_scoop->getStart()->getVirtualOffset()==0);

	// add 0-init'd pointer to table
	string new_got_entry_str(ptrsize,0);	 // zero-init a pointer-sized string


	// create a new, unpinned, rw+relro scoop that's an empty pointer.
	const auto start_addr=firp->addNewAddress(firp->getFile()->getBaseID(), 0);
	const auto end_addr=firp->addNewAddress(firp->getFile()->getBaseID(), ptrsize-1);
	auto external_func_addr_scoop=firp->addNewDataScoop(name, start_addr,end_addr, NULL, 6, true, new_got_entry_str);

	// add string to string table 
	const auto dl_str_pos=add_to_scoop(name+'\0', dynstr_scoop);

	// add symbol to dlsym
	T_Elf_Sym dl_sym;
	memset(&dl_sym,0,sizeof(T_Elf_Sym));
	dl_sym.st_name=dl_str_pos;
	dl_sym.st_info=((STB_GLOBAL<<4)| (STT_OBJECT));
	string dl_sym_str((const char*)&dl_sym, sizeof(T_Elf_Sym));
	unsigned int dl_pos=add_to_scoop(dl_sym_str,dynsym_scoop);

	// if versioning info is avail in this binary, we are required to add a new entry for the new symbol
	if(gnu_version_scoop)
	{
		// update the gnu.version section so that the new symbol has a version.
		const auto new_version_str=string("\0\0", 2);	 // \0\0 means *local*, as in, don't index the gnu.verneeded array.
		add_to_scoop(new_version_str,gnu_version_scoop);
	}

	// find the rela count.  can't insert before that.
	int rela_count=0;
	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->getSize(); i+=sizeof(T_Elf_Dyn))
	{
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->getContents().c_str()[i];
		if(dyn_entry.d_tag==DT_RELACOUNT)	 // diff than rela size.
		{
			// add to the size
			rela_count=dyn_entry.d_un.d_val;
			break;
		}
	}

	// create the new reloc 
	T_Elf_Rela dl_rel;
	memset(&dl_rel,0,sizeof(dl_rel));
	auto r_info_tmp=decltype(dl_rel.r_info)(dl_pos);
	dl_rel.r_info= ((r_info_tmp/sizeof(T_Elf_Sym))<<rela_shift) | reloc_type;
	string dl_rel_str((const char*)&dl_rel, sizeof(dl_rel));

// need to fixup relocs
	unsigned int at=rela_count*sizeof(T_Elf_Rela);
	insert_into_scoop_at<ptrsize>(dl_rel_str, relscoop, firp, at);

	/*
	 * Relocation_t* dl_reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE,  at+((uintptr_t)&dl_rel.r_offset -(uintptr_t)&dl_rel), "dataptr_to_scoop", external_func_addr_scoop);
	relscoop->getRelocations().insert(dl_reloc);
	firp->getRelocations().insert(dl_reloc);
	*/
	auto dl_reloc=firp->addNewRelocation(relscoop,at+((uintptr_t)&dl_rel.r_offset -(uintptr_t)&dl_rel), "dataptr_to_scoop", external_func_addr_scoop);
	(void)dl_reloc;
	
	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->getSize(); i+=sizeof(T_Elf_Dyn))
	{
		// cast the index'd c_str to an Elf_Dyn pointer and deref it to assign to a 
		// reference structure.  That way editing the structure directly edits the string.
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->getContents().c_str()[i];
		if(dyn_entry.d_tag==DT_RELASZ)
			// add to the size
			dyn_entry.d_un.d_val+=sizeof(T_Elf_Rela);

		// we insert the zest_cfi_dispatch symbol after the relative relocs.
		// but we need to adjust the start if there are no relative relocs.
		if(at == 0  && dyn_entry.d_tag==DT_RELA)
			// subtract from the start.
			dyn_entry.d_un.d_val-=sizeof(T_Elf_Rela);

	}
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
bool SCFI_Instrument::add_got_entries()
{

	// find all the necessary scoops;
	const auto dynamic_scoop=find_scoop(firp,".dynamic");
	//const auto gotplt_scoop=find_scoop(firp,".got.plt");
	//const auto got_scoop=find_scoop(firp,".got");
	const auto dynstr_scoop=find_scoop(firp,".dynstr");
	const auto dynsym_scoop=find_scoop(firp,".dynsym");
	//const auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	//const auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");
	//const auto relscoop=relaplt_scoop!=NULL ?  relaplt_scoop : relplt_scoop;
	//const auto search_in = gotplt_scoop==NULL ? got_scoop : gotplt_scoop;
	//const auto to_dl_runtime_resolve=find_runtime_resolve<T_Elf_Sym,T_Elf_Rela, T_Elf_Dyn, rela_shift, reloc_type, ptrsize>(search_in);


	// add necessary GOT entries.
	add_got_entry<T_Elf_Sym,T_Elf_Rela,T_Elf_Dyn,reloc_type,rela_shift,ptrsize>("zest_cfi_dispatch");


	// also add a zest cfi "function" that's exported so dlsym can find it.
	auto zestcfi_str_pos=add_to_scoop(string("zestcfi")+'\0', dynstr_scoop);

	// add zestcfi symbol to binary
	T_Elf_Sym zestcfi_sym;
	memset(&zestcfi_sym,0,sizeof(T_Elf_Sym));
	zestcfi_sym.st_name=zestcfi_str_pos;
	zestcfi_sym.st_size=1234;
	zestcfi_sym.st_info=((STB_GLOBAL<<4)| (STT_FUNC));
	string zestcfi_sym_str((const char*)&zestcfi_sym, sizeof(T_Elf_Sym));
	unsigned int zestcfi_pos=add_to_scoop(zestcfi_sym_str,dynsym_scoop);

	// add "function" for zestcfi"
	// for now, return that the target is allowed.  the nonce plugin will have to have a slow path for this later.
	assert(firp->getArchitectureBitWidth()==64); // fixme for 32-bit, should jmp to ecx.
	zestcfi_function_entry=addNewAssembly("jmp r11");

	// this jump can target any IBT in the module.
	// ICFS_t *newicfs=new ICFS_t;
	// firp->GetAllICFS().insert(newicfs);
	auto newicfs=firp->addNewICFS();
	for(auto insn : firp->getInstructions())
	{
		if(insn->getIndirectBranchTargetAddress() != NULL )
			newicfs->insert(insn);
	};
	zestcfi_function_entry->setIBTargets(newicfs);
	firp->assembleRegistry();
	

	// add a relocation so that the zest_cfi "function"  gets pointed to by the symbol
	auto zestcfi_reloc=firp->addNewRelocation(dynsym_scoop,zestcfi_pos+((uintptr_t)&zestcfi_sym.st_value - (uintptr_t)&zestcfi_sym), "data_to_insn_ptr", zestcfi_function_entry);
	(void)zestcfi_reloc;


	// update strtabsz after got/etc entries are added.
	for(int i=0;i+sizeof(T_Elf_Dyn)<dynamic_scoop->getSize(); i+=sizeof(T_Elf_Dyn))
	{
		T_Elf_Dyn &dyn_entry=*(T_Elf_Dyn*)&dynamic_scoop->getContents().c_str()[i];
		if(dyn_entry.d_tag==DT_STRSZ)
		{
			dyn_entry.d_un.d_val=dynstr_scoop->getContents().size();
		}
	}


	return true;
}

template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
bool SCFI_Instrument::add_libdl_as_needed_support()
{
	DataScoopSet_t::iterator it;
	// use this to determine whether a scoop has a given name.

	auto dynamic_scoop=find_scoop(firp,".dynamic");
	auto dynstr_scoop=find_scoop(firp,".dynstr");
	auto dynsym_scoop=find_scoop(firp,".dynsym");
	auto relaplt_scoop=find_scoop(firp,".rela.dyn coalesced w/.rela.plt");
	auto relplt_scoop=find_scoop(firp,".rel.dyn coalesced w/.rel.plt");


	// they all have to be here, or none are.
	assert( (dynamic_scoop==NULL && dynstr_scoop==NULL && dynsym_scoop==NULL ) || 
		(dynamic_scoop!=NULL && dynstr_scoop!=NULL && dynsym_scoop!=NULL )
		);


	// not dynamic executable w/o a .dynamic section.
	if(!dynamic_scoop)
		return true;

	// may need to enable --step move_globals --step-option move_globals:--cfi
	if(relaplt_scoop == NULL && relplt_scoop==NULL)
	{
		cerr<<"Cannot find relocation-scoop pair:  Did you enable '--step move_globals --step-option move_globals:--cfi' ? "<<endl;
		exit(1);
	}

	assert(relaplt_scoop == NULL || relplt_scoop==NULL); // can't have both
	assert(relaplt_scoop != NULL || relplt_scoop!=NULL); // can't have neither


	auto libld_str_pos=add_to_scoop(string("libzestcfi.so")+'\0', dynstr_scoop);


	// a new dt_needed entry for libdl.so
	T_Elf_Dyn new_dynamic_entry;
	memset(&new_dynamic_entry,0,sizeof(new_dynamic_entry));
	new_dynamic_entry.d_tag=DT_NEEDED;
	new_dynamic_entry.d_un.d_val=libld_str_pos;
	string new_dynamic_entry_str((const char*)&new_dynamic_entry, sizeof(T_Elf_Dyn));
	// a null terminator
	T_Elf_Dyn null_dynamic_entry;
	memset(&null_dynamic_entry,0,sizeof(null_dynamic_entry));
	string null_dynamic_entry_str((const char*)&null_dynamic_entry, sizeof(T_Elf_Dyn));

	// declare an entry for the .dynamic section and add it.
	int index=0;
	while(1)
	{
		// assert we don't run off the end.
		assert((index+1)*sizeof(T_Elf_Dyn) <= dynamic_scoop->getContents().size());

		T_Elf_Dyn* dyn_ptr=(T_Elf_Dyn*) & dynamic_scoop->getContents().c_str()[index*sizeof(T_Elf_Dyn)];
	
		if(memcmp(dyn_ptr,&null_dynamic_entry,sizeof(T_Elf_Dyn)) == 0 )
		{
			cout<<"Inserting new DT_NEEDED at index "<<dec<<index<<endl;
			// found a null terminator entry.
			for(unsigned int i=0; i<sizeof(T_Elf_Dyn); i++)
			{
				// copy new_dynamic_entry ontop of null entry.
				auto str=dynamic_scoop->getContents();
				str[index*sizeof(T_Elf_Dyn) + i ] = ((char*)&new_dynamic_entry)[i];
				dynamic_scoop->setContents(str);
			}

			// check if there's room for the new null entry
			if((index+2)*sizeof(T_Elf_Dyn) <= dynamic_scoop->getContents().size())
			{
				/* yes */
				T_Elf_Dyn* next_entry=(T_Elf_Dyn*)&dynamic_scoop->getContents().c_str()[(index+1)*sizeof(T_Elf_Dyn)];
				// assert it's actually null 
				assert(memcmp(next_entry,&null_dynamic_entry,sizeof(T_Elf_Dyn)) == 0 );
			}
			else
			{
				// add to the scoop 
				add_to_scoop(null_dynamic_entry_str,dynamic_scoop);
			}
			break;
		}

		index++;
	}

#if 0
	cout<<".dynamic contents after scfi update:"<<hex<<endl;
	const string &dynstr_contents=dynamic_scoop->getContents();
	for(unsigned int i=0;i<dynstr_contents.size(); i+=16)
	{
		cout<<*(long long*) &dynstr_contents.c_str()[i] <<" "
		    <<*(long long*) &dynstr_contents.c_str()[i+8] <<endl;
	}
#endif

	return true;

}



bool SCFI_Instrument::execute()
{

	bool success=true;

	// this adds an instruction that needs instrumenting by future phases.
	// do not move later.
	if(do_multimodule)
	{
		if(firp->getArchitectureBitWidth()==64)
			success = success && add_dl_support<Elf64_Sym, Elf64_Rela, Elf64_Dyn, R_X86_64_GLOB_DAT, 32, 8>();
		else
			success = success && add_dl_support<Elf32_Sym, Elf32_Rel, Elf32_Dyn, R_386_GLOB_DAT, 8, 4>();
	}


	// this selects colors and is used in instrument jumps.
	// do not move later.
	if(do_coloring)
	{
		color_map.reset(new ColoredInstructionNonces_t(firp, nonce_size)); 
		assert(color_map);
		success = success && color_map->build();
	
	}
        
	if(do_color_exe_nonces)
        {
            // A separate color map is created for exe nonces so that exe and 
            // non-exe colored nonces can be different sizes.
            // Note that these independent color slots will not conflict with
            // the non-exe slots, but ONLY because exe nonces come after a target,
            // while non-exe nonces come before.
            exe_nonce_color_map.reset(new ColoredInstructionNonces_t(firp, exe_nonce_size)); 
            assert(exe_nonce_color_map);
            success = success && exe_nonce_color_map->build();
            // Note that it is in theory possible (but I'm assuming unlikely) that 
            // unnecessary colored exe nonce slots will be added due to 
            // insn's sharing the same IBTs on func ret sites but having 
            // differing IBT's on non-func ret sites. (Because we reuse the non-exe
            // nonce color map, which pays attention to all IBT's and
            // not just the IBT's that target func ret sites, yet we are only putting
            // exe nonces on func ret sites).
        }
        
	success = success && instrument_jumps();	// to handle moving of relocs properly if
							// an insn is both a IBT and a IB,
							// we instrument first, then add relocs for targets

	success = success && mark_targets();		// put relocs on all targets so that the backend can put nonces in place.

	return success;
}


