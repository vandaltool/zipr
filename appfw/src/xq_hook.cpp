#include <dlfcn.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <xqilla/xqilla-simple.hpp>
#include <iostream>

using namespace std;

extern "C" void xqfw_init();
extern "C" int xqfw_verify(const char *p_command);



extern "C" 
xmlXPathObjectPtr	xmlXPathEvalExpression	(const xmlChar * str, xmlXPathContextPtr ctxt)
{

	static xmlXPathObjectPtr (*my_xmlXPathEvalExpression)(const xmlChar * str, xmlXPathContextPtr ctxt)=NULL;
	xqfw_init();


	if(getenv("APPFW_VERBOSE"))
		cout <<"In xmlPathObjectPtr with string="<<str<<endl;


  	if (!my_xmlXPathEvalExpression)
	{
		void *p = dlsym(RTLD_NEXT, "xmlXPathEvalExpression");
    		my_xmlXPathEvalExpression = (xmlXPathObjectPtr (*)(const xmlChar * str, xmlXPathContextPtr ctxt))p;
	}

	if(xqfw_verify((const char*)str))
	{
		if(getenv("APPFW_VERBOSE"))
			cout<<"xPath detected OK!"<<endl;
		return (*my_xmlXPathEvalExpression)(str,ctxt);
	}
	else
	{
		if(getenv("APPFW_VERBOSE"))
			cout<<"xPath injection detected!"<<endl;
		return (*my_xmlXPathEvalExpression)((const xmlChar*)"intentionally bad xpath query due to command injection" ,ctxt);
		
	}

}

XQQuery* XQilla::parse(unsigned short const* one, DynamicContext* two, unsigned short const* three, unsigned int four, 
	xercesc_3_1::MemoryManager* five /* , XQQuery* six */)
{
	XQQuery *six=NULL;

	xqfw_init();
	XQQuery* (*my_parse)(unsigned short const*, DynamicContext*, unsigned short const*, unsigned int, xercesc_3_1::MemoryManager*, XQQuery*)=NULL;

  	if (!my_parse)
	{
		void *p = dlsym(RTLD_NEXT, "_ZN6XQilla5parseEPKtP14DynamicContextS1_jPN11xercesc_3_113MemoryManagerEP7XQQuery");
    		my_parse = (XQQuery* (*)(unsigned short const*, DynamicContext*, unsigned short const*, unsigned int, xercesc_3_1::MemoryManager*, XQQuery*))p;
	}

	string s;
	for(int i=0;one[i]!=0;i++)
		s+=(char)one[i];

	if(xqfw_verify(s.c_str()))
	{
		if(getenv("APPFW_VERBOSE"))
			cout<<"xPath detected OK!"<<endl;
		return (*my_parse)(one,two,three,four,five,six);
	}
	else
	{
		if(getenv("APPFW_VERBOSE"))
			cout<<"XQuery injection detected!"<<endl;
		unsigned short newone[4];
		newone[0]='b';
		newone[1]='a';
		newone[2]='d';
		newone[3]='\0';
		return (*my_parse)(newone,two,three,four,five,six);
		
	}

}

