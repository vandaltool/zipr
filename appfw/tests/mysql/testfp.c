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

/* 
 * Test for false positives
 *
*/

#include <mysql.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;

   char *server = "localhost";
   char *user = "root";
   char *password = "root";
   char *database = "testdata";
	
   conn = mysql_init(NULL);
   
   /* Connect to database */
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
   }

   /* test for false positives */
   test1(conn);
   test2(conn);
   test3(conn);
   test4(conn);
   test5(conn);

   /* cleanup */
	mysql_close(conn);
	return 0;
}

/* Test multi-statement selects */
void test1(MYSQL *conn)
{
	char query[1024];

	printf("Testing multi-statements selects\n");

	sprintf(query, "Select * FROM users_1796 LIMIT 10");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10;");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10 ;");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10 ; ");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 whEre id='abc' LIMIT 10 ; ");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10 ; SELECT * FROM users_1796 LIMIT 2 ;");
	mysql_query(conn, query);
}

void test2(MYSQL *conn)
{
	char query[1024];

	printf("Testing AS, GROUP BY, MIN, MAX, INSERT, CREATE\n");

	sprintf(query,"SELECT foo, MIN(bar) AS bar FROM tbl GROUP BY foo");
	mysql_query(conn, query);

	sprintf(query,"SELECT foo, MIN(bar), count(*) AS bar FROM tbl GROUP BY foo;");
	mysql_query(conn, query);

	sprintf(query,"SELECT foo, MAX(bar) AS Count FROM tbl GROUP BY foo ORDER BY Count DESC;");
	mysql_query(conn, query);

	sprintf(query,"SELECT a.foo, a.bar FROM tbl a JOIN ( SELECT foo, MAX(bar) AS Count FROM tbl GROUP BY foo ) b ON a.foo=b.foo AND a.bar=b.count ORDER BY a.foo, a.bar; "); 

	sprintf(query," CREATE TABLE supparts(supID char(2),partID char(2)); INSERT INTO supparts VALUES ('s1','p1'),('s1','p2'),('s1','p3'),('s1','p4'),('s1','p5'),('s1','p6'), ('s2','p1'),('s2','p2'),('s3','p2'),('s4','p2'),('s4','p4'),('s4','p5'); ");
	mysql_query(conn, query);
}

void test3(MYSQL *conn)
{
	char query[1024];

	printf("Testing complex queries\n");

	sprintf(query, "select s.name, sum(l.qty) as n from salespersons s join orders o using(salespersonid) join orderlines l using(orderid) join products     p using(productid) where p.name='computer desk' group by s.name order by n desc limit 1;"); 
	mysql_query(conn, query);
}

void test4(MYSQL *conn)
{
	char query[1024];

	printf("Testing multi-line statements\n");

	sprintf(query, "select s.name, sum(l.qty) as n \
	from salespersons s join orders o using(salespersonid) join \
	orderlines l using(orderid) join products     p using(productid) where p.name='computer desk' group by s.name order by n desc limit 1;"); 
	mysql_query(conn, query);
	return;

	// bug -- we're not handling \n or \ right
	sprintf(query, "\n \
select s.name, sum(l.qty) as n      -- sum quantities \n \
from salespersons s \n \
join orders       o using(salespersonid) \n \
join orderlines   l using(orderid) \n \
join products     p using(productid) \n \
where p.name='computer desk'     \n \
group by s.name                     -- aggregate by salesperson \n \
order by n desc limit 1;            -- order by descending sum, pick off top value ");
	mysql_query(conn, query);

	sprintf(query, "\
	SELECT s.partID, s, thiscol, s.thatcol, anothercol, x.Suppliers \
	FROM supparts s \
	JOIN ( \
	  SELECT partID,GROUP_CONCAT(supID ORDER BY supID) AS Suppliers  \
	    FROM supparts  \
		  GROUP BY partID \
		  ) x USING(partID)"); 
	mysql_query(conn, query);
}


void test5(MYSQL *conn)
{
	char query[1024];

	printf("Testing INSERT\n");

	sprintf(query,"INSERT INTO bookings ( court_id , member1 , member2 , time , fee ) VALUES  (1, 1000, 1001, '2009-09-09 15:49:38', 3.00),  (2, 1000, 1000, '2009-09-08 15:50:04', 3.00);"); 
	mysql_query(conn, query);

	sprintf(query,"SELECT member, ROUND(SUM(fee/2),2) AS total FROM (SELECT member1 AS member, fee FROM bookings UNION ALL SELECT member2, fee FROM bookings ) AS tmp GROUP BY member; ");
	mysql_query(conn, query);
}

