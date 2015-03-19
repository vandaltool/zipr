#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <beaengine/BeaEngine.h>
 
int main(int argc, char* argv[])
{

//FILE* fin=fopen(argv[1],"rb");
//assert(fin);
int max_len=10*1024*1024; /* 10 meg */
char *buf=malloc(max_len);
int bytes_read=fread(buf,1,max_len,stdin);
assert(bytes_read>0);

/* ============================= Init datas */
DISASM MyDisasm;
int len;
int Error = 0;
UInt64 EndCodeSection = (UInt64)(buf+bytes_read);


 
/* ============================= Init the Disasm structure (important !)*/
(void) memset (&MyDisasm, 0, sizeof(DISASM));
 
/* ============================= Init EIP */
MyDisasm.EIP = (UIntPtr) buf;
MyDisasm.Options = NasmSyntax + PrefixedNumeral;
MyDisasm.Archi = 64;
MyDisasm.VirtualAddr = 0;

 printf("0 0 bits 64\n"); 
 
/* ============================= Loop for Disasm */
while (!Error){
    /* ============================= Fix SecurityBlock */
    MyDisasm.SecurityBlock = (UIntPtr)EndCodeSection - (UIntPtr)MyDisasm.EIP+1;
 
    len = Disasm(&MyDisasm);
    if (len == OUT_OF_BLOCK) {
        (void) printf("; disasm engine is not allowed to read more memory \n");
        Error = 1;
    }
    else if (len == UNKNOWN_OPCODE) {
        (void) printf("unknown opcode\n");
        Error = 1;
    }
    else {
	int i;
	if(argc>2)
	{
		printf("%08X    ", (int)(MyDisasm.EIP-(UInt64)buf));
		for(i=0;i<8;i++)
		{
			if(i>=len)
				printf("  ");
			else
				printf("%02X", ((UInt8*)MyDisasm.EIP)[i]);
		}
	}
	printf("  ");
        (void) puts(MyDisasm.CompleteInstr);

	if(i<len && argc>2)
	{
		printf("           -");
		for(;i<len;i++)
			printf("%02X", ((UInt8*)MyDisasm.EIP)[i]);
		printf("\n");
	}


        MyDisasm.EIP = MyDisasm.EIP + (UIntPtr)len;
        if (MyDisasm.EIP >= EndCodeSection) {
            (void) printf("0 0 ; End of buffer reached ! \n");
            Error = 1;
        }
    }
};
return 0;
}
