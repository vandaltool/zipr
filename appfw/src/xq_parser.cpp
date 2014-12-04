/*
 * Copyright (c) 2013, 2014 - University of Virginia 
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

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <cctype>
#include <list>
#include <string>
#include <sstream>
#include <string.h>
#include <assert.h>
#include <ctype.h>


using namespace std;

extern "C" {
#include "appfw.h"
}

static __thread char *tainted_data=NULL;

void check_taint(int position)
{
	if(getenv("APPFW_VERY_VERBOSE"))
		cout<<"Checking taint at "<<std::dec<<position<<endl;	

	int i=position-1;	 /* 0 indexed */
	if(tainted_data[i]==APPFW_BLESSED)
		tainted_data[i]=APPFW_BLESSED_KEYWORD;
	else if(tainted_data[i]==APPFW_TAINTED)
		tainted_data[i]=APPFW_SECURITY_VIOLATION;
	else
		assert(tainted_data[i]==APPFW_BLESSED_KEYWORD || tainted_data[i]==APPFW_SECURITY_VIOLATION);

}

void check_quote(istream &fin, int &start,  char quote_char)
{
	check_taint(start);

	while(1)
	{
		char c=fin.get();
		start++;
		if(getenv("APPFW_VERY_VERBOSE"))
			cout<<"Char is "<<c<<endl;
		if(c==quote_char)		
		{
			if(fin.peek()==quote_char)
			{
				c=fin.get();
				start++;
				if(getenv("APPFW_VERY_VERBOSE"))
					cout<<"Char is "<<c<<endl;
				continue;
			}
			if(getenv("APPFW_VERY_VERBOSE"))
				cout << "Found end quote at " << start<<endl;
			check_taint(start);
			return;
		}
		if(c==EOF)
		{
			cout<<"Error in quote matching, eof found"<<endl;
			exit(1);
		}

	}

}

void check_for_comment(istream &fin, int &start)
{
	check_taint(start);

	/* get the 2nd character of the quote */
	char c=fin.get();
	start++;
	if(getenv("APPFW_VERY_VERBOSE"))
		cout<<"Char is "<<c<<endl;
	assert(c==':');
	check_taint(start);

	int depth=1;
	while(depth>1)
	{
		c=fin.get();
		start++;
		if(getenv("APPFW_VERY_VERBOSE"))
			cout<<"Char is "<<c<<endl;
		check_taint(start);
		switch(c)
		{
			case '(':
			{
				if(fin.peek()==':')	
				{
					fin.get();
					start++;
					if(getenv("APPFW_VERY_VERBOSE"))
						cout<<"Char is "<<c<<endl;
					depth++;
					check_taint(start);
				}
				break;
			}
			case ':':	
			{
				if(fin.peek()==')')	
				{
					fin.get();
					start++;
					if(getenv("APPFW_VERY_VERBOSE"))
						cout<<"Char is "<<c<<endl;
					depth--;
					check_taint(start);
				}
				break;
			}
			case EOF:
			{
				cout<<"Error in quote matching, eof found"<<endl;
				exit(1);
			}
		}	
		
	}
}

void parse(istream &fin)
{
	int start=0;

        while(!fin.eof())
        {
                char c=fin.get();
		start++;
		if(getenv("APPFW_VERY_VERBOSE"))
			cout<<"Char is "<<c<<endl;

		switch(c)
		{
			case '"':
			case '\'':
				check_quote(fin,start,c);
				break;
			case '(':
				check_taint(start);
				if(fin.peek()==':')
					check_for_comment(fin,start);
				break;
			case EOF:
				return;
			default:
				/* spacing makes no diff. to xquery commands
				 * so we ignore them.  
				 * generally we will allow numeric constants
				 * (at least integers) to be tainted, too
				 * this test proxies for tainted digits and
				 * spacing 
				 */
				if(isdigit(c) || isspace(c))
				{	/* empty */ 		
				}
				else 
				{
					check_taint(start);
				}
		}

	}
}

extern "C"
void xq_parse(char* to_parse, char* taint_markings)
{
        tainted_data=taint_markings;
        stringstream sin;
        sin<<to_parse;

        if(getenv("APPFW_VERBOSE"))
                cout<<"Parsing "<<to_parse<<" length="<<strlen(to_parse)<<endl;

        parse(sin);

}


