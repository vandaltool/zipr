
#include <irdb-transform>
#include <libIRDB-core.hpp>
#include <keystone/keystone.h>
#include <capstone.h>
#include <bits/stdc++.h> 

// Copied from PnTransform
// @todo: create a utility library with the one interface


using namespace std;
using namespace IRDB_SDK;

void copyInstruction(Instruction_t* src, Instruction_t* dest);
Instruction_t* copyInstruction(FileIR_t* virp, Instruction_t* instr);
Instruction_t* allocateNewInstruction(FileIR_t* virp, DatabaseID_t p_fileID,Function_t* func);
Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr);
vector<string> assemblegroup(string group);



void setInstructionsDetails(FileIR_t* virp, Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
        if (p_instr == NULL) return;

        p_instr->setDataBits(p_dataBits);
        p_instr->setComment(p_instr->getDisassembly());
        p_instr->setFallthrough(p_fallThrough);
        p_instr->setTarget(p_target);

	auto real_virp=dynamic_cast<libIRDB::FileIR_t*>(virp);
	assert(real_virp);

        real_virp->GetAddresses().insert(p_instr->getAddress());
        real_virp->GetInstructions().insert(p_instr);
}


//For all insertBefore functions:
//The "first" instruction will have its contents replaced and a duplicate of "first" will be in the follow of first. 
//This duplicate is returned since the user already has a pointer to first.
//To insert before an instruction is the same as modifying the original instruction, and inserting after it
//a copy of the original instruction 
Instruction_t* IRDB_SDK::insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t* next = copyInstruction(virp,first);

	//In case the fallthrough is null, generate spri has to have a 
	//place to jump, which is determined by the original address.
	//This code is not placed in copyInstruction since this is only needed
	//when inserting before
	next->setOriginalAddressID(first->getOriginalAddressID());
	//"Null" out the original address (it should be as if the instruction was not in the database).
	first->setOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);
	auto real_first=dynamic_cast<libIRDB::Instruction_t*>(first);
	assert(real_first);
	real_first->GetRelocations().clear();
        first->setIBTargets(NULL);
	//Note that the instruction just inserted should have the same exception handling
        //info as the instructions immediately around it.
        //Thus the exception handling information (EhCallSite and EhProgram) are kept the 
        //same from the copy of first (unlike with relocations and IBT's).

	virp->changeRegistryKey(first,next);
	IRDB_SDK::setInstructionAssembly(virp,first,assembly,next,target);

	return next;
}

vector<Instruction_t*> IRDB_SDK::insertAssemblyInstructionsBefore(FileIR_t* firp, Instruction_t* before, string instructions, Instruction_t *target) {
	vector<string> databits = assemblegroup(instructions);
	vector<Instruction_t*> results;
	int size = databits.size();
	results.push_back(before);
	for(int i = size - 1; i >= 0; i--) {
		Instruction_t* curins;
		if(i == size - 1) {
			curins = insertDataBitsBefore(firp, before, databits[size - 1], target);
		}
		else {
			curins = insertDataBitsBefore(firp, curins, databits[i], target);
		}
		results.push_back(curins);
	}
	reverse(results.begin(), results.end());
	return results;
}

vector<Instruction_t*> IRDB_SDK::insertAssemblyInstructionsAfter(FileIR_t* firp, Instruction_t* after, string instructions, Instruction_t *target) {
        //string newBits = after->assemblegroup(instructions);
        //return insertDataBitsBefore(firp, after, newBits, NULL);
	vector<string> databits = assemblegroup(instructions);
	vector<Instruction_t*> results;
	int size = databits.size();
	results.push_back(after);
	for(int i = 0; i < size; i++) {
		Instruction_t* curins;
		if(i == 0) {
			curins = insertDataBitsAfter(firp, after, databits[0], target);
		}
		else {
			curins = insertDataBitsAfter(firp, curins, databits[i], target);
		}
		results.push_back(curins);
	}
	return results;
}

#if 0
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly)
{
	return insertAssemblyBefore(virp,first,assembly,NULL);
}

vector<Instruction_t*> insertAssemblyInstructionsBefore(FileIR_t* virp, Instruction_t* first, string assemblygroup)
{
	return insertAssemblyInstructionsBefore(virp, first, assemblygroup, NULL)
}

Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits)
{
        return insertDataBitsBefore(virp,first,dataBits,NULL);
}
#endif

Instruction_t* IRDB_SDK::insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
	Instruction_t* next = copyInstruction(virp,first);

        //In case the fallthrough is null, generate spri has to have a 
        //place to jump, which is determined by the original address.
        //This code is not placed in copyInstruction since this is only needed
        //when inserting before
        next->setOriginalAddressID(first->getOriginalAddressID());
        //"Null" out the original address (it should be as if the instruction was not in the database).
        first->setOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);
	auto real_first=dynamic_cast<libIRDB::Instruction_t*>(first);
	assert(real_first);
        real_first->GetRelocations().clear();
	first->setIBTargets(NULL);
	//Note that the instruction just inserted should have the same exception handling
	//info as the instructions immediately around it.
	//Thus the exception handling information (EhCallSite and EhProgram) are kept the 
	//same from the copy of first (unlike with relocations and IBT's).

	virp->changeRegistryKey(first,next);
        setInstructionsDetails(virp,first,dataBits,next,target);

        return next;
}

Instruction_t* IRDB_SDK::insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t *new_instr = allocateNewInstruction(virp,first);
	IRDB_SDK::setInstructionAssembly(virp,new_instr,assembly,first->getFallthrough(), target);
        first->setFallthrough(new_instr);
        return new_instr;
}

#if 0
Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly)
{
        return insertAssemblyAfter(virp,first,assembly,NULL);

}
#endif

Instruction_t* IRDB_SDK::insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
	Instruction_t *new_instr = allocateNewInstruction(virp,first);
        setInstructionsDetails(virp,new_instr,dataBits,first->getFallthrough(), target);
        first->setFallthrough(new_instr);

        return new_instr;
}

#if 0
Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits)
{
        return insertDataBitsAfter(virp,first,dataBits,NULL);
}

vector<Instruction_t*> insertAssemblyInstructionsAfter(FileIR_t* virp, Instruction_t* first, string assemblygroup)
{
	return insertAssemblyInstructionsAfter(virp, first, assemblygroup, NULL)
}
#endif


/** This function ssembles a group of instructions, separated by semicolons/newlines, into databits, and returns a vector of assembled instructions, with each item inside the vector being a string that represents each assembled instruction.
 * Param 1: The group of semicolon/newline delimited assembly instructions to be assembled.
 * Returns: a vector of assembled instructions, with each item inside the vector being a string that represents each assembled instruction.
 */
vector<string> assemblegroup(string group) {
        const auto bits = FileIR_t::getArchitectureBitWidth();
        auto *encode = (char *)NULL;
        auto count = (size_t)0;
        auto size = (size_t)0;

        const auto mode = (bits == 32) ? KS_MODE_32 : 
                      (bits == 64) ? KS_MODE_64 :
                      throw std::invalid_argument("Cannot map IRDB bit size to keystone bit size");
    
    const auto machinetype = FileIR_t::getArchitecture()->getMachineType();
    const auto arch = (machinetype == IRDB_SDK::admtI386 || machinetype == IRDB_SDK::admtX86_64) ? KS_ARCH_X86 :
                      (machinetype == IRDB_SDK::admtArm32) ? KS_ARCH_ARM :
                      (machinetype == IRDB_SDK::admtAarch64) ? KS_ARCH_ARM64 : 
                      (machinetype == IRDB_SDK::admtMips64 || machinetype == IRDB_SDK::admtMips32) ? KS_ARCH_MIPS :
                      throw std::invalid_argument("Cannot map IRDB architecture to keystone architure");
    auto ks = (ks_engine *)NULL;
    const auto err = ks_open(arch, mode, &ks);
        assert(err == KS_ERR_OK);

        if(ks_asm(ks, group.c_str(), 0, (unsigned char **)&encode, &size, &count) != KS_ERR_OK) { //string or cstr
                ks_free((unsigned char*)encode);
                ks_close(ks);
                throw std::runtime_error("ERROR: ks_asm() failed during instrunction assembly.");
    }
        else {
        	vector<string> assembled;
        	csh handle;
        	auto insn = (cs_insn *)NULL;
        	auto cscount = (size_t)0;

			const auto csmode = (bits == 32) ? CS_MODE_32 : 
                      (bits == 64) ? CS_MODE_64 :
                      throw std::invalid_argument("Cannot map IRDB bit size to keystone bit size");

            const auto csarch = (machinetype == IRDB_SDK::admtI386 || machinetype == IRDB_SDK::admtX86_64) ? CS_ARCH_X86 :
                      (machinetype == IRDB_SDK::admtArm32) ? CS_ARCH_ARM :
                      (machinetype == IRDB_SDK::admtAarch64) ? CS_ARCH_ARM64 : 
                      (machinetype == IRDB_SDK::admtMips64 || machinetype == IRDB_SDK::admtMips32) ? CS_ARCH_MIPS :
                      throw std::invalid_argument("Cannot map IRDB architecture to keystone architure");

            const auto cserr = cs_open(csarch, csmode, &handle);
            assert(cserr == CS_ERR_OK);
            cscount = cs_disasm(handle, (const unsigned char*)encode, size, 0x1000, 0, &insn);
            if(cscount == count) {
            	auto assembleidx = 0;
            	for(unsigned int i = 0; i < cscount; i++) {
            		assembled.push_back(string(&encode[assembleidx], insn[i].size));
            		assembleidx += insn[i].size;
            	}
            }
            else {
            	throw std::runtime_error("ERROR: cs_disasm() failed during instrunction assembly.");
            }
            cs_free(insn, cscount);
            ks_free((unsigned char*)encode);
            ks_close(ks);
            cs_close(&handle);
            return assembled;
        }

}


Instruction_t* IRDB_SDK::addNewDataBits(FileIR_t* firp, Instruction_t *p_instr, string p_bits)
{
	Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->getAddress()->getFileID(), p_instr->getFunction());
        else
                newinstr = allocateNewInstruction(firp,firp->getFile()->getFileID(), NULL);

        newinstr->setDataBits(p_bits);

        if (p_instr)
        {
                newinstr->setFallthrough(p_instr->getFallthrough());
                p_instr->setFallthrough(newinstr);
        }

        return newinstr;
}

Instruction_t* IRDB_SDK::addNewDataBits(FileIR_t* firp, string p_bits)
{
	return IRDB_SDK::addNewDataBits(firp,nullptr,p_bits);
}


Instruction_t* IRDB_SDK::addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm)
{
	Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->getAddress()->getFileID(), p_instr->getFunction());
        else
                newinstr = allocateNewInstruction(firp,firp->getFile()->getFileID(), NULL);

        firp->registerAssembly(newinstr, p_asm);

        if (p_instr)
        {
                newinstr->setFallthrough(p_instr->getFallthrough());
                p_instr->setFallthrough(newinstr);
        }

        return newinstr;
}

Instruction_t* IRDB_SDK::addNewAssembly(FileIR_t* firp, string p_asm)
{
	return IRDB_SDK::addNewAssembly(firp,nullptr,p_asm);
}


//Does not insert into any variant
Instruction_t* copyInstruction(Instruction_t* instr)
{
	Instruction_t* cpy = new libIRDB::Instruction_t();

	copyInstruction(instr,cpy);

	return cpy;
}

Instruction_t* copyInstruction(FileIR_t* virp, Instruction_t* instr)
{
	Instruction_t* cpy = allocateNewInstruction(virp,instr);

	copyInstruction(instr,cpy);

	return cpy;
}

void copyInstruction(Instruction_t* src, Instruction_t* dest)
{
	auto real_dest=dynamic_cast<libIRDB::Instruction_t*>(dest);
	dest->setDataBits(src->getDataBits());
	dest->setComment(src->getComment());
	dest->setCallback(src->getCallback());
	dest->setFallthrough(src->getFallthrough());
	dest->setTarget(src->getTarget());
        dest->setIBTargets(src->getIBTargets()); 
	real_dest->GetRelocations()=src->getRelocations();
        dest->setEhProgram(src->getEhProgram());
	dest->setEhCallSite(src->getEhCallSite());
}

Instruction_t* allocateNewInstruction(FileIR_t* virp, DatabaseID_t p_fileID,Function_t* func)
{
	auto instr = new libIRDB::Instruction_t();
	auto a     = new libIRDB::AddressID_t();

	a->setFileID(p_fileID);

	instr->setFunction(func);
	instr->setAddress(a);
	if(func)
	{
		auto real_func=dynamic_cast<libIRDB::Function_t*>(func);
		assert(func);
		real_func->GetInstructions().insert(instr);
	}

	auto real_virp=dynamic_cast<libIRDB::FileIR_t*>(virp);
	assert(real_virp);
	real_virp->GetInstructions().insert(instr);
	real_virp->GetAddresses().insert(a);

//	inserted_instr[func].insert(instr);
//	inserted_addr[func].insert(a);
	
	return instr;
}

Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr)
{
	Function_t *func = template_instr->getFunction();
	DatabaseID_t fileID = template_instr->getAddress()->getFileID();
	return allocateNewInstruction(virp, fileID, func);
}

void IRDB_SDK::setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, const string& p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	if (p_instr == NULL) return;
	
	///TODO: what if bad assembly?
	virp->registerAssembly(p_instr,p_assembly);
	p_instr->setComment(p_assembly); 
	p_instr->setFallthrough(p_fallThrough); 
	p_instr->setTarget(p_target); 

	auto real_virp=dynamic_cast<libIRDB::FileIR_t*>(virp);
	assert(real_virp);
	real_virp->GetAddresses().insert(p_instr->getAddress());
	real_virp->GetInstructions().insert(p_instr);
}

void IRDB_SDK::setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, const string& p_assembly)
{
	IRDB_SDK::setInstructionAssembly(virp,p_instr,p_assembly, p_instr->getFallthrough(), p_instr->getTarget());
}


