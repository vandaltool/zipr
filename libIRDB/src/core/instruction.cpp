#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h> 
#include <fstream>
#include <sstream>
#include <iomanip>


using namespace libIRDB;
using namespace std;

Instruction_t::Instruction_t() :
	BaseObj_t(NULL), 
	data(""),
	callback(""),
	comment("")
{
	SetBaseID(NOT_IN_DATABASE);
	my_address=NULL;
	my_function=NULL;
	orig_address_id=NOT_IN_DATABASE;
	fallthrough=NULL;
	target=NULL;
	indTarg=NULL;
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
	data(thedata),
	callback(my_callback),
	comment(my_comment),
	indTarg(my_indTarg)
{
	SetBaseID(id);
	my_address=addr;
	my_function=func;
	orig_address_id=orig_id;
	fallthrough=NULL;
	target=NULL;
}

int Instruction_t::Disassemble(DISASM &disasm){
 
  	memset(&disasm, 0, sizeof(DISASM));
  
  	disasm.Options = NasmSyntax + PrefixedNumeral;
	disasm.Archi = FileIR_t::GetArchitectureBitWidth();
  	disasm.EIP = (UIntPtr) GetDataBits().c_str();
  	disasm.VirtualAddr = GetAddress()->GetVirtualOffset();
  	int instr_len = Disasm(&disasm);
 	 
  	return instr_len;  
}

std::string Instruction_t::getDisassembly()
{
  	DISASM disasm;
  	Disassemble(disasm);
  	return std::string(disasm.CompleteInstr);
}

// 
// Given an instruction in assembly, returns the raw bits in a string
// On error, return the empty string
//
bool Instruction_t::Assemble(string assembly)
{
   const string assemblyFile = "tmp.asm"; 
   const string binaryOutputFile = "tmp.bin";

   //remove any preexisting assembly or nasm generated files
   string command = "rm -f " + assemblyFile;
   system(command.c_str());
   command = "rm -f "+assemblyFile+".bin";
   system(command.c_str());

   ofstream asmFile;
   asmFile.open(assemblyFile.c_str());
   if(!asmFile.is_open())
   {
     return false;
   }

   asmFile<<NASM_BIT_WIDTH<<endl; // define to be 32

   asmFile<<assembly<<endl;
   asmFile.close();

   command = "nasm " + assemblyFile + " -o "+ binaryOutputFile;
   system(command.c_str());

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
    for (int i = 0; i < filesize; ++i)
      rawBits[i] = memblock[i];

    // should erase those 2 files here

    this->SetDataBits(rawBits);
    return true;
}


string Instruction_t::WriteToDB(File_t *fid, db_id_t newid, bool p_withHeader)
{
	assert(fid);
	assert(my_address);

	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

	db_id_t func_id=NOT_IN_DATABASE;
	if(my_function)
		func_id=my_function->GetBaseID();

	db_id_t ft_id=NOT_IN_DATABASE;
	if(fallthrough)
		ft_id=fallthrough->GetBaseID();

	db_id_t targ_id=NOT_IN_DATABASE;
	if(target)
		targ_id=target->GetBaseID();

	db_id_t indirect_bt_id=NOT_IN_DATABASE;
	if(indTarg)
		indirect_bt_id=indTarg->GetBaseID();

	string q;
	
	if (p_withHeader) 
		q = string("insert into ")+fid->instruction_table_name +
                string(" (instruction_id, address_id, parent_function_id, orig_address_id, fallthrough_address_id, target_address_id, data, callback, comment, ind_target_address_id, doip_id) VALUES ");
	else
		q = ",";

	ostringstream hex_data;
	hex_data << setfill('0') << hex;;
	for (size_t i = 0; i < data.length(); ++i)
		hex_data << setw(2) << (int)(data[i]&0xff);
	
	q += string("('") + to_string(GetBaseID())            	+ string("', ") +
                string("'") + to_string(my_address->GetBaseID())   	+ string("', ") +
                string("'") + to_string(func_id)            		+ string("', ") +
                string("'") + to_string(orig_address_id)         	+ string("', ") +
                string("'") + to_string(ft_id)         			+ string("', ") +
                string("'") + to_string(targ_id)         		+ string("', ") +
                string("decode('") + hex_data.str()                     + string("', 'hex'), ") +
                string("'") + callback                              	+ string("', ") +
                string("'") + comment                              	+ string("', ") +
                string("'") + to_string(indirect_bt_id)                 + string("', ") +
                string("'") + to_string(GetDoipID())            	+ string("')  ") ;

	return q;
}


/* return true if this instructino exits the function -- true if there's no function, because each instruction is it's own function? */
bool Instruction_t::IsFunctionExit() const 
{ 
	if(!my_function) 
		return true;  

	/* if there's a target that's outside this function */
	Instruction_t *target=GetTarget();
	if(target && !is_in_set(my_function->GetInstructions(),target))
		return true;

	/* if there's a fallthrough that's outside this function */
	Instruction_t *ft=GetFallthrough();
	if(fallthrough && !is_in_set(my_function->GetInstructions(),ft))
		return true;

	/* some instructions have no next-isntructions defined in the db, and we call them function exits */
	if(!target && !fallthrough)
		return true;

	return false;
}


bool Instruction_t::SetsStackPointer(ARGTYPE* arg)
{
	if(arg->AccessMode!=WRITE)
		return false;
	int access_type=arg->ArgType & 0xFFFF0000;

	if(access_type==REGISTER_TYPE + GENERAL_REG +REG4)
		return true;
	return false;
	
}

bool Instruction_t::SetsStackPointer(DISASM* disasm)
{
	if(strstr(disasm->Instruction.Mnemonic, "push")!=NULL)
		return true;
	if(strstr(disasm->Instruction.Mnemonic, "pop")!=NULL)
		return true;
	if(strstr(disasm->Instruction.Mnemonic, "call")!=NULL)
		return true;
	if(disasm->Instruction.ImplicitModifiedRegs==REGISTER_TYPE+GENERAL_REG+REG4)
		return true;


	if(SetsStackPointer(&disasm->Argument1)) return true;
	if(SetsStackPointer(&disasm->Argument2)) return true;
	if(SetsStackPointer(&disasm->Argument3)) return true;

	return false;

}
