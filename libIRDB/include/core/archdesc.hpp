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


enum ADFileType_t { AD_ELF, AD_CGC, AD_PE, AD_NONE };
enum ADMachineType_t { admtAarch64,  admtX86_64, admtI386, admtNone }; 

class ArchitectureDescription_t
{
	public:

		ArchitectureDescription_t() : bits(0), ft(AD_NONE), mt(admtNone) {}

		int GetBitWidth() const 		{ return bits; }	
		void SetBitWidth(const int _bits) 	{ bits=_bits; }	

		ADFileType_t GetFileType() const	{ return ft; }	
		void SetFileType(const ADFileType_t t) 	{ ft=t; }

		ADMachineType_t getMachineType() const 		{ return mt; }	
		void setMachineType(const ADMachineType_t t) 	{ mt=t; }

	private:

		size_t bits;
		ADFileType_t ft;
		ADMachineType_t mt;
};

