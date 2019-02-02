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

namespace libIRDB
{


using ADFileType_t    = IRDB_SDK::ADFileType_t;
using ADMachineType_t = IRDB_SDK::ADMachineType_t;

class ArchitectureDescription_t : virtual public IRDB_SDK::ArchitectureDescription_t
{
	public:

		virtual ~ArchitectureDescription_t() {}
		ArchitectureDescription_t() : bits(0), ft(IRDB_SDK::adftNone), mt(IRDB_SDK::admtNone) {}

		int getBitWidth() const 		{ return bits; }	
		void setBitWidth(const int _bits) 	{ bits=_bits; }	

		ADFileType_t getFileType() const	{ return ft; }	
		void setFileType(const ADFileType_t t) 	{ ft=t; }

		ADMachineType_t getMachineType() const 		{ return mt; }	
		void setMachineType(const ADMachineType_t t) 	{ mt=t; }

	private:

		size_t bits;
		ADFileType_t ft;
		ADMachineType_t mt;
};

}
