/*
 * Copyright (c) 2014, 2015 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#ifndef csowarn_h
#define csowarn_h

typedef enum
{
	CSOWE_First,
	CSOWE_BufferOverrun,
	CSOWE_BufferUnderrun,
	CSOWE_DivisionByZero,
	CSOWE_DoubleFree,
	CSOWE_FormatString,
	CSOWE_IntegerOverflowofAllocationSize,
	CSOWE_NegativeShiftAmount,
	CSOWE_NullPointerDereference,
	CSOWE_ShiftAmountExceedsBitWidth,
	CSOWE_TaintedDereference,
	CSOWE_UnreasonableSizeArgument,
	CSOWE_UseAfterFree,
	CSOWE_Default,
	CSOWE_Last
} CSO_WarningEnum_t;


class CSO_WarningType_t
{
	public:

		CSO_WarningType_t(std::string _type_string) { SetType(_type_string);   }

		void SetType(std::string _type_string)
		{
			if(_type_string=="Buffer Overrun")
				ty=CSOWE_BufferOverrun;
			else if(_type_string=="Buffer Underrun")
				ty=CSOWE_BufferUnderrun;
			else if(_type_string=="Division By Zero")
				ty=CSOWE_DivisionByZero;
			else if(_type_string=="Double Free")
				ty=CSOWE_DoubleFree;
			else if(_type_string=="Format String")
				ty=CSOWE_FormatString;
			else if(_type_string=="Integer Overflow of Allocation Size")
				ty=CSOWE_IntegerOverflowofAllocationSize;
			else if(_type_string=="Negative Shift Amount")
				ty=CSOWE_NegativeShiftAmount;
			else if(_type_string=="Null Pointer Dereference")
				ty=CSOWE_NullPointerDereference;
			else if(_type_string=="Shift Amount Exceeds Bit Width")
				ty=CSOWE_ShiftAmountExceedsBitWidth;
			else if(_type_string=="Tainted Dereference")
				ty=CSOWE_TaintedDereference;
			else if(_type_string=="Unreasonable Size Argument")
				ty=CSOWE_UnreasonableSizeArgument;
			else if(_type_string=="Use After Free")
				ty=CSOWE_UseAfterFree;
			else if(_type_string=="Default")
				ty=CSOWE_Default;
			else
				assert(0);
		};

		CSO_WarningEnum_t GetType() const { return ty; }

		bool operator==(const CSO_WarningEnum_t& we) const { return we==ty; }
		bool operator==(const CSO_WarningType_t& wt) const { return wt.GetType()==ty; }

	private:
		CSO_WarningEnum_t  ty;
};

class CSO_WarningRecord_t
{
	public:
		CSO_WarningRecord_t() : buf_size(0), insn_addr(0), t("Default") {}

		void SetType(const CSO_WarningType_t& _t) { t=_t;}
		const CSO_WarningType_t& GetType() const { return  t;} 

		void SetBufferSize(size_t _size) { buf_size=_size;}
		size_t GetBufferSize() const { return buf_size; } 

		void SetInstructionAddress(uintptr_t _addr) { insn_addr=_addr;}
		uintptr_t GetInstructionAddress() const { return insn_addr; } 

	private:

		size_t buf_size;
		uintptr_t insn_addr;
		CSO_WarningType_t t;

};

#endif

