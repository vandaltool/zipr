
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


#include "appfw.h"

using namespace std;

static __thread bool starting_command=false;
static __thread list<pair<string,int> > *sub_commands=NULL;
static __thread char *tainted_data=NULL;

static int check_taint(int s, int e)
{
	if(getenv("APPFW_VERBOSE"))
		cout<<"Checking taint from "<<s<<" to "<<e<<""<<endl;

	for(int i=s;i<e;i++)
	{
		if(tainted_data[i]==APPFW_BLESSED)
			tainted_data[i]=APPFW_BLESSED_KEYWORD;
		else if(tainted_data[i]==APPFW_TAINTED)
			tainted_data[i]=APPFW_SECURITY_VIOLATION;
		else 
			assert(tainted_data[i]==APPFW_BLESSED_KEYWORD || tainted_data[i]==APPFW_SECURITY_VIOLATION);
	}
}

static void discard_comment(istream &fin, int start)
{
	char c=0;
	int position=((int)fin.tellg())-1+start;
	string s="#";
	while(c!='\n')
	{
		c=fin.get();
		s+=c;
	}
	check_taint(position,position+1);
	if(getenv("APPFW_VERBOSE"))
		cout<<"Found comment at "<<position<<": "<<s;
}

static void start_command()
{
	starting_command=true;
	if(getenv("APPFW_VERBOSE"))
		cout<<"Starting new command"<<endl;
}


static void get_string_literal(istream &fin, char c, int start)
{
	string s;
	int position=((int)fin.tellg())-1+start;
	s+=c;
	do
	{
		char d=fin.get();
		s+=d;
		if(d==c)
			break;
		if(d=='`')
			get_string_literal(fin,d,((int)fin.tellg())+start);
	}
	while (!fin.eof());
	if(getenv("APPFW_VERBOSE"))
		cout<<"Found string literal "<<s<<" at "<<position<<endl;

	if(c=='`')
	{
		if(getenv("APPFW_VERBOSE"))
			cout<<"Pushing literal to parse later\n";
		(*sub_commands).push_back(pair<string,int>(s.substr(1,s.length()-2),position));
	}

}

static void get_variable(istream &fin, int start)
{
	string s;
	int position=((int)fin.tellg())-1+start;
	char d=fin.get();

	// check for special characters
	if(d=='$' || d=='_' || d=='@' || d=='?')
	{
		check_taint(position,position+2);
		if(getenv("APPFW_VERBOSE"))
		{
			cout<<"Found special variable at "<<position<<": $"<<d;
			fprintf(stdout, " or in hex: %x\n", d);
		}
		return;
	}
	// check for paren'd or braced variable names 
	if(d=='(' || d=='{')
	{
		s="$";
		s+=d;

		char end_paren=')';
		if(d=='{')
			end_paren='}';
		do
		{
			d=fin.get();
			s+=d;
			if(d==end_paren)
				break;
		} while(!fin.eof());
		if(getenv("APPFW_VERBOSE"))
			cout<<"Found paren'd variable at "<<position<<": '"<<s<<"'"<<endl;
		check_taint(position,position+s.length());
		if(end_paren==')')
		{
			if(getenv("APPFW_VERBOSE"))
				cout<<"Pushing string "<<s<<" as subcommand"<<endl;
			(*sub_commands).push_back(pair<string,int>(s.substr(2,s.length()-3),position+2));
		}
		return;
	}

	// normal variable
	fin.unget();
	s="$";
	do
	{
		d=fin.get();
		if(!isalnum(d) && d!='_')
		{
			fin.unget();
			break;
		}
		s+=d;
	} while (!fin.eof());
	if(getenv("APPFW_VERBOSE"))
		cout<<"Found normal variable at "<<position<<": '"<<s<<"'"<<endl;
	check_taint(position,position+s.length());

}

static inline bool can_start_word(char c)
{
	return isalnum(c) || c=='/' || c=='-' || c=='.' || c=='_';

}

static inline bool can_continue_word(char c)
{
	return can_start_word(c);
}


static inline bool is_keyword(string s)
{
	if(!starting_command)
		return false;
	return
		s=="if" || 
		s=="then" || 
		s=="else" || 
		s=="elif" || 
		s=="fi" || 
		s=="for" || 
		s=="do" || 
		s=="done" || 
		s=="esac" || 
		s=="while";
}

static inline bool is_start_command_word(string s)
{
	if(!is_keyword(s))
		return false;
	return
		s=="then" || 
		s=="do" || 
		s=="else" ; 
}

static inline bool is_command_word(string s)
{
	return starting_command;
}

static void get_word(istream &fin, char c, int start)
{
	string s; 
	char d;
	int position=((int)fin.tellg())-1+start;
	s+=c;
	do
	{
		d=fin.get();
		if(!can_continue_word(d))
		{
			fin.unget();
			break;
		}
		s+=d;
	}
	while (!fin.eof());

	if(d=='=')
	{
		check_taint(position,s.length()+position);
		if(getenv("APPFW_VERBOSE"))
			cout<<"Found assignment word at "<<position<<": "<<s<<endl;
	}
	else if (is_keyword(s))
	{
		check_taint(position,s.length()+position);
		if(getenv("APPFW_VERBOSE"))
			cout<<"Found key word at "<<position<<": "<<s<<endl;
	}
	else if (is_command_word(s))
	{
		check_taint(position,s.length()+position);
		if(getenv("APPFW_VERBOSE"))
			cout<<"Found command word at "<<position<<": "<<s<<endl;
	}
	else
	{
		if(s[0]=='-')
			check_taint(position,position+s.length());
		if(getenv("APPFW_VERBOSE"))
			cout<<"Found option word at "<<position<<": "<<s<<endl;
	}

	if(is_start_command_word(s))
		start_command();

}

static void parse(istream &fin, int start)
{

	start_command();
	while(!fin.eof())
	{
		char c=fin.get();

		switch(c)
		{
			case '#': 
			{
				// taint checked in discard_comment
				discard_comment(fin,start);
				continue;
			}
			case ';':
			{
				int position=((int)fin.tellg())-1+start;
				check_taint(position,position+1);
				start_command();
				continue;
			}
			case '\n':
			{
				start_command();
				continue;
			}
			
			case '|':
			case '&':
			{
				char oldc=c;
				c=fin.get();
				if(c!='|' || c!='&')
				{
					int position=((int)fin.tellg())-1+start;
					check_taint(position,position+1);
					if(getenv("APPFW_VERBOSE"))
						cout<<"Found special token separator at "<<position<<": "<<oldc<<endl;
					start_command();
				}
				fin.unget();
				continue;
			}
			case '$':
				// taint checked in called func
				get_variable(fin,start);
				break;
			case '\'':
			case '`':
			case '"': 
				// taint checked in called func
				get_string_literal(fin,c,start);
				break;

			case EOF:
				return;

			default:
			{
				if (can_start_word(c))	
				{
					// taint checked in called func
					get_word(fin,c,start);
				}
				else if (isspace(c))
				{
					// no taint check needed for spaces 
					// cout<<"Whitespace"<<endl;
					continue; /* spaces change nothing */
				}
				else
				{
					int position=((int)fin.tellg())-1+start;
					check_taint(position,position+1);
					if(getenv("APPFW_VERBOSE"))
					{
						cout <<"Found special character: "<<c;
						fprintf(stdout, " or in hex: %x\n", c);
					}
				}
				
			}
			break;
					
		}
		starting_command=false;
	}
}


extern "C" 
void osc_parse(char* to_parse, char* taint_markings)
{
	list<pair<string,int> > my_sub_commands;
	sub_commands=&my_sub_commands;
	/* set global variables */
	tainted_data=taint_markings;
	(*sub_commands).clear();
	
	stringstream sin;
	sin<<to_parse;

	if(getenv("APPFW_VERBOSE"))
		cout<<"Parsing "<<to_parse<<" length="<<strlen(to_parse)<<endl;
	parse(sin,0);

	while(!(*sub_commands).empty())
	{
		string s=(*sub_commands).front().first;
		int  pos=(*sub_commands).front().second;
		(*sub_commands).pop_front();
		stringstream ss(stringstream::in|stringstream::out);
		ss<<s<<endl;
		if(getenv("APPFW_VERBOSE"))
			cout<<"Parsing sub-command " << s <<endl;
		parse(ss,pos);
		if(getenv("APPFW_VERBOSE"))
			cout<<"Done with " << s <<endl<<endl;
	}


}
