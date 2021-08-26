

#ifndef assemblestr_h
#define assemblestr_h

static void assemblestr(ks_engine * &ks, IRDB_SDK::Instruction_t *ins, const char * instruct, char * &encode, size_t &size, size_t &count)
{
	// Count is the number of statements assembled.  
	// Size is the number of bytes returned by the assembling.
	// we want exactly one instruction back, but size can't tell us how many instructions
	// nor can the ks_asm return code or count, because multiple "statements" can be 
	// assembled to one instruction, e.g. 
	//
	// L1: jmp L1  --  two statements, one instruction.
	// nop; nop --  is two bytes, but 2 statements
	// # comment -- is one statement but no bytes.
	// So, we sanity check the best we can here by checking all 3 are in a sane range
	// 
	// Todo: add a call to capstone to disassemble the bytes and verify that capstone
	// returns 1.
	if(
		ks_asm(ks, instruct, 0, (unsigned char **)&encode, &size, &count) != KS_ERR_OK || 
		count <= 0 || 
		size<=0 || size >17 
	  ) 
	{ 
		ks_free((unsigned char*)encode);
		ks_close(ks);
		throw std::runtime_error("ERROR: ks_asm() failed during instrunction assembly.");
	}
	else 
	{
		ins->setDataBits(string(encode, size));
		ks_free((unsigned char*)encode);
	}
}

#endif // assemblestr_h
