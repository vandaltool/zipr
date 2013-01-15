/*
@GOOD_ARGS cbc 1mgr8  
@BAD_ARGS cbc "sdfs' or 1=1 or password/text() ='"
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS home/john
Example 1

Consider the following simple XML document that stores authentication information and a 
snippet of Java code that uses XPath query to retrieve authentication information:

Example Language: XML 
<users>
<user>
<login>john</login>
<password>abracadabra</password>
<home_dir>/home/john</home_dir>
</user>
<user>
<login>cbc</login>
<password>1mgr8</password>
<home_dir>/home/cbc</home_dir>
</user>
</users>
The Java code used to retrieve the home directory based on the provided credentials is:

(Bad Code)Example Language: Java 

XPath xpath = XPathFactory.newInstance().newXPath();
XPathExpression xlogin = xpath.compile("//users/user[login/text()='" + login.getUserName() + "' and password/text() = '" + login.getPassword() + "']/home_dir/text()");
Document d = DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(new File("db.xml"));
String homedir = xlogin.evaluate(d);

Assume that user "john" wishes to leverage XPath Injection and login without a valid password. 
By providing a username "john" and password "' or ''='" the XPath expression now becomes

//users/user[login/text()='john' or ''='' and password/text() = '' or ''='']/home_dir/text()
which, of course, lets user "john" login without a valid password, thus bypassing authentication.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

xmlDocPtr
getdoc (char *docname) {
	xmlDocPtr doc;
	doc = xmlParseFile(docname);
	
	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return NULL;
	}

	return doc;
}

xmlXPathObjectPtr
getnodeset (xmlDocPtr doc, xmlChar *xpath){
	
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		printf("Error in xmlXPathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		printf("Error in xmlXPathEvalExpression\n");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
                printf("No result\n");
		return NULL;
        }	
	return result;
}
int
main(int argc, char **argv) {

	char *docname = "./passwrd_xml.xml";
	xmlDocPtr doc;
	xmlChar *xpath ; 
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr result;
	int i;
	xmlChar *keyword;
        char xlogin[356];
		
	if (argc <= 2) {
		printf("Usage:  <user name> <password> \n");
		return(0);
	}

	doc = getdoc(docname);

//XPathExpression xlogin = xpath.compile("//users/user[login/text()='" + login.getUserName() + "' and password/text() = '" + login.getPassword() + "']/home_dir/text()");
        
        if(strlen(argv[1])+strlen(argv[2])<256){
           strcpy(xlogin, "//users/user[login/text()='");
           strcat(xlogin, argv[1]); 
           strcat(xlogin, "' and password/text() = '"); 
           strcat(xlogin, argv[2]); 
           //strcat(xlogin, "' or 1=1 or password/text() ='"); 
           strncat(xlogin, "']/home_dir/text()",18);
           printf("test me= %s\n",xlogin);
        }
	//xpath = (xmlChar*) "//login";
	xpath = (xmlChar*) xlogin;

	result = getnodeset (doc, xpath);
	if (result) {
		nodeset = result->nodesetval;
		for (i=0; i < nodeset->nodeNr; i++) {
			//keyword = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
			keyword = xmlNodeListGetString(doc, nodeset->nodeTab[i], 1);
		printf("keyword: %s\n", keyword);
		xmlFree(keyword);
		}
		xmlXPathFreeObject (result);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return (0);
}


