/* Copyright 2006-2009, BeatriX
 * File coded by BeatriX
 *
 * This file is part of BeaEngine.
 *
 *    BeaEngine is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    BeaEngine is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with BeaEngine.  If not, see <http://www.gnu.org/licenses/>. */

#include <assert.h>

void __bea_callspec__   implicit_xmm0(ARGTYPE *arg, PDISASM pMyDisasm)
{
        if(GV.SSE_)
        {
                #ifndef BEA_LIGHT_DISASSEMBLY
                        (void) strcpy((char*) arg->ArgMnemonic, RegistersSSE[0]);
                #endif
                arg->ArgType=REGISTER_TYPE+SSE_REG+REG0;
        }
        else if(GV.AVX_)
        {
                #ifndef BEA_LIGHT_DISASSEMBLY
                        (void) strcpy((char*) arg->ArgMnemonic, RegistersAVX[0]);
                #endif

                arg->ArgType=REGISTER_TYPE+AVX_REG+REG0;
        }
        else
                assert(0);

}


/* ====================================================================
 *      0x 0f 58
 * ==================================================================== */
void __bea_callspec__ addps_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
	{
       		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
	}
        #endif
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==3)) {
//	assert(!GV.VEX.has_vex);
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		(void) strcat ((*pMyDisasm).Instruction.Mnemonic, "addsd ");
        #endif
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1|| (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {

//	assert(!GV.VEX.has_vex);
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "addss ");
        #endif
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix|| (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "addpd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2fword;
       	#ifndef BEA_LIGHT_DISASSEMBLY
      		(void) strcat ((*pMyDisasm).Instruction.Mnemonic, "addps ");
       	#endif
    }
    Vx_opt_GxEx_vexlen(pMyDisasm);
}

/* ====================================================================
 *      0x 0f d0
 * ==================================================================== */
void __bea_callspec__ addsubpd_(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+SIMD_FP_PACKED;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "addsubps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }

    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+SIMD_FP_PACKED;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "addsubpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 55
 * ==================================================================== */
void __bea_callspec__ andnps_VW(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "andnpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "andnps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 54
 * ==================================================================== */
void __bea_callspec__ andps_VW(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "andpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "andps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 3a 0d
 * ==================================================================== */
void __bea_callspec__ blendpd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "blendpd ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 0c
 * ==================================================================== */
void __bea_callspec__ blendps_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
	   if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vblendps ");
	   else
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "blendps ");
        #endif
	if(GV.VEX.has_vex)
	{
        	GV.ImmediatSize = 8;
        	GV.AVX_ = GV.VEX.length;
        	GV.SSE_ = !GV.VEX.length;
        	VxGxEx(pMyDisasm);
        	GV.SSE_ = 0;
        	GV.AVX_ = 0;
        	GV.EIP_++;
        	if (!Security(0, pMyDisasm)) return;
        	GV.third_arg = 1;
        	GV.forth_arg = 1;
        	(*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));

		/* forth arg */
        	#ifndef BEA_LIGHT_DISASSEMBLY
           	(void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument4.ArgMnemonic, "%x",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        	#endif
        	(*pMyDisasm).Argument4.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        	(*pMyDisasm).Argument4.ArgSize = 8;
	}

	else
	{
        	GV.ImmediatSize = 8;
        	GV.SSE_ = 1;
        	GxEx(pMyDisasm);
        	GV.SSE_ = 0;
        	GV.EIP_++;
        	if (!Security(0, pMyDisasm)) return;
        	GV.third_arg = 1;
        	(*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        	#ifndef BEA_LIGHT_DISASSEMBLY
           	(void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%x",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        	#endif
        	(*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        	(*pMyDisasm).Argument3.ArgSize = 8;
	}

    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 15
 * ==================================================================== */
void __bea_callspec__ blendvpd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "blendvpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
	GV.third_arg=1;
	implicit_xmm0(&(*pMyDisasm).Argument3,pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 38 14
 * ==================================================================== */
void __bea_callspec__ blendvps_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "blendvps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f c2
 * ==================================================================== */
void __bea_callspec__ cmpps_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif

    /* ========= 0xf2 */
    GV.ImmediatSize = 8;
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "cmpsd ");
        #endif
    }
    /* ========== 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "cmpss ");
        #endif
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "cmppd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "cmpps ");
        #endif
    }
    	Vx_opt_GxEx_vexlen(pMyDisasm);
    	if (!Security(0, pMyDisasm)) return;
    	GV.third_arg = 1;
    	if(GV.VEX.has_vex)
	{
    		GV.forth_arg = 1;
		L_imm(&pMyDisasm->Argument4,pMyDisasm);
	}
	else
	{
		L_imm(&pMyDisasm->Argument3,pMyDisasm);
	}
}


/* ====================================================================
 *      0x 0f 38 f0
 * ==================================================================== */
void __bea_callspec__ crc32_GvEb(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+ACCELERATOR_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "crc32 ");
        #endif
        if (GV.OperandSize == 64) {
            GV.MemDecoration = Arg2byte;
            GV.OperandSize = 8;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.OperandSize = 64;
        }
        else if (GV.OperandSize == 32) {
            GV.MemDecoration = Arg2byte;
            GV.OperandSize = 8;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.OperandSize = 32;
        }
        else {
            GV.MemDecoration = Arg2byte;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        }

        if (GV.OperandSize == 16) {
            GV.OperandSize = 32;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.OperandSize = 16;
        }
        else {
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        }
        GV.EIP_ += GV.DECALAGE_EIP+2;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 f1
 * ==================================================================== */
void __bea_callspec__ crc32_GvEv(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+ACCELERATOR_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "crc32 ");
        #endif

        if (GV.OperandSize == 64) {
            GV.MemDecoration = Arg2qword;
        }
        else if (GV.OperandSize == 32) {
            GV.MemDecoration = Arg2dword;
        }
        else {
            GV.MemDecoration = Arg2word;
        }
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);

        if (GV.OperandSize == 16) {
            GV.OperandSize = 32;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.OperandSize = 16;
        }
        else {
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        }
        GV.EIP_ += GV.DECALAGE_EIP+2;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 2f
 * ==================================================================== */
void __bea_callspec__ comiss_VW(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "comisd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "comiss ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}

/* ====================================================================
 *      0x 0f 5a
 * ==================================================================== */
void __bea_callspec__ cvtps2pd_(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==3)) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vcvtsd2ss ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtsd2ss ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========== 0xf3 */
    else if (GV.PrefRepe == 1|| (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vcvtss2sd ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtss2sd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix|| (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vcvtpd2ps ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtpd2ps ");
        #endif
	if(GV.VEX.has_vex && GV.VEX.length==1)
	{
        	GV.AVX_ = 1;
		GV.MemDecoration=Arg2yword;
    		MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        	GV.AVX_ = 0;
        	GV.SSE_ = 1;
    		Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        	GV.SSE_ = 0;
    		GV.EIP_ += GV.DECALAGE_EIP+2;

	}
	else
	{
        	GV.SSE_ = 1;
		GV.MemDecoration=Arg2oword;
        	GxEx(pMyDisasm);
        	GV.SSE_ = 0;
	}
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtps2pd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 5b
 * ==================================================================== */
void __bea_callspec__ cvtdq2ps_(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
        if(GV.VEX.has_vex)
        {
                (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        }
        #endif

    /* ========== 0xf3 */
    if (GV.PrefRepe == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "cvttps2dq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "cvtps2dq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "cvtdq2ps ");
        #endif

        	GV.AVX_ = GV.VEX.length;
        	GV.SSE_ = !GV.VEX.length;
        	GxEx(pMyDisasm);
        	GV.SSE_ = 0;
        	GV.AVX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 2a
 * ==================================================================== */
void __bea_callspec__ cvtpi2ps_(PDISASM pMyDisasm)
{
    /* ========= VEX 0xf2 */
    if (GV.VEX.has_vex && GV.VEX.implicit_prefixes==3)
    {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vcvtsi2sd ");
        #endif


	if(GV.VEX.W==0 || GV.Architecture==32)
	{
/************************************************************
VEX.NDS.LIG.F2.0F.W0 2A /r
VCVTSI2SD xmm1, xmm2, r/m32
RVM V/V AVX Convert one signed doubleword integer from
r/m32 to one double-precision floating-point
value in xmm1.
NOTES:
1. Encoding the VEX prefix with VEX.W=1 in non-64-bit mode is ignored.
AVX Convert one signed quadword integer from
r/m64 to one double-precision floating-point
value in xmm1
*************************************************************/
		GV.MemDecoration=Arg3dword;
        	GV.SSE_ = 1;
		V_reg(&(*pMyDisasm).Argument2, pMyDisasm);
        	Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
		GV.OperandSize=GV.OriginalOperandSize;
		GV.SSE_ = 0;
        	MOD_RM(&(*pMyDisasm).Argument3, pMyDisasm);
	}
	else if(GV.VEX.W==1)
	{
		GV.MemDecoration=Arg3qword;
        	GV.SSE_ = 1;
		V_reg(&(*pMyDisasm).Argument2, pMyDisasm);
        	Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
		GV.SSE_ = 0;
		GV.OperandSize=64;
        	MOD_RM(&(*pMyDisasm).Argument3, pMyDisasm);
/************************************************************
VEX.NDS.LIG.F2.0F.W1 2A /r
VCVTSI2SD xmm1, xmm2, r/m64
RVM V/N.E.1
*************************************************************/
	}
	else
		assert(0);

        GV.EIP_+= GV.DECALAGE_EIP+2;
	GV.third_arg=1;
	
    }
    /* ========= 0xf2 */
    else if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtsi2sd ");
        #endif
        if (GV.REX.W_ == 1) {
            GV.MemDecoration = Arg2qword;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 1;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
        else {
            GV.MemDecoration = Arg2dword;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 1;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
    }
    /* ========== 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtsi2ss ");
        #endif
        if (GV.REX.W_ == 1) {
            GV.MemDecoration = Arg2qword;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 1;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
        else {
            GV.MemDecoration = Arg2dword;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 1;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtpi2pd ");
        #endif
        GV.MemDecoration = Arg2qword;
        GV.MMX_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.MMX_ = 0;
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
    else {
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtpi2ps ");
        #endif
        GV.MemDecoration = Arg2qword;
        GV.MMX_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.MMX_ = 0;
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
}


/* ====================================================================
 *      0x 0f 2d
 * ==================================================================== */
void __bea_callspec__ cvtps2pi_(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtsd2si ");
        #endif
        if (GV.REX.W_ == 1) {
            GV.MemDecoration = Arg2qword;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 0;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
        else {
            GV.MemDecoration = Arg2qword;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 0;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
    }
    /* ========== 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtss2si ");
        #endif
        if (GV.REX.W_ == 1) {
            GV.MemDecoration = Arg2dword;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 0;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
        else {
            GV.MemDecoration = Arg2dword;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 0;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtpd2pi ");
        #endif
        GV.MemDecoration = Arg2dqword;
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.MMX_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.MMX_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
    else {
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtps2pi ");
        #endif
        GV.MemDecoration = Arg2qword;
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.MMX_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.MMX_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
}


/* ====================================================================
 *      0x 0f 2c
 * ==================================================================== */
void __bea_callspec__ cvttps2pi_(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvttsd2si ");
        #endif
        if (GV.REX.W_ == 1) {
            GV.MemDecoration = Arg2qword;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 0;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
        else {
            GV.MemDecoration = Arg2qword;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            GV.SSE_ = 0;
            Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.EIP_+= GV.DECALAGE_EIP+2;
        }
    }
    /* ========== 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvttss2si ");
        #endif
        GV.MemDecoration = Arg2dword;
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvttpd2pi ");
        #endif
        GV.MemDecoration = Arg2dqword;
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.MMX_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.MMX_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
    else {
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvttps2pi ");
        #endif
        GV.MemDecoration = Arg2qword;
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.MMX_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.MMX_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
}


/* ====================================================================
 *      0x 0f e6
 * ==================================================================== */
void __bea_callspec__ cvtpd2dq_(PDISASM pMyDisasm)
{
    /* ========== 0xf2 */
    if (GV.PrefRepne == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==3)) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vcvtpd2dq ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtpd2dq ");
        #endif
	if(GV.VEX.has_vex && GV.VEX.length==1)
	{
        	GV.AVX_ = 1;
		GV.MemDecoration=Arg2yword;
    		MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        	GV.AVX_ = 0;
        	GV.SSE_ = 1;
    		Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        	GV.SSE_ = 0;
    		GV.EIP_ += GV.DECALAGE_EIP+2;

	}
	else
	{
        	GV.SSE_ = 1;
                GV.MemDecoration=Arg2oword;
        	GxEx(pMyDisasm);
        	GV.SSE_ = 0;
	}
    }
    /* ========== 0xf3 */
    else if (GV.PrefRepe == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vcvtdq2pd ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvtdq2pd ");
        #endif
	if(GV.VEX.has_vex && GV.VEX.length==1)
	{
		GV.MemDecoration=Arg2fword;
        	GV.SSE_ = 1;
    		MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        	GV.SSE_ = 0;

        	GV.AVX_ = 1;
    		Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        	GV.AVX_ = 0;
    		GV.EIP_ += GV.DECALAGE_EIP+2;

	}
	else
	{
        	GV.SSE_ = 1;
                GV.MemDecoration=Arg2fword;
        	GxEx(pMyDisasm);
        	GV.SSE_ = 0;
	}
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vcvttpd2dq ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "cvttpd2dq ");
        #endif
	if(GV.VEX.has_vex && GV.VEX.length==1)
	{
        	GV.AVX_ = 1;
		GV.MemDecoration=Arg2yword;
    		MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        	GV.AVX_ = 0;
        	GV.SSE_ = 1;
    		Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        	GV.SSE_ = 0;
    		GV.EIP_ += GV.DECALAGE_EIP+2;

	}
	else
	{
        	GV.SSE_ = 1;
                GV.MemDecoration=Arg2oword;
        	GxEx(pMyDisasm);
        	GV.SSE_ = 0;
	}
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 3a 41
 * ==================================================================== */
void __bea_callspec__ dppd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+DOT_PRODUCT;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "dppd ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}

/* ====================================================================
 *      0x 0f 3a 40
 * ==================================================================== */
void __bea_callspec__ dpps_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+DOT_PRODUCT;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "dpps ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 5e
 * ==================================================================== */
void __bea_callspec__ divps_VW(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "divsd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "divss ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "divpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "divps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 3a 17
 * ==================================================================== */
void __bea_callspec__ extractps_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg1dword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+INSERTION_EXTRACTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "extractps ");
        #endif
        GV.ImmediatSize = 8;
        MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 7c
 * ==================================================================== */
void __bea_callspec__ haddpd_VW(PDISASM pMyDisasm)
{

    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+SIMD_FP_HORIZONTAL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "haddpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+SIMD_FP_HORIZONTAL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "haddps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 7d
 * ==================================================================== */
void __bea_callspec__ hsubpd_VW(PDISASM pMyDisasm)
{

    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+SIMD_FP_HORIZONTAL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "hsubpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+SIMD_FP_HORIZONTAL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "hsubps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 3a 21
 * ==================================================================== */
void __bea_callspec__ insertps_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+INSERTION_EXTRACTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "insertps ");
        #endif
        GV.SSE_ = 1;
        GV.MOD_= ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 6) & 0x3;
        if (GV.MOD_== 0x3) {
            GV.MemDecoration = Arg2qword;
        }
        else {
            GV.MemDecoration = Arg2dword;
        }

        GV.ImmediatSize = 8;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);


        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}



/* ====================================================================
 *      0x 0f f0
 * ==================================================================== */
void __bea_callspec__ lddqu_(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+SPECIALIZED_128bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "lddqu ");
        #endif
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_+= GV.DECALAGE_EIP+2;
    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f f7
 * ==================================================================== */
void __bea_callspec__ maskmovq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CACHEABILITY_CONTROL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "maskmovdqu ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+CACHEABILITY_CONTROL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "maskmovq ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 5f
 * ==================================================================== */
void __bea_callspec__ maxps_VW(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "maxsd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "maxss ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "maxpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "maxps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 5d
 * ==================================================================== */
void __bea_callspec__ minps_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==3)) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "minsd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "minss ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "minpd ");
        #endif

	if(GV.VEX.has_vex)
        {
                GV.AVX_ = GV.VEX.length;
                GV.SSE_ = !GV.VEX.length;
                GV.MemDecoration=Arg3fword;
                V_reg(&(*pMyDisasm).Argument2, pMyDisasm);
                MOD_RM(&(*pMyDisasm).Argument3, pMyDisasm);
                GV.third_arg=1;
                Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
                GV.AVX_ = 0;
                GV.SSE_ = 0;
                GV.EIP_ += GV.DECALAGE_EIP+2;

	}
	else
	{
        	GV.SSE_ = 1;
        	GxEx(pMyDisasm);
        	GV.SSE_ = 0;
	}
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "minps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}

/* ====================================================================
 *      0x 0f 28
 * ==================================================================== */
void __bea_callspec__ movaps_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
	#endif

    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movapd ");
        #endif
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movaps ");
        #endif
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
}

/* ====================================================================
 *      0x 0f 29
 * ==================================================================== */
void __bea_callspec__ movaps_WV(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
	#endif
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
//	assert(!GV.VEX.has_vex);	 /* FIXME */
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movapd ");
        #endif
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
    else {
        GV.MemDecoration = Arg1fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movaps ");
        #endif
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 16
 * ==================================================================== */
void __bea_callspec__ movhps_VM(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif
    /* ========= 0xf3 */
    if (GV.PrefRepe == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movshdup ");
        #endif
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movhpd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+DATA_TRANSFER;
        GV.MOD_= ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 6) & 0x3;
        if (GV.MOD_== 0x3) {
            #ifndef BEA_LIGHT_DISASSEMBLY
               (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movlhps ");
            #endif
        }
        else {
            #ifndef BEA_LIGHT_DISASSEMBLY
               (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movhps ");
            #endif
        }
    }
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
	if(GV.VEX.has_vex)
        	VxGxEx(pMyDisasm);
	else
        	GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
}


/* ====================================================================
 *      0x 0f 17
 * ==================================================================== */
void __bea_callspec__ movhps_MV(PDISASM pMyDisasm)
{

    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movhpd ");
        #endif
        GV.SSE_ = 1;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg1fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movhps ");
        #endif
        GV.SSE_ = 1;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 12
 * ==================================================================== */
void __bea_callspec__ movlps_VM(PDISASM pMyDisasm)
{

        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==3)) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movddup ");
        #endif
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
	if(GV.VEX.has_vex)
		VxGxEx(pMyDisasm);
	else
        	GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE3_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movsldup ");
        #endif
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
	if(GV.VEX.has_vex)
		VxGxEx(pMyDisasm);
	else
        	GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movlpd ");
        #endif
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
	if(GV.VEX.has_vex)
		VxGxEx(pMyDisasm);
	else
        	GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
    else {
	GV.MemDecoration = Arg2fword; 
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);

        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+DATA_TRANSFER;
        if (GV.MOD_== 0x3) {
            #ifndef BEA_LIGHT_DISASSEMBLY
               (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movhlps ");
            #endif
        }
        else {
            #ifndef BEA_LIGHT_DISASSEMBLY
               (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movlps ");
            #endif
        }
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
	if(GV.VEX.has_vex)
		VxGxEx(pMyDisasm);
	else
        	GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 13
 * ==================================================================== */
void __bea_callspec__ movlps_MV(PDISASM pMyDisasm)
{

    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movlpd ");
        #endif
        GV.SSE_ = 1;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg1fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movlps ");
        #endif
        GV.SSE_ = 1;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 50
 * ==================================================================== */
void __bea_callspec__ movmskps_(PDISASM pMyDisasm)
{
    GV.MOD_= ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 6) & 0x3;
    if (GV.MOD_!= 0x3) {
        FailDecode(pMyDisasm);
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movmskpd ");
        #endif
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_ += GV.DECALAGE_EIP+2;

    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+DATA_TRANSFER;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movmskps ");
        #endif
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_ += GV.DECALAGE_EIP+2;

    }
}


/* ====================================================================
 *      0x 0f 38 2a
 * ==================================================================== */
void __bea_callspec__ movntdqa_(PDISASM pMyDisasm)
{

    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+STREAMING_LOAD;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movntdqa ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f c3
 * ==================================================================== */
void __bea_callspec__ movnti_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CACHEABILITY_CONTROL;
    #ifndef BEA_LIGHT_DISASSEMBLY
       (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movnti ");
    #endif
    EvGv(pMyDisasm);

}


/* ====================================================================
 *      0x 0f 2b
 * ==================================================================== */
void __bea_callspec__ movntps_(PDISASM pMyDisasm)
{
    GV.MOD_= ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 6) & 0x3;
    if (GV.MOD_== 0x3) {
        FailDecode(pMyDisasm);
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CACHEABILITY_CONTROL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movntpd ");
        #endif
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_ += GV.DECALAGE_EIP+2;

    }
    else {
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CACHEABILITY_CONTROL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movntps ");
        #endif
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_ += GV.DECALAGE_EIP+2;

    }
}


/* ====================================================================
 *      0x 0f e7
 * ==================================================================== */
void __bea_callspec__ movntq_(PDISASM pMyDisasm)
{
    GV.MOD_= ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 6) & 0x3;
    if (GV.MOD_== 0x3) {
        FailDecode(pMyDisasm);
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+CACHEABILITY_CONTROL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movntdq ");
        #endif
        GV.SSE_ = 1;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg1qword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+CACHEABILITY_CONTROL;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "movntq ");
        #endif
        GV.MMX_ = 1;
        ExGx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 10
 * ==================================================================== */
void __bea_callspec__ movups_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
        if(GV.VEX.has_vex)
                (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif

    /* ========= 0xf2 */
    if (GV.PrefRepne == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==3)) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movsd ");
        #endif
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1 || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==2)) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movss ");
        #endif
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movupd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movups ");
        #endif
    }
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
}



/* ====================================================================
 *      0x 0f 11
 * ==================================================================== */
void __bea_callspec__ movups_WV(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
        if(GV.VEX.has_vex)
                (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movsd ");
        #endif
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg1fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movss ");
        #endif
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movupd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg1dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "movups ");
        #endif
    }
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
        ExGx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
}


/* ====================================================================
 *      0x 0f 3a 42
 * ==================================================================== */
void __bea_callspec__ mpsadbw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+SAD_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "mpsadbw ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 59
 * ==================================================================== */
void __bea_callspec__ mulps_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;

	/* seems nasm doesn't like memory declarations on mulsd */
        GV.MemDecoration = Arg2fword; 
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "mulsd ");
        #endif
    }
    /* ========== 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "mulss ");
        #endif
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "mulpd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "mulps ");
        #endif
    }
        GV.AVX_ = GV.VEX.length;
        GV.SSE_ = !GV.VEX.length;
	if(GV.VEX.has_vex)
	{
		GV.MemDecoration+=100;
        	VxGxEx(pMyDisasm);
	}
	else
        	GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.AVX_ = 0;
}


/* ====================================================================
 *      0x 0f 56
 * ==================================================================== */
void __bea_callspec__ orps_VW(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "orpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "orps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 2b
 * ==================================================================== */
void __bea_callspec__ packusdw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "packusdw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;

    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f d4
 * ==================================================================== */
void __bea_callspec__ paddq_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SIMD128bits;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "paddq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "paddq ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f e0
 * ==================================================================== */
void __bea_callspec__ pavgb_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "pavgb ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    else {
        GV.MemDecoration = Arg2qword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pavgb ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f e3
 * ==================================================================== */
void __bea_callspec__ pavgw_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "pavgw ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    else {
        GV.MemDecoration = Arg2qword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pavgw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 3a 0f
 * ==================================================================== */
void __bea_callspec__ palignr_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "palignr ");
        #endif
        GV.ImmediatSize = 8;
        Vx_opt_GxEx_vexlen(pMyDisasm);
	GV.third_arg=1;
	if(GV.VEX.has_vex)
	{
		L_imm(&(*pMyDisasm).Argument4,pMyDisasm);
		GV.forth_arg=1;
	}
	else
		L_imm(&(*pMyDisasm).Argument3,pMyDisasm);
	

    }
    else {
        GV.MemDecoration = Arg2qword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "palignr ");
        #endif
        GV.ImmediatSize = 8;
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
	L_imm(&(*pMyDisasm).Argument3,pMyDisasm);

    }
}


/* ====================================================================
 *      0x 0f 38 10
 * ==================================================================== */
void __bea_callspec__ pblendvb_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_BLENDING_INSTRUCTION;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pblendvb ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pblendvb ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }

    #ifndef BEA_LIGHT_DISASSEMBLY
       (void) strcpy ((*pMyDisasm).Argument3.ArgMnemonic, RegistersSSE[0]);
    #endif
    (*pMyDisasm).Argument3.ArgType = REGISTER_TYPE+SSE_REG+REG0;
    (*pMyDisasm).Argument3.ArgSize = 8;
    GV.third_arg=1;
 
}


/* ====================================================================
 *      0x 0f 3a 0e
 * ==================================================================== */
void __bea_callspec__ pblendw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+SAD_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pblendw ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 38 29
 * ==================================================================== */
void __bea_callspec__ pcmpeqq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_EQUALITY;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pcmpeqq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;

    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 61
 * ==================================================================== */
void __bea_callspec__ pcmpestri_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pcmpestri ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 60
 * ==================================================================== */
void __bea_callspec__ pcmpestrm_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pcmpestrm ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 63
 * ==================================================================== */
void __bea_callspec__ pcmpistri_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pcmpistri ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 62
 * ==================================================================== */
void __bea_callspec__ pcmpistrm_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pcmpestrm ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 38 37
 * ==================================================================== */
void __bea_callspec__ pcmpgtq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pcmpgtq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;

    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 14
 * ==================================================================== */
void __bea_callspec__ pextrb_(PDISASM pMyDisasm)
{
    UInt8 oldRexW=GV.REX.W_;
    int oldOperandSize=GV.OperandSize;

    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {

        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+INSERTION_EXTRACTION;
	if(GV.VEX.has_vex && GV.VEX.W!=0)
		FailDecode(pMyDisasm);
	
    	GV.REX.W_=0;	/* REX.w ignored for pextrb */

        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vpextrb ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pextrb ");
        #endif
	if(GV.VEX.has_vex)
		GV.OperandSize=64;
	else
		GV.OperandSize=32;
        GV.MOD_= ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 6) & 0x3;
        if (GV.MOD_== 0x3) {
            GV.MemDecoration = Arg1dword;
        }
        else {
            GV.MemDecoration = Arg1byte;
        }
        GV.ImmediatSize = 8;
        MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
	GV.OperandSize=oldOperandSize;
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_ += GV.DECALAGE_EIP+3;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    	GV.REX.W_=oldRexW;	/* restore REX.w if it was ignored */
    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 16
 * ==================================================================== */
void __bea_callspec__ pextrd_(PDISASM pMyDisasm)
{
    int oldOperandSize=GV.OperandSize;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+INSERTION_EXTRACTION;
        if (GV.REX.W_ == 0x1 || GV.VEX.W==0x1) {
            #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vpextrq ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pextrq ");
            #endif
            GV.MemDecoration = Arg1qword;
            GV.OperandSize = 64;
        }
        else {
            #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vpextrd ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pextrd ");
            #endif
            GV.MemDecoration = Arg1dword;
        }
        GV.ImmediatSize = 8;
	/* pextrq should always be size=64.  pextrd needs toc hange based on the v- or not v-variant */
        if (!(GV.REX.W_ == 0x1 || GV.VEX.W==0x1)) {
	if(GV.VEX.has_vex)
		GV.OperandSize=64;
	else
		GV.OperandSize=32;
	}
        MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
	GV.OperandSize=oldOperandSize;
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_ += GV.DECALAGE_EIP+3;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%x",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}

/* ====================================================================
 *      0x 0f c5
 * ==================================================================== */
void __bea_callspec__ pextrw_(PDISASM pMyDisasm)
{
    int oldOperandSize=GV.OperandSize;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {

        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vpextrw ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pextrw ");
        #endif
        GV.MemDecoration = Arg2dqword;
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
	if(GV.VEX.has_vex)
		GV.OperandSize=64;
	else
		GV.OperandSize=32;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
	GV.OperandSize=oldOperandSize;
        GV.EIP_ += GV.DECALAGE_EIP+2;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
	L_imm(&(*pMyDisasm).Argument3, pMyDisasm);


    }
    else {
	if(GV.VEX.has_vex) FailDecode(pMyDisasm);
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pextrw ");
        #endif
        GV.MemDecoration = Arg2dqword;
        GV.ImmediatSize = 8;
        GV.MMX_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.MMX_ = 0;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_ += GV.DECALAGE_EIP+2;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
	L_imm(&(*pMyDisasm).Argument3, pMyDisasm);

    }

}

/* ====================================================================
 *      0x 0f 3a 15
 * ==================================================================== */
void __bea_callspec__ pextrw2_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix || (GV.VEX.has_vex && GV.VEX.implicit_prefixes==1)) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+INSERTION_EXTRACTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "vpextrw ");
		else
           		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pextrw ");
        #endif
        GV.MOD_= ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 6) & 0x3;
        if (GV.MOD_== 0x3) {
            GV.MemDecoration = Arg1dword;
        }
        else {
            GV.MemDecoration = Arg1word;
        }
        GV.ImmediatSize = 8;
        MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_ += GV.DECALAGE_EIP+2;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
	L_imm(&(*pMyDisasm).Argument3, pMyDisasm);


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 38 02
 * ==================================================================== */
void __bea_callspec__ phaddd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phaddd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phaddd ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 03
 * ==================================================================== */
void __bea_callspec__ phaddsw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phaddsw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phaddsw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 01
 * ==================================================================== */
void __bea_callspec__ phaddw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phaddw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phaddw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 41
 * ==================================================================== */
void __bea_callspec__ phminposuw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+HORIZONTAL_SEARCH;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phminposuw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 05
 * ==================================================================== */
void __bea_callspec__ phsubw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phsubw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phsubw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 06
 * ==================================================================== */
void __bea_callspec__ phsubd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phsubd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phsubd ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 07
 * ==================================================================== */
void __bea_callspec__ phsubsw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phsubsw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "phsubsw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 3a 20
 * ==================================================================== */
void __bea_callspec__ pinsrb_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2byte;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+INSERTION_EXTRACTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pinsrb ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 3a 22
 * ==================================================================== */
void __bea_callspec__ pinsrd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+INSERTION_EXTRACTION;
        if (GV.REX.W_ == 0x1) {
            #ifndef BEA_LIGHT_DISASSEMBLY
               (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pinsrq ");
            #endif
            GV.MemDecoration = Arg2qword;
        }
        else {
            #ifndef BEA_LIGHT_DISASSEMBLY
               (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pinsrd ");
            #endif
            GV.MemDecoration = Arg2dword;
        }
        GV.ImmediatSize = 8;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_ += GV.DECALAGE_EIP+3;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;


    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f c4
 * ==================================================================== */
void __bea_callspec__ pinsrw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pinsrw ");
        #endif
        GV.MemDecoration = Arg2word;
        GV.ImmediatSize = 8;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_ += GV.DECALAGE_EIP+2;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
	L_imm(&(*pMyDisasm).Argument3, pMyDisasm);


    }
    else {
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pinsrw ");
        #endif
        GV.MemDecoration = Arg2word;
        GV.ImmediatSize = 8;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.MMX_ = 1;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.MMX_ = 0;
        GV.EIP_ += GV.DECALAGE_EIP+2;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
	L_imm(&(*pMyDisasm).Argument3, pMyDisasm);

    }

}


/* ====================================================================
 *      0x 0f 38 3c
 * ==================================================================== */
void __bea_callspec__ pmaxsb_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxsb ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 3d
 * ==================================================================== */
void __bea_callspec__ pmaxsd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxsd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 3e
 * ==================================================================== */
void __bea_callspec__ pmaxuw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxuw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 3f
 * ==================================================================== */
void __bea_callspec__ pmaxud_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxud ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 38
 * ==================================================================== */
void __bea_callspec__ pminsb_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminsb ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 39
 * ==================================================================== */
void __bea_callspec__ pminsd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminsd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 3a
 * ==================================================================== */
void __bea_callspec__ pminuw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminuw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}

/* ====================================================================
 *      0x 0f 38 3b
 * ==================================================================== */
void __bea_callspec__ pminud_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_MINMAX;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminud ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f da
 * ==================================================================== */
void __bea_callspec__ pminub_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminub ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminub ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f de
 * ==================================================================== */
void __bea_callspec__ pmaxub_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxub ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxub ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f ea
 * ==================================================================== */
void __bea_callspec__ pminsw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminsw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pminsw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f ee
 * ==================================================================== */
void __bea_callspec__ pmaxsw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxsw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaxsw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 04
 * ==================================================================== */
void __bea_callspec__ pmaddubsw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaddubsw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmaddubsw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f d7
 * ==================================================================== */
void __bea_callspec__ pmovmskb_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {

        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovmskb ");
        #endif
        GV.MemDecoration = Arg2dqword;
        GV.SSE_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.SSE_ = 0;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_ += GV.DECALAGE_EIP+2;
    }
    else {
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovmskb ");
        #endif
        GV.MemDecoration = Arg2qword;
        GV.MMX_ = 1;
        MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
        GV.MMX_ = 0;
        Reg_Opcode(&(*pMyDisasm).Argument1, pMyDisasm);
        GV.EIP_ += GV.DECALAGE_EIP+2;
    }

}


/* ====================================================================
 *      0x 0f 38 21
 * ==================================================================== */
void __bea_callspec__ pmovsxbd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovsxbd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 22
 * ==================================================================== */
void __bea_callspec__ pmovsxbq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovsxbq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 20
 * ==================================================================== */
void __bea_callspec__ pmovsxbw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovsxbw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 25
 * ==================================================================== */
void __bea_callspec__ pmovsxdq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovsxdq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 23
 * ==================================================================== */
void __bea_callspec__ pmovsxwd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovsxwd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 24
 * ==================================================================== */
void __bea_callspec__ pmovsxwq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovsxwq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 31
 * ==================================================================== */
void __bea_callspec__ pmovzxbd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovzxbd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 32
 * ==================================================================== */
void __bea_callspec__ pmovzxbq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovzxbq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 30
 * ==================================================================== */
void __bea_callspec__ pmovzxbw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovzxbw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 35
 * ==================================================================== */
void __bea_callspec__ pmovzxdq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovzxdq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 33
 * ==================================================================== */
void __bea_callspec__ pmovzxwd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovzxwd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 34
 * ==================================================================== */
void __bea_callspec__ pmovzxwq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+CONVERSION_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmovzxwq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 28
 * ==================================================================== */
void __bea_callspec__ pmuldq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmuldq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 40
 * ==================================================================== */
void __bea_callspec__ pmulld_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmulld ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 38 0b
 * ==================================================================== */
void __bea_callspec__ pmulhrsw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmulhrsw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmulhrsw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f e4
 * ==================================================================== */
void __bea_callspec__ pmulhuw_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmulhuw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmulhuw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f f4
 * ==================================================================== */
void __bea_callspec__ pmuludq_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmuludq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pmuludq ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* =======================================
 *      0x 0f b8
 * ======================================= */
void __bea_callspec__ popcnt_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE42_INSTRUCTION+DATA_TRANSFER;
    #ifndef BEA_LIGHT_DISASSEMBLY
       (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "popcnt ");
    #endif
    GvEv(pMyDisasm);
    FillFlags(pMyDisasm,114);
}


/* ====================================================================
 *      0x 0f f6
 * ==================================================================== */
void __bea_callspec__ psadbw_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SIMD64bits;
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psadbw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2fword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psadbw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 00
 * ==================================================================== */
void __bea_callspec__ pshufb_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "pshufb ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pshufb ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 70
 * ==================================================================== */
void __bea_callspec__ pshufw_(PDISASM pMyDisasm)
{
    (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SIMD128bits;
    /* ========= 0xf3 */
    if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword; 
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pshufhw ");
        #endif
        GV.ImmediatSize = 8;
        Vx_opt_GxEx_vexlen_imm(pMyDisasm);

    }
    /* ========= 0xf2 */
    else if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pshuflw ");
        #endif
        GV.ImmediatSize = 8;
        Vx_opt_GxEx_vexlen_imm(pMyDisasm);

    }

    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pshufd ");
        #endif
        GV.ImmediatSize = 8;
        Vx_opt_GxEx_vexlen_imm(pMyDisasm);

    }
    else {
        GV.MemDecoration = Arg2fword;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pshufw ");
        #endif
        GV.ImmediatSize = 8;
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%x",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;

    }
}

/* ====================================================================
 *      0x 0f 38 08
 * ==================================================================== */
void __bea_callspec__ psignb_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+PACKED_SIGN;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psignb ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+PACKED_SIGN;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psignb ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 0a
 * ==================================================================== */
void __bea_callspec__ psignd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+PACKED_SIGN;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psignd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+PACKED_SIGN;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psignd ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 09
 * ==================================================================== */
void __bea_callspec__ psignw_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+PACKED_SIGN;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psignw ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSSE3_INSTRUCTION+PACKED_SIGN;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psignw ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f fb
 * ==================================================================== */
void __bea_callspec__ psubq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SIMD128bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psubq ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2qword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SIMD128bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psubq ");
        #endif
        GV.MMX_ = 1;
        GxEx(pMyDisasm);
        GV.MMX_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 38 17
 * ==================================================================== */
void __bea_callspec__ ptest_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+PACKED_TEST;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "ptest ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        FailDecode(pMyDisasm);
    }

}

/* ====================================================================
 *      0x 0f 6c
 * ==================================================================== */
void __bea_callspec__ punpcklqdq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SIMD128bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "punpcklqdq ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    else {
        FailDecode(pMyDisasm);
    }

}

/* ====================================================================
 *      0x 0f 6d
 * ==================================================================== */
void __bea_callspec__ punpckhqdq_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SIMD128bits;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "punpckhqdq ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    else {
        FailDecode(pMyDisasm);
    }

}


/* ====================================================================
 *      0x 0f 53
 * ==================================================================== */
void __bea_callspec__ rcpps_(PDISASM pMyDisasm)
{
    /* ========== 0xf3 */
    if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2dword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "rcpss ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "rcpps ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 3a 09
 * ==================================================================== */
void __bea_callspec__ roundpd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+ROUND_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "roundpd ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;

    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 3a 08
 * ==================================================================== */
void __bea_callspec__ roundps_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+ROUND_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "roundps ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;

    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 3a 0b
 * ==================================================================== */
void __bea_callspec__ roundsd_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+ROUND_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "roundsd ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;

    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 3a 0a
 * ==================================================================== */
void __bea_callspec__ roundss_(PDISASM pMyDisasm)
{
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dword;
        (*pMyDisasm).Instruction.Category = SSE41_INSTRUCTION+ROUND_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "roundss ");
        #endif
        GV.ImmediatSize = 8;
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
        GV.EIP_++;
        if (!Security(0, pMyDisasm)) return;
        GV.third_arg = 1;
        (*pMyDisasm).Instruction.Immediat = *((UInt8*)(UIntPtr) (GV.EIP_- 1));
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) CopyFormattedNumber(pMyDisasm, (char*) (*pMyDisasm).Argument3.ArgMnemonic, "%.2X",(Int64) *((UInt8*)(UIntPtr) (GV.EIP_- 1)));
        #endif
        (*pMyDisasm).Argument3.ArgType = CONSTANT_TYPE+ABSOLUTE_;
        (*pMyDisasm).Argument3.ArgSize = 8;

    }
    else {
        FailDecode(pMyDisasm);
    }
}


/* ====================================================================
 *      0x 0f 52
 * ==================================================================== */
void __bea_callspec__ rsqrtps_(PDISASM pMyDisasm)
{
    /* ========== 0xf3 */
    if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "rsqrtss ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "rsqrtps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f c6
 * ==================================================================== */
void __bea_callspec__ shufps_(PDISASM pMyDisasm)
{

    /* ========== 0x66 */
    GV.ImmediatSize = 8;
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "shufpd ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "shufps ");
        #endif
        Vx_opt_GxEx_vexlen(pMyDisasm);
    }
    	(*pMyDisasm).Argument1.AccessMode = READ;
    	if (!Security(0, pMyDisasm)) return;
    	GV.third_arg = 1;

	if(GV.VEX.has_vex)
	{
    		GV.forth_arg = 1;
		L_imm(&pMyDisasm->Argument4,pMyDisasm);
	}
	else
	{
		L_imm(&pMyDisasm->Argument3,pMyDisasm);
	}
}


/* ====================================================================
 *      0x 0f 51
 * ==================================================================== */
void __bea_callspec__ sqrtps_VW(PDISASM pMyDisasm)
{
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "sqrtsd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "sqrtss ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "sqrtpd ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "sqrtps ");
        #endif
        GV.SSE_ = 1;
        GxEx(pMyDisasm);
        GV.SSE_ = 0;
    }
}


/* ====================================================================
 *      0x 0f 5c
 * ==================================================================== */
void __bea_callspec__ subps_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
        #endif
    /* ========= 0xf2 */
    if (GV.PrefRepne == 1) {
        (*pMyDisasm).Prefix.RepnePrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "subsd ");
        #endif
    }
    /* ========= 0xf3 */
    else if (GV.PrefRepe == 1) {
        (*pMyDisasm).Prefix.RepPrefix = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "subss ");
        #endif
    }
    /* ========== 0x66 */
    else if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "subpd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+ARITHMETIC_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "subps ");
        #endif
    }
    Vx_opt_GxEx_vexlen(pMyDisasm);
}


/* ====================================================================
 *      0x 0f 2e
 * ==================================================================== */
void __bea_callspec__ ucomiss_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
	#endif
    /* ========== 0x66 */
    if (pMyDisasm->Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2fword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "ucomisd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+COMPARISON_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "ucomiss ");
        #endif
    }
        Vx_opt_GxEx_vexlen(pMyDisasm);
}


/* ====================================================================
 *      0x 0f 15
 * ==================================================================== */
void __bea_callspec__ unpckhps_(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
	#endif
    /* ========== 0x66 */
    if (pMyDisasm->Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "unpckhpd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "unpckhps ");
        #endif
    }
        Vx_opt_GxEx_vexlen(pMyDisasm);
}

/* ====================================================================
 *      0x 0f 14
 * ==================================================================== */
void __bea_callspec__ unpcklps_(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
	#endif
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "unpcklpd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+SHUFFLE_UNPACK;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "unpcklps ");
        #endif
    }
        Vx_opt_GxEx_vexlen(pMyDisasm);
}


/* ====================================================================
 *      0x 0f 57
 * ==================================================================== */
void __bea_callspec__ xorps_VW(PDISASM pMyDisasm)
{
        #ifndef BEA_LIGHT_DISASSEMBLY
	if(GV.VEX.has_vex)
           (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
	#endif
    /* ========== 0x66 */
    if ((*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
        GV.OperandSize = GV.OriginalOperandSize;
        (*pMyDisasm).Prefix.OperandSize = MandatoryPrefix;
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE2_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "xorpd ");
        #endif
    }
    else {
        GV.MemDecoration = Arg2dqword;
        (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+LOGICAL_INSTRUCTION;
        #ifndef BEA_LIGHT_DISASSEMBLY
           (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "xorps ");
        #endif
    }
        Vx_opt_GxEx_vexlen(pMyDisasm);
}
