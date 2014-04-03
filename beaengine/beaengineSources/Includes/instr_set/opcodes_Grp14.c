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
 * 0f 73 /<const>
 * ==================================================================== */
void __bea_callspec__ G14_(PDISASM pMyDisasm)
{

    GV.REGOPCODE = ((*((UInt8*)(UIntPtr) (GV.EIP_+1))) >> 3) & 0x7;
    if (GV.REGOPCODE == 2) {
        if (GV.OperandSize == 16 || pMyDisasm->Prefix.OperandSize==InUsePrefix) {
            (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHIFT_ROTATE;
            GV.MemDecoration = Arg1dqword;
            GV.ImmediatSize = 8;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;

            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psrlq ");
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
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psrlq ");
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
    else if (GV.REGOPCODE == 3) {
        if (GV.OperandSize == 16 || pMyDisasm->Prefix.OperandSize==InUsePrefix) {
            (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHIFT_ROTATE;
            GV.ImmediatSize = 8;
            GV.AVX_ = GV.VEX.length;
            GV.SSE_ = !GV.VEX.length;
	    if(GV.VEX.has_vex)
		{
            		V_reg(&(*pMyDisasm).Argument1, pMyDisasm);
            		MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
            		GV.EIP_ += GV.DECALAGE_EIP+2;
            		if (!Security(0, pMyDisasm)) return;
	    		L_imm(&(*pMyDisasm).Argument3, pMyDisasm);
			GV.third_arg=1;
		}
		else
		{
            		MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            		GV.EIP_ += GV.DECALAGE_EIP+2;
            		if (!Security(0, pMyDisasm)) return;
	    		L_imm(&(*pMyDisasm).Argument2, pMyDisasm);
		}
            GV.SSE_ = 0;
            GV.AVX_ = 0;

            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
		if(GV.VEX.has_vex)
                   	(void) strcat ((*pMyDisasm).Instruction.Mnemonic, "v");
                   (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "psrldq ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }


        }
        else {
            FailDecode(pMyDisasm);
        }

    }
    else if (GV.REGOPCODE == 6) {
        if (GV.OperandSize == 16 || pMyDisasm->Prefix.OperandSize==InUsePrefix) {
            (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHIFT_ROTATE;
            GV.MemDecoration = Arg1dqword;
            GV.ImmediatSize = 8;
            GV.SSE_ = 1;
            MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            GV.SSE_ = 0;
            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psllq ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.EIP_ += GV.DECALAGE_EIP+2;
            if (!Security(0, pMyDisasm)) return;
		L_imm(&pMyDisasm->Argument2,pMyDisasm);
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
                   (void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "psllq ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.EIP_ += GV.DECALAGE_EIP+2;
            if (!Security(0, pMyDisasm)) return;
		L_imm(&pMyDisasm->Argument2,pMyDisasm);
        }
    }
    else if (GV.REGOPCODE == 7) {
        if (GV.OperandSize == 16 || (*pMyDisasm).Prefix.OperandSize == InUsePrefix) {
            (*pMyDisasm).Instruction.Category = SSE_INSTRUCTION+SHIFT_ROTATE;
            GV.ImmediatSize = 8;
            GV.AVX_ = GV.VEX.length;
            GV.SSE_ = !GV.VEX.length;


	    if(GV.VEX.has_vex)
	    {
            	GV.MemDecoration = Arg2dqword;
            	MOD_RM(&(*pMyDisasm).Argument2, pMyDisasm);
	    	V_reg(&(*pMyDisasm).Argument1, pMyDisasm);
		GV.third_arg=1;
            	GV.EIP_ += GV.DECALAGE_EIP+2;
            	if (!Security(0, pMyDisasm)) return;
	    	L_imm(&(*pMyDisasm).Argument3, pMyDisasm);
	    }
	    else
	    {
            	GV.MemDecoration = Arg1dqword;
            	MOD_RM(&(*pMyDisasm).Argument1, pMyDisasm);
            	GV.EIP_ += GV.DECALAGE_EIP+2;
            	if (!Security(0, pMyDisasm)) return;
	    	L_imm(&(*pMyDisasm).Argument2, pMyDisasm);
	    }
            if (GV.MOD_== 0x3) {
                #ifndef BEA_LIGHT_DISASSEMBLY
		   if(GV.VEX.has_vex)
                   	(void) strcpy ((*pMyDisasm).Instruction.Mnemonic, "v");
                   (void) strcat ((*pMyDisasm).Instruction.Mnemonic, "pslldq ");
                #endif
            }
            else {
                FailDecode(pMyDisasm);
            }
            GV.AVX_ = 0;
            GV.SSE_ = 0;
        }
        else {
            FailDecode(pMyDisasm);
        }

    }
    else {
        FailDecode(pMyDisasm);
    }

}
