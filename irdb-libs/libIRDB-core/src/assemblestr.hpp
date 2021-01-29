

#ifndef assemblestr_h
#define assemblestr_h

static void assemblestr(ks_engine * &ks, IRDB_SDK::Instruction_t *ins, const char * instruct, char * &encode, size_t &size, size_t &count)
{
        if(ks_asm(ks, instruct, 0, (unsigned char **)&encode, &size, &count) != KS_ERR_OK) { //string or cstr
                ks_free((unsigned char*)encode);
                ks_close(ks);
                throw std::runtime_error("ERROR: ks_asm() failed during instrunction assembly.");
    }
        else {
                ins->setDataBits(string(encode, size));
                ks_free((unsigned char*)encode);
        }
}

#endif // assemblestr_h
