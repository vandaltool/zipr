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
 * MERCHANTIBTILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#ifndef _MEDS_IBTANNOTATION_H_
#define _MEDS_IBTANNOTATION_H_

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
class MEDS_IBTAnnotation : public MEDS_AnnotationBase
{
	public:
		typedef enum { SWITCH, RET, DATA, UNREACHABLE, ADDRESSED, INDIRCALL, UNKNOWN } ibt_reason_code_t;

		MEDS_IBTAnnotation()=delete;
		MEDS_IBTAnnotation( const string& p_rawLine) 
			: xref_addr(0), reason(UNKNOWN)
		{ 
        		setInvalid();
        		parse(p_rawLine);

		};

		ApplicationAddress GetXrefAddr() { return xref_addr; }
		ibt_reason_code_t GetReason() { return reason; }

		virtual const std::string toString() const
		{ 
			return "IBT";
		}

	protected:

		void parse(const string& p_rawLine)
		{
			string tofind="INSTR XREF IBT";
			size_t pos=p_rawLine.find(tofind);
			if(pos==string::npos)
				return;

        		setValid();

                        VirtualOffset vo(p_rawLine);
                        m_virtualOffset = vo;


			stringstream stream(p_rawLine.substr(pos+tofind.length()));

			string from_type;
			stream >> from_type;

			if(string("FROMIB") == from_type)
			{
				stream >> hex >> xref_addr;
				//cout<<"fromib: '"<<p_rawLine<<"'"<<endl;
			}
			else if(string("FROMDATA") == from_type)
			{
				stream >> hex >> xref_addr;
				reason=DATA;
				//cout<<"fromdata: '"<<p_rawLine<<"'"<<endl;
				return;
			}
			else if(string("FROMUNKNOWN") == from_type)
			{
				// no other fields for from UNKNOWN
				xref_addr=0;
				//cout<<"fromunknown: '"<<p_rawLine<<"'"<<endl;
			}

			string reason_code;
			stream >> reason_code;

			if(string("RETURNTARGET") == reason_code)
			{ reason=RET; }
			else if(string("TAILCALLRETURNTARGET") == reason_code)
			{ reason=RET; }
			else if(string("SWITCHTABLE") == reason_code)
			{ reason=SWITCH; }
			else if(string("UNREACHABLEBLOCK") == reason_code)
			{ reason=UNREACHABLE; }
			else if(string("CODEADDRESSTAKEN") == reason_code)
			{ reason=ADDRESSED; }
			else if(string("INDIRCALL") == reason_code)
			{ reason=INDIRCALL; }
			else
			{ reason=UNKNOWN; }

			if(reason==UNKNOWN) 
				cout<<"unknown reason code: '"<<reason_code<<"'"<<endl;
				
		}

	private:

		ApplicationAddress xref_addr;
		ibt_reason_code_t reason;
};

}

#endif
