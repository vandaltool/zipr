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

#ifndef _MEDS_TAKENADDRESSANNOTATION_H_
#define _MEDS_TAKENADDRESSANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_AnnotationBase.hpp"
#include <iostream>

namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS shadow annotation
//
class MEDS_TakesAddressAnnotation : public MEDS_AnnotationBase
{
	public:
		typedef enum { SWITCH, RET, UNKNOWN } ib_type_t;

		MEDS_TakesAddressAnnotation( const string& p_rawLine) : 
			referenced_address(0),
			m_isCode(false)
		{ 
        		setInvalid();
        		parse(p_rawLine);

		};


		virtual const std::string toString() const
		{ 
			std::string ret="TakesAddress="+to_string(referenced_address);
			return ret;
		}

		ApplicationAddress GetReferencedAddress() const { return referenced_address; }
		bool isCode() const { return m_isCode; }
		bool isData() const { return !m_isCode; }

	protected:

		void parse(const string& p_rawLine)
		{
			const auto tofind=string("INSTR XREF TAKES_ADDRESS_OF ");
			const auto codestring=string("CODE");
			const auto pos=p_rawLine.find(tofind);
			if(pos==string::npos)
				return;

			// she be valid
			setValid();

			m_isCode=(p_rawLine.find(codestring)!=string::npos);
        		m_virtualOffset = VirtualOffset(p_rawLine);

			// skips over INSTR*{code|data}
			const auto rest=p_rawLine.substr(pos+tofind.length()+codestring.length());
			istringstream is(rest);
			is >> hex >>referenced_address;	/* get the address */

			// cout<<"Found takes_address of "<<m_virtualOffset.to_string() << " to " 
			//    << hex << referenced_address<< " from '" << rest << "'"<<endl;

				
		}


	private:

		ApplicationAddress referenced_address;
		bool m_isCode;
};

}

#endif
