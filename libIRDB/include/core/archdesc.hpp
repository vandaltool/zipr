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


enum AD_FileType_t { AD_ELF, AD_CGC };

class ArchitectureDescription_t
{
	public:

	ArchitectureDescription_t() : bits(0) {}

	int GetBitWidth() 		{ return bits; }	
	void SetBitWidth(int _bits) 	{ bits=_bits; }	

	AD_FileType_t GetFileType() 		{ return ft; }	
	void SetFileType(AD_FileType_t t) 	{ ft=t; }

	private:

		int bits;
		AD_FileType_t ft;
};

