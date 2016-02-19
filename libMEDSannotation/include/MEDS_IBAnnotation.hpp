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

#ifndef _MEDS_IBANNOTATION_H_
#define _MEDS_IBANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_AnnotationBase.hpp"

namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS shadow annotation
//
class MEDS_IBAnnotation : public MEDS_AnnotationBase
{
	public:
		typedef enum { SWITCH, RET, UNKNOWN } ib_type_t;

		MEDS_IBAnnotation( const string& p_rawLine) : the_type(UNKNOWN), count(0), complete(false)
		{ 
        		setInvalid();
        		parse(p_rawLine);

		};

		ib_type_t GetType() const  { return the_type; }
		int GetCount() const { return count; }
		bool IsComplete() const { return complete; }

		virtual const std::string toString() const
		{ 
			std::string ret="IB";
			if(IsComplete()) ret+=" COMPLETE";
			ret+=" Count=<na> type=<na>";
			return ret;
		}

	protected:

		void parse(const string& p_rawLine)
		{
			string tofind="INSTR XREF FROMIB";
			size_t pos=p_rawLine.find(tofind);
			if(pos==string::npos)
				return;

        		VirtualOffset vo(p_rawLine);
        		m_virtualOffset = vo;


			// she be valid
			setValid();

			tofind="COMPLETE";
			size_t pos2=p_rawLine.find(tofind);
			if(pos2!=string::npos)
			{
				pos=pos2;
				complete=true;
			}

			string rest=p_rawLine.substr(pos+tofind.length());
			istringstream is(rest);
			is>>count;	/* get count */

			if(p_rawLine.find("RETURNTARGET")) set_type(RET);
			if(p_rawLine.find("SWITCHTABLE")) set_type(SWITCH); 
				
		}

		void set_type(ib_type_t t)
		{
			assert(the_type==UNKNOWN);
		}

	private:

		ib_type_t the_type;
		int count;
		bool complete;
};

}

#endif
