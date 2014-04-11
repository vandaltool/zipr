
/* 


Description of VEX encodings from wiki.
The VEX coding scheme uses a code prefix consisting of 2 or 3 
bytes which is added to existing or new instruction codes.[1]

In x86 architecture, instructions with a memory operand may use the ModR/M 
byte which specifies the addressing mode. This byte has three bit fields:

    mod, bits [7:6] - combined with the r/m field, encodes either 8 
	registers or 24 addressing modes. Also encodes opcode information 	
	for some instructions
    reg/opcode, bits [5:3] - specifies either a register or three more 
	bits of opcode information, as specified in the primary opcode byte
    r/m, bits [2:0] - can specify a register as an operand, or combine 
	with the mod field to encode an addressing mode.

The base-plus-index and scale-plus-index forms of 32-bit addressing 
(encoded with r/m=100 and mod <>11) require another addressing byte, the SIB 
byte. It has the following fields:

    scale factor, encoded with bits [7:6]
    index register, bits [5:3]
    base register, bits [2:0].

To use 64-bit addressing and additional registers present in the x86-64 
architecture, the REX prefix has been introduced which provides additional space 
for encoding addressing modes. Bit-field W expands the operand size to 64 bits, 
R expands reg, B expands r/m or reg (depending on the opcode format used), and 
X and B expand index and base in the SIB byte. However REX prefix is encoded quite 
inefficiently, wasting half of its 8 bits.

VEX encoding 3-byte VEX
	7 	6 	5 	4 	3 	2 	1 	0
Byte 0 	1 	1 	0 	0 	0 	1 	0 	0
Byte 1 	R̅ 	X̅ 	B̅ 	m4 	m3 	m2 	m1 	m0
Byte 2 	W 	v̅3 	v̅2 	v̅1 	v̅0 	L 	p1 	p0

2-byte VEX
	7 	6 	5 	4 	3 	2 	1 	0
Byte 0 	1 	1 	0 	0 	0 	1 	0 	1
Byte 1 	R̅ 	v̅3 	v̅2 	v̅1 	v̅0 	L 	p1 	p0

The VEX prefix provides a compact representation of the REX prefix, as well as 
various other prefixes, to expand the addressing mode, register enumeration 
and operand size and width:

    .) R̅, X̅ and B̅ bits are inversion of the REX prefix's R, X and B bits; 
	these provide a fourth (high) bit for register index fields 
	(ModRM reg, SIB index, and ModRM r/m; SIB base; or opcode reg fields, 
	respectively) allowing access to 16 instead of 8 registers. The W bit 
	is equivalent to the REX prefix's W bit, and specifies a 64-bit 
	operand; for non-integer instructions, it is a general opcode extension 
	bit.
    .) v̅ is the inversion of an additional source register index.
    .) m replaces leading opcode prefix bytes. The values 1, 2 and 3 are equivalent 
	to opcode prefixes 0F, 0F 38 and 0F 3A; all other values are reserved. 
	The 2-byte VEX prefix always corresponds to the 0F prefix.
    .) L indicates the vector length; 0 for 128-bit SSE (XMM) registers, and 1 
	for 256-bit AVX (YMM) registers.
    .) p encodes additional prefix bytes. The values 0, 1, 2, and 3 correspond to 
	implied prefixes of none, 66, F3, and F2. These encode the operand type 
	for SSE instructions: packed single, packed double, scalar single and 
	scalar double, respectively.

The VEX opcode bytes, C4h and C5h, are the same as that used by the LDS and LES instructions. 
These instructions are not supported in 64-bit mode, while in 32-bit mode a following ModRM 
byte can not be of the form 11xxxxxx (which would specify a register operand). Various bits 
are inverted to ensure that the second byte of a VEX prefix is always of this form in 32-bit mode.

Instructions that need more than three operands have an extra suffix byte specifying one or 
two additional register operands. Instructions coded with the VEX prefix can have up to 
five operands. At most one of the operands can be a memory operand; and at most one of the 
operands can be an immediate constant of 4 or 8 bits. The remaining operands are registers.

The AVX instruction set is the first instruction set extension to use the VEX coding scheme. 
The AVX instructions have up to four operands. The AVX instruction set allows the VEX prefix 
to be applied only to instructions using the SIMD XMM registers. However, the VEX coding scheme 
has space for applying the VEX prefix to other instructions as well in future instruction sets.

Legacy SIMD instructions with a VEX prefix added are equivalent to the same instructions without 
VEX prefix with the following differences:

    .) The VEX-encoded instruction can have one more operand, making it non-destructive.

    .) 128-bit XMM instruction without VEX prefix leaves the upper half of the full 
	256-bit YMM register unchanged, while the VEX-encoded version sets the upper 
	half to zero.

Instructions that use the whole 256-bit YMM register should not be mixed with non-VEX 
instructions that leave the upper half of the register unchanged, for reasons of efficiency.
*/

#include <assert.h>


static void finish_vex(PDISASM pMyDisasm)
{
	/* the VEX prefix negates the use of some other prefixes */
	if(GV.REX.state || 
	   (*pMyDisasm).Prefix.LockPrefix ||
	   (*pMyDisasm).Prefix.RepnePrefix ||
	   (*pMyDisasm).Prefix.RepPrefix ||
	   (*pMyDisasm).Prefix.OperandSize
	  )
		FailDecode(pMyDisasm);
	
	/* deal with sizing */
	GV.OperandSize=128;
	if(GV.VEX.length)
		GV.OperandSize=256;


	/* deal with implicit prefixes. */
	/* the implicit prefix can also be an opcode-size for SSE instructions, 
	 * so it's impossible to know ahead of time if we should convert
	 * this to an actual prefix indicator, so the difficulty of 
	 * dealing with prefix vrs opcode size/extension needs to be handled
	 * in the actual instruction 
	 */
	switch(GV.VEX.implicit_prefixes)
	{ 
		case 0:
			break;
		case 1:  /* 0x66 */
			(*pMyDisasm).Prefix.OperandSize = InUsePrefix; 
			break;
		case 2:  /* 0xf3 */
			(*pMyDisasm).Prefix.RepPrefix= InUsePrefix; 
			GV.PrefRepe= 1; /* why are these diff?  not sure, but seems to be how it should be */
			break;
		case 3:  /* 0xf2 */
			(*pMyDisasm).Prefix.RepnePrefix= InUsePrefix; 
			GV.PrefRepne = 1; /* why are these diff?  not sure, but seems to be how it should be */
			break;
		default:
			FailDecode(pMyDisasm);
	}
}


/* ====================================================================
 *      0x c4
VEX encoding 3-byte VEX
	7 	6 	5 	4 	3 	2 	1 	0
Byte 0 	1 	1 	0 	0 	0 	1 	0 	0
Byte 1 	R̅ 	X̅ 	B̅ 	m4 	m3 	m2 	m1 	m0
Byte 2 	W 	v̅3 	v̅2 	v̅1 	v̅0 	L 	p1 	p0
 * ==================================================================== */
void __bea_callspec__ HandleVex3(PDISASM pMyDisasm)
{

	UInt8 byte1=0, byte2=0;
	if(GV.REX.state) FailDecode(pMyDisasm);

	assert(pMyDisasm);
    	if (!Security(4, pMyDisasm)) return;

	GV.VEX.has_vex=1; /* TRUE */
	GV.VEX.has_vex3=1;

	/* check byte 0 */
	assert(*(UInt8*)GV.EIP_ == 0xc4);

	/* advance to byte 1 */
    	GV.EIP_++;
	byte1=*(UInt8*)GV.EIP_;

	/* fill in fields from byte1. */
	GV.VEX.notR=(byte1>>7)&1;	/* bit 8 */
	GV.REX.R_=!GV.VEX.notR;
	GV.VEX.notX=(byte1>>6)&1;	/* bit 7 */
	GV.REX.X_=!GV.VEX.notX;
	GV.VEX.notB=(byte1>>5)&1;	/* bit 6 */
	GV.REX.B_=!GV.VEX.notB;
	GV.VEX.opcode_escape=byte1&0x1f; /* bits 5-0 */


	/* advance to byte 2 */
    	GV.EIP_++;
	byte2=*(UInt8*)GV.EIP_;
	/* fill in fields from byte2 */
	GV.VEX.W=(byte2>>7)&0x1;	/* bit 7 */
	GV.REX.W_=GV.VEX.W;
	GV.VEX.notV=(byte2>>3)&0xf;	/* bits 6-3 */
	GV.VEX.length=(byte2>>2)&01;	/* bit 2 */
	GV.VEX.implicit_prefixes=(byte2>>0)&0x3;	/* bits 1-0 */

    	GV.EIP_++;


	finish_vex(pMyDisasm);
	

	/* dispatch to the right table, depending on the vex opcode_escape value */
	switch(GV.VEX.opcode_escape)
	{
		case 1:
    			(*pMyDisasm).Instruction.Opcode = *((UInt8*) (UIntPtr)GV.EIP_)+0x0F00;
    			(void) opcode_map2[*((UInt8*) (UIntPtr)GV.EIP_)](pMyDisasm);
			break;
		case 2:
    			(*pMyDisasm).Instruction.Opcode = *((UInt8*) (UIntPtr)GV.EIP_)+0x0F3800;
    			(void) opcode_map3[*((UInt8*) (UIntPtr)GV.EIP_)](pMyDisasm);
			break;
		case 3:
    			(*pMyDisasm).Instruction.Opcode = *((UInt8*) (UIntPtr)GV.EIP_)+0x0F3A00;
    			(void) opcode_map4[*((UInt8*) (UIntPtr)GV.EIP_)](pMyDisasm);
			break;
		default:	
			FailDecode(pMyDisasm);
	}
}

/* ====================================================================
 *      0x c5

2-byte VEX encoding

	7 	6 	5 	4 	3 	2 	1 	0
Byte 0 	1 	1 	0 	0 	0 	1 	0 	1
Byte 1 	R̅ 	v̅3 	v̅2 	v̅1 	v̅0 	L 	p1 	p0

 * ==================================================================== */
void __bea_callspec__ HandleVex2(PDISASM pMyDisasm)
{
	UInt8 byte1=0;
	assert(pMyDisasm);
    	if (!Security(3, pMyDisasm)) return;
	if(GV.REX.state) FailDecode(pMyDisasm);

	GV.VEX.has_vex=1; /* TRUE */
	GV.VEX.has_vex2=1;
	GV.VEX.opcode_escape=1; /* use 0xf as opcode table */


	assert(*(UInt8*)GV.EIP_ == 0xc5);
    	GV.EIP_++;
	byte1=*(UInt8*)GV.EIP_;

	/* fill in fields from byte1. */
	GV.VEX.notR=(byte1>>7)&1;	/* bit 7 */
	GV.REX.R_=!GV.VEX.notR;
	GV.VEX.notX=1;			/* not set, so they should default to 1, not 0 */
	GV.REX.X_=!GV.VEX.notX;
	GV.VEX.notB=1;
	GV.REX.B_=!GV.VEX.notB;

	GV.VEX.notV=(byte1>>3)&0xf;	/* bits 6-3 */
	GV.VEX.length=(byte1>>2)&01;	/* bit 2 */
	GV.VEX.implicit_prefixes=(byte1>>0)&0x3;	/* bits 1-0 */

    	GV.EIP_++;

	finish_vex(pMyDisasm);

	/* dispatch to 0xf decode func table */
    	(*pMyDisasm).Instruction.Opcode = *((UInt8*) (UIntPtr)GV.EIP_)+0x0F00;
    	(void) opcode_map2[*((UInt8*) (UIntPtr)GV.EIP_)](pMyDisasm);
	

}


/* V_reg - Process REX.notV into a reg in Arg */
void V_reg(ARGTYPE* arg, PDISASM pMyDisasm)
{
	int reg=~GV.VEX.notV;
	reg&=0xf; 

	/* range check */
	assert(reg>=0 && reg<=15);

	arg->ArgSize=GV.OperandSize;
	arg->ArgType=REGISTER_TYPE+REGS[reg];

	if(GV.OperandSize==128 || GV.SSE_)
	{
    			#ifndef BEA_LIGHT_DISASSEMBLY
       				(void) strcpy((char*) arg->ArgMnemonic, RegistersSSE[reg]);
    			#endif
			arg->ArgType+=SSE_REG;
	}
	else if(GV.OperandSize==256 || GV.AVX_)
	{
    			#ifndef BEA_LIGHT_DISASSEMBLY
       				(void) strcpy((char*) arg->ArgMnemonic, RegistersAVX[reg]);
    			#endif
			arg->ArgType+=AVX_REG;
	}
	else
	{
			FailDecode(pMyDisasm);
	}
}

void L_imm(ARGTYPE* arg, PDISASM pMyDisasm)
{
	
	UInt8 imm8;

    	if (!Security(1, pMyDisasm)) return;
	imm8=*(UInt8*)GV.EIP_;

	arg->ArgSize=8;
	arg->ArgType=CONSTANT_TYPE+ABSOLUTE_;

        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) arg->ArgMnemonic, "%x",(Int64) imm8);
        #endif

	GV.EIP_++;
}

/* L_reg -- process an 8-byte immediate into a register for Arg */
void L_reg(ARGTYPE* arg, PDISASM pMyDisasm)
{
	
	UInt8 imm8,reg;

    	if (!Security(1, pMyDisasm)) return;
	imm8=*(UInt8*)GV.EIP_;
	imm8>>=4;
	imm8&=0xf;

	reg=imm8;

	/* range check */
	assert(reg<=15);

	arg->ArgSize=GV.OperandSize;
	arg->ArgType=REGISTER_TYPE+REGS[reg];

	switch(GV.OperandSize)
	{
		case 128:
    			#ifndef BEA_LIGHT_DISASSEMBLY
       				(void) strcpy((char*) arg->ArgMnemonic, RegistersSSE[reg]);
    			#endif
			arg->ArgType+=SSE_REG;
			break;
		case 256:
    			#ifndef BEA_LIGHT_DISASSEMBLY
       				(void) strcpy((char*) arg->ArgMnemonic, RegistersAVX[reg]);
    			#endif
			arg->ArgType+=AVX_REG;
			break;
		default:
			FailDecode(pMyDisasm);
			break;
	}

	GV.EIP_++;
}

void VxHxWxLx(PDISASM pMyDisasm)
{
	if(!GV.VEX.has_vex)
		FailDecode(pMyDisasm);

	V_reg(&(*pMyDisasm).Argument2, pMyDisasm);
    	MOD_RM(&(*pMyDisasm).Argument3, pMyDisasm);
    	Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
    	GV.EIP_ += GV.DECALAGE_EIP+2;
	L_reg(&(*pMyDisasm).Argument4, pMyDisasm);

	GV.third_arg=1;
	GV.forth_arg=1;
}

void VyHyWyLy(PDISASM pMyDisasm)
{
	if(!GV.VEX.has_vex)
		FailDecode(pMyDisasm);

	V_reg(&(*pMyDisasm).Argument2, pMyDisasm);
    	MOD_RM(&(*pMyDisasm).Argument3, pMyDisasm);
    	Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
    	GV.EIP_ += GV.DECALAGE_EIP+2;
	L_reg(&(*pMyDisasm).Argument4, pMyDisasm);

	GV.third_arg=1;
	GV.forth_arg=1;


}

/*
VEX.NDS.128.66.0F3A.W0 4B /r /is4
VBLENDVPD xmm1, xmm2, xmm3/m128, xmm4
RVMR V/V AVX Conditionally copy double-precision floatingpoint
values from xmm2 or xmm3/m128 to
xmm1, based on mask bits in the mask
operand, xmm4.

VEX.NDS.256.66.0F3A.W0 4B /r /is4
VBLENDVPD ymm1, ymm2, ymm3/m256, ymm4
RVMR V/V AVX Conditionally copy double-precision floatingpoint
values from ymm2 or ymm3/m256 to
ymm1, based on mask bits in the mask
operand, ymm4.
 */ 
void vblendvpd /*VxHxWxLx */ (PDISASM pMyDisasm)
{

        (*pMyDisasm).Instruction.Category = AVX_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vblendvpd ");
        #endif

    	if (!Security(1, pMyDisasm)) return;

	if(GV.VEX.length==0)
	{
		GV.SSE_=1;
		VxHxWxLx(pMyDisasm);
	}
	else
	{
		GV.AVX_=1;
		VyHyWyLy(pMyDisasm);
	}
}


/* not really VEX instructions, but handle all 3dnow instructions. */
void three_dnow_ (PDISASM pMyDisasm)
{
	UInt8 suffix;

	GV.MMX_=1;
	GV.MemDecoration=Arg2fword;
    	MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
    	Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
    	GV.EIP_ += GV.DECALAGE_EIP+2;
	GV.MMX_=0;


	suffix=*(UInt8*)GV.EIP_;	
	GV.EIP_++;

	switch(suffix)
	{
		case 0xbf:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pavgusb "); break;
		case 0x9e:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfadd "); break;
		case 0x9a:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfsub "); break;
		case 0xaa:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfsubr "); break;
		case 0xae:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfacc "); break;
		case 0x90:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfcmpge "); break;
		case 0xa0:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfcmpgt "); break;
		case 0xb0:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfcmpeq "); break;
		case 0x94:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfmin "); break;
		case 0xa4:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfmax "); break;
		case 0x0d:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pi2fd "); break;
		case 0x1d:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pf2id "); break;
		case 0x96:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfrcp "); break;
		case 0x97:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfrsqrt "); break;
		case 0xb4:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfmul "); break;
		case 0xa6:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfrcpit1 "); break;
		case 0xa7:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfrsqit1 "); break;
		case 0xb6:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pfrcqit2 "); break;
		case 0xb7:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmulhrwa "); break;
		case 0xbb:	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pswapd "); break;
		default:	FailDecode(pMyDisasm); break;
	}


}

void helperf128 (PDISASM pMyDisasm, const char* mnemonic, int third_reg_avx)
{
	if(GV.VEX.has_vex && GV.VEX.length==1 && GV.VEX.implicit_prefixes==1 /* 66 */ && GV.VEX.W==0)
	{
        	(*pMyDisasm).Instruction.Category = AVX_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        	#ifndef BEA_LIGHT_DISASSEMBLY
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, mnemonic);
        	#endif

		GV.MemDecoration=Arg3fword;
		GV.AVX_=third_reg_avx;
		GV.SSE_=!third_reg_avx;
    		MOD_RM(&(*pMyDisasm).Argument3, pMyDisasm);
		GV.AVX_=0;
		GV.SSE_=0;
		GV.AVX_=1;
		V_reg( &(*pMyDisasm).Argument2, pMyDisasm);
    		Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
		GV.AVX_=0;
    		GV.EIP_ += GV.DECALAGE_EIP+2;
		GV.third_arg=1;
		GV.forth_arg=1;
		L_imm(&(*pMyDisasm).Argument4,pMyDisasm);
	}
	else
		FailDecode(pMyDisasm);
}



/* vbroadcastss -- shortened for table formatting reasons */
void vbrdcstss  (PDISASM pMyDisasm)
{
	int origOpSize=0;

        (*pMyDisasm).Instruction.Category = AVX_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vbroadcastss ");
        #endif

        if(!GV.VEX.has_vex)
		FailDecode(pMyDisasm);

	origOpSize=GV.OperandSize;
	GV.OperandSize=128+GV.VEX.length*128;

	GV.AVX_=GV.VEX.length;
	GV.SSE_=!GV.VEX.length;

	GV.MemDecoration=Arg2dword;
    	MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
    	Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
    	GV.EIP_ += GV.DECALAGE_EIP+2;

	GV.AVX_=0;
	GV.SSE_=0;
	GV.OperandSize=origOpSize;
}

/* vbroadcastsd -- shortened for table formatting reasons */
void vbrdcstsd  (PDISASM pMyDisasm)
{
assert(pMyDisasm);
assert(0);
}

/* 0f 3a 19 */
void vextraf128 (PDISASM pMyDisasm)
{
	if(GV.VEX.has_vex && GV.VEX.length==1 && GV.VEX.implicit_prefixes==1 && GV.VEX.W==0)
	{
        	(*pMyDisasm).Instruction.Category = AVX_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        	#ifndef BEA_LIGHT_DISASSEMBLY
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vextractf128 ");
        	#endif
		GV.MemDecoration=Arg1fword;
		GV.SSE_=1;
    		MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
		GV.SSE_=0;
		GV.AVX_=1;
    		Reg_Opcode(&(*pMyDisasm).Argument2, pMyDisasm);
		GV.AVX_=0;
    		GV.EIP_ += GV.DECALAGE_EIP+2;
		GV.third_arg=1;
		L_imm(&(*pMyDisasm).Argument3,pMyDisasm);
	}
	else
		FailDecode(pMyDisasm);
}


/* 0f 3a 18 */
void vinsrtf128 (PDISASM pMyDisasm)
{
		helperf128(pMyDisasm, "vinsertf128 ", 0);
}

/* 0f 3a 06 */
void vperm2f128 (PDISASM pMyDisasm)
{
	helperf128(pMyDisasm, "vperm2f128 ", 1);
}

/* 0f 38 0c */
void vpermilps1 (PDISASM pMyDisasm)
{
	if(GV.VEX.has_vex && GV.VEX.length==1 && GV.VEX.implicit_prefixes==1 /* 66 */ && GV.VEX.W==0)
	{

        	(*pMyDisasm).Instruction.Category = AVX_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        	#ifndef BEA_LIGHT_DISASSEMBLY
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vpermilps ");
        	#endif

		GV.MemDecoration=Arg3fword;
		GV.AVX_=GV.VEX.length;
		GV.SSE_=!GV.VEX.length;
    		MOD_RM(&(*pMyDisasm).Argument3, pMyDisasm);
		V_reg( &(*pMyDisasm).Argument2, pMyDisasm);
    		Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
    		GV.EIP_ += GV.DECALAGE_EIP+2;
		GV.third_arg=1;
		GV.AVX_=0;
		GV.SSE_=0;
	}
	else
		FailDecode(pMyDisasm);
}

/* 0f 3a 04 */
void vpermilps2 (PDISASM pMyDisasm)
{
	assert(pMyDisasm); /* avoids warning */
	assert(0);
}
