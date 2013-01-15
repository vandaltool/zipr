/*
CWE-652: Improper Neutralization of Data within XQuery Expressions ('XQuery Injection')

Description Summary

The software uses external input to dynamically construct an XQuery expression used to retrieve data from an XML database, but it does not neutralize or incorrectly neutralizes that input. This allows an attacker to control the structure of the query. 
Extended Description

The net effect is that the attacker will have control over the information selected from the XML database and may use that ability to control application flow, modify logic, retrieve unauthorized data, or bypass important checks (e.g. authentication). 

Common Consequences
Confidentiality 
Technical Impact: Read application data
An attacker might be able to read sensitive information from the XML database.
 

@GOOD_ARGS cbc 1mgr8  
@BAD_ARGS cbc "sdfs' or 1=1 or password/text() ='"
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS home/john


   */
#include <iostream>
#include <xqilla/xqilla-simple.hpp>


int main(int argc, char *argv[]) {

        char xlogin[356];

        if (argc <= 2) {
		std::cout << "Usage:  <user name> <password>" << std::endl;
                return(0);
        }

//XPathExpression xlogin = xpath.compile("//users/user[login/text()='" + login.getUserName() + "' and password/text() = '" + login.getPassword() + "']/home_dir/text()");

        if(strlen(argv[1])+strlen(argv[2])<256){
           strcpy(xlogin, "/users/user[login/text()='");
           strcat(xlogin, argv[1]);
           strcat(xlogin, "' and password/text() = '");
           strcat(xlogin, argv[2]);
           //strcat(xlogin, "' or 1=1 or password/text() ='"); 
           strcat(xlogin, "']/home_dir/text()");
           std::cout << "XQuery=" <<  xlogin << std::endl;
        }


  // Initialise Xerces-C and XQilla by creating the factory object
  XQilla xqilla;

  // Parse an XQuery expression
  // (AutoDelete deletes the object at the end of the scope)
  AutoDelete<XQQuery> query(xqilla.parse(X(xlogin)));

  // Create a context object
  AutoDelete<DynamicContext> context(query->createDynamicContext());

  // Parse a document, and set it as the context item
  Sequence seq = context->resolveDocument(X("passwrd_xml.xml"));
  if(!seq.isEmpty() && seq.first()->isNode()) {
    context->setContextItem(seq.first());
    context->setContextPosition(1);
    context->setContextSize(1);
  }

  // Execute the query, using the context
  Result result = query->execute(context);

  // Iterate over the results, printing them
  Item::Ptr item;
  while(item = result->next(context)) {
    std::cout << UTF8(item->asString(context)) << std::endl;
  }

  return 0;
}
