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

#include <mysql.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char *argv[]) 
{
   MYSQL *conn = NULL;

   char query[2048];
   char bogus[2048];
   char *query_data = getenv("QUERY_DATA");

   // make sure we have 1 character fragments
   sprintf(&bogus[3],"o");
   sprintf(&bogus[5],"r");

   sprintf(query,"SeLEcT * FROM someTable \n WHERE id='%s'", query_data);

   printf("query: %s\n", query);

   mysql_query(conn, query);
}

