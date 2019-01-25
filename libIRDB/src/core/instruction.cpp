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

#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h> 
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utils.hpp>

#undef EIP

using namespace libIRDB;
using namespace std;

Instruction_t::Instruction_t() :
	BaseObj_t(NULL), 
	my_address(NULL),
	my_function(NULL),
	orig_address_id(NOT_IN_DATABASE),
	fallthrough(NULL),
	target(NULL),
	data(""),
	callback(""),
	comment(""),
	indTarg(NULL),
	icfs(NULL),
	eh_pgm(NULL),
	eh_cs(NULL)
{
	setBaseID(NOT_IN_DATABASE);
}

Instruction_t::Instruction_t(db_id_t id, 
		AddressID_t *addr, 
		Function_t *func, 
		db_id_t orig_id, 
                std::string thedata, 
		std::string my_callback, 
		std::string my_comment, 
		AddressID_t *my_indTarg, 
		db_id_t doip_id) :

	BaseObj_t(NULL), 
	my_address(addr),
	my_function(func),
	orig_address_id(orig_id),
	fallthrough(NULL),
	target(NULL),
	data(thedata),
	callback(my_callback),
	comment(my_comment),
	indTarg(my_indTarg),
	icfs(NULL),
	eh_pgm(NULL),
	eh_cs(NULL)
{
	setBaseID(id);
}

/*
int Instruction_t::Disassemble(DISASM &disasm) const
{
 
  	memset(&disasm, 0, sizeof(DISASM));
  
  	disasm.Options = NasmSyntax + PrefixedNumeral;
	disasm.Archi = FileIR_t::getArchitectureBitWidth();
  	disasm.EIP = (UIntPtr) getDataBits().c_str();
  	disasm.VirtualAddr = getAddress()->getVirtualOffset();
  	int instr_len = Disasm(&disasm);
 	 
  	return instr_len;  
}
*/

std::string Instruction_t::getDisassembly() const
{
//  	DISASM disasm;
//  	Disassemble(this,disasm);
//  	return std::string(disasm.CompleteInstr);

	const auto d=DecodedInstruction_t::factory(this);
	return d->getDisassembly();
}

// 
// Given an instruction in assembly, returns the raw bits in a string
// On error, return the empty string
//
bool Instruction_t::assemble(string assembly)
{
   const string assemblyFile = "tmp.asm"; 
   const string binaryOutputFile = "tmp.bin";

   //remove any preexisting assembly or nasm generated files
   string command = "rm -f " + assemblyFile;
   command_to_stream(command,cout);
   command = "rm -f "+assemblyFile+".bin";
   command_to_stream(command,cout);

   ofstream asmFile;
   asmFile.open(assemblyFile.c_str());
   if(!asmFile.is_open())
   {
     return false;
   }

   asmFile<<"BITS "<<std::dec<<FileIR_t::getArchitectureBitWidth()<<endl; 

   asmFile<<assembly<<endl;
   asmFile.close();

   command = "nasm " + assemblyFile + " -o "+ binaryOutputFile;
   command_to_stream(command,cout);

    ifstream binreader;
    unsigned int filesize;
    binreader.open(binaryOutputFile.c_str(),ifstream::in|ifstream::binary);

    if(!binreader.is_open())
    {
      return false;
    }

    binreader.seekg(0,ios::end);

    filesize = binreader.tellg();

    binreader.seekg(0,ios::beg);

    if (filesize == 0) return false;

    unsigned char *memblock = new unsigned char[filesize];

    binreader.read((char*)memblock,filesize);
    binreader.close();

    string rawBits;
    rawBits.resize(filesize);
    for (auto i = 0U; i < filesize; ++i)
      rawBits[i] = memblock[i];

    // should erase those 2 files here

    this->setDataBits(rawBits);
    return true;
}


vector<string> Instruction_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid);
	assert(my_address);

	if(getBaseID()==NOT_IN_DATABASE)
		setBaseID(newid);

	auto func_id=NOT_IN_DATABASE;
	if(my_function) func_id=my_function->getBaseID();

	auto ft_id=NOT_IN_DATABASE;
	if(fallthrough) ft_id=fallthrough->getBaseID();

	auto targ_id=NOT_IN_DATABASE;
	if(target) targ_id=target->getBaseID();

	auto icfs_id=NOT_IN_DATABASE;
	if (icfs) icfs_id=icfs->getBaseID();

	auto indirect_bt_id=NOT_IN_DATABASE;
	if(indTarg) indirect_bt_id=indTarg->getBaseID();

	auto eh_pgm_id=NOT_IN_DATABASE;
	if(eh_pgm) eh_pgm_id=eh_pgm->getBaseID();

	auto eh_css_id=NOT_IN_DATABASE;
	if(eh_cs) eh_css_id=eh_cs->getBaseID();

	ostringstream hex_data;
	hex_data << setfill('0') << hex;;
	for (size_t i = 0; i < data.length(); ++i)
		hex_data << setw(2) << (int)(data[i]&0xff);


	return {
		to_string(getBaseID()),
                to_string(my_address->getBaseID()),
                to_string(func_id),
                to_string(orig_address_id),
                to_string(ft_id),
                to_string(targ_id),
                to_string(icfs_id),
                to_string(eh_pgm_id),
                to_string(eh_css_id),
                hex_data.str(),
                callback,
                comment,
                to_string(indirect_bt_id),
                to_string(getDoipID()) };
}


/* return true if this instructino exits the function -- true if there's no function, because each instruction is it's own function? */
bool Instruction_t::isFunctionExit() const 
{ 
	if(!my_function) 
		return true;  

	/* if there's a target that's outside this function */
	auto target=getTarget();
	if(target && target->getFunction()!=getFunction()) // !is_in_set(my_function->GetInstructions(),target))
		return true;

	/* if there's a fallthrough that's outside this function */
	auto ft=getFallthrough();
	if(fallthrough && ft->getFunction()!=getFunction()) // !is_in_set(my_function->GetInstructions(),ft))
		return true;

	/* some instructions have no next-isntructions defined in the db, and we call them function exits */
	if(!target && !fallthrough)
		return true;

	return false;
}

IRDB_SDK::Function_t* Instruction_t::getFunction() const
{
	return my_function;
}


IRDB_SDK::EhProgram_t*  Instruction_t::getEhProgram()  const  { return eh_pgm; }
IRDB_SDK::EhCallSite_t* Instruction_t::getEhCallSite() const  { return eh_cs; }
IRDB_SDK::AddressID_t*  Instruction_t::getIndirectBranchTargetAddress() const { return indTarg; }

void Instruction_t::setAddress     (IRDB_SDK::AddressID_t* newaddr)   { my_address=dynamic_cast<AddressID_t*>(newaddr); }
void Instruction_t::setFunction    (IRDB_SDK::Function_t* func   )    { my_function=dynamic_cast<Function_t*>(func);}
void Instruction_t::setFallthrough (IRDB_SDK::Instruction_t* i)       { fallthrough=dynamic_cast<Instruction_t*>(i); }
void Instruction_t::setTarget      (IRDB_SDK::Instruction_t* i)       { target=dynamic_cast<Instruction_t*>(i); }
void Instruction_t::setIBTargets   (IRDB_SDK::ICFS_t *p_icfs)         { icfs=dynamic_cast<ICFS_t*>(p_icfs); }
void Instruction_t::setEhProgram(IRDB_SDK::EhProgram_t* orig)         { eh_pgm=dynamic_cast<EhProgram_t*>(orig); }
void Instruction_t::setEhCallSite(IRDB_SDK::EhCallSite_t* orig)       { eh_cs=dynamic_cast<EhCallSite_t*>(orig); }
void Instruction_t::setIndirectBranchTargetAddress(IRDB_SDK::AddressID_t* myIndTarg)  { indTarg=dynamic_cast<AddressID_t*>(myIndTarg); }


