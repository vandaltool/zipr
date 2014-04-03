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

/* ====================================================================
 *
 * ==================================================================== */
void __bea_callspec__ G13_(PDISASM pMyDisasm)
{

    GV.REGOPCODE = ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 3) & 0x7;
    if (GV.REGOPCODE == 2) {
        if (GV.OperandSize == 16) {
            (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHIFT_ROTATE;
            GV.MemDecoration = Arg1dqword;
            GV.ImmediatSize = 8;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;
            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psrld ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.EIP_ += GV.DECALAGE_EIP+2;
            if (!Security(0, pMyDisasm)) return;

	    L_imm(&(*pMyDisasm).Argument2,pMyDisasm);
        }
        else {
            (*pMyDisasm).Instruction.Category = MMX_INSTRUCTION+SHIFT_ROTATE;
            GV.MemDecoration = Arg1qword;
            GV.ImmediatSize = 8;
            GV.MMX_ = 1;
            MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.MMX_ = 0;
            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psrld ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.EIP_ += GV.DECALAGE_EIP+2;
            if (!Security(0, pMyDisasm)) return;
	    L_imm(&(*pMyDisasm).Argument2,pMyDisasm);
        }
    }
    else if (GV.REGOPCODE == 4) {
        if (GV.OperandSize == 16 || (*pMyDisasm).Prefix.OperandSize==InUsePrefix) {
            	(*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHIFT_ROTATE;

		if(GV.VEX.has_vex)
		{
			V_reg(&(*pMyDisasm).Argument1, pMyDisasm);
            		GV.ImmediatSize = 8;
            		GV.AVX_ = GV.VEX.length;
            		GV.SSE_ = !GV.VEX.length;
            		MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            		GV.SSE_ = 0;
            		GV.AVX_ = 0;
            		GV.EIP_ += GV.DECALAGE_EIP+2;
            		if (!Security(0, pMyDisasm)) return;
            		L_imm(&(*pMyDisasm).Argument3, pMyDisasm);
			GV.third_arg=1;
		}
		else
		{
            		GV.ImmediatSize = 8;
            		GV.SSE_ = 1;
            		MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            		GV.SSE_ = 0;
            		GV.EIP_ += GV.DECALAGE_EIP+2;
            		if (!Security(0, pMyDisasm)) return;
            		L_imm(&(*pMyDisasm).Argument2, pMyDisasm);
		}

            	if (GV.MOD_== 0x3) {
               	#ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
               		(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
               	(void) strcat ((*pMyDisasm).Instruction.Mnemonic, "psrad ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }

        }
        else {
            (*pMyDisasm).Instruction.Category = MMX_INSTRUCTION+SHIFT_ROTATE;
            GV.MemDecoration = Arg1qword;
            GV.ImmediatSize = 8;
            GV.MMX_ = 1;
            MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.MMX_ = 0;
            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psrad ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.EIP_ += GV.DECALAGE_EIP+2;
            if (!Security(0, pMyDisasm)) return;

	    L_imm(&(*pMyDisasm).Argument2, pMyDisasm);
        }

    }
    else if (GV.REGOPCODE == 6) {
        if (GV.OperandSize == 16) {
            (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHIFT_ROTATE;
            GV.MemDecoration = Arg1dqword;
            GV.ImmediatSize = 8;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;
            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pslld ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.EIP_ += GV.DECALAGE_EIP+2;
            if (!Security(0, pMyDisasm)) return;

		L_imm(&(*pMyDisasm).Argument2, pMyDisasm);

        }
        else {
            (*pMyDisasm).Instruction.Category = MMX_INSTRUCTION+SHIFT_ROTATE;
            GV.MemDecoration = Arg1qword;
            GV.ImmediatSize = 8;
            GV.MMX_ = 1;
            MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.MMX_ = 0;
            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "pslld ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.EIP_ += GV.DECALAGE_EIP+2;
            if (!Security(0, pMyDisasm)) return;

		L_imm(&(*pMyDisasm).Argument2, pMyDisasm);
        }
    }

    else {
        FailDecode(pMyDisasm);
    }

}
