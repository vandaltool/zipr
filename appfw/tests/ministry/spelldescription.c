#include <mysql.h>
#include <stdio.h>
#include <string.h>

void get_spell_password(char *spell, char *password)
{
  char name[1024];
  char value[1024];
  char *query = getenv("QUERY_STRING");
  char *phrase;

  phrase = strtok(query,"&");
  while( phrase ) {
      parse(phrase, name, value);
	  if (strcasecmp(name, "spell") == 0)
	  {
        strcpy(spell, value);
	  }
	  else if (strcasecmp(name, "password") == 0)
	  {
        strcpy(password, value);
	  }

      phrase = strtok((char *)0,"&");
  }
}

void display_html_header()
{
  printf("Content-type: text/html\n\n");
  printf("\n<html>\n<head></head><body><img src=\"/ministry/images/header.jpg\"><br>\n");
}

void display_html_footer()
{
	printf("</body></html>");
}

int main(int argc, char *argv[]) 
{
   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;

   char *server = "localhost";
   char *user = "root";
   char *db_password = "root";
   char *database = "ministry";

   display_html_header();
	
   conn = mysql_init(NULL);
   
   /* Connect to database */
   if (!mysql_real_connect(conn, server,
         user, db_password, database, 0, NULL, 0)) {
      printf("%s\n", mysql_error(conn));
      return 0;
   }

	// Get spell and password from HTML query string
	char spell[1024];
	char password[1024];

	get_spell_password(&spell, &password);

	char query[2048];
	char *fmtString = "SELECT * FROM spells WHERE spell='%s' AND password='%s'";
   
	sprintf(query, fmtString, spell, password);

	if (mysql_query(conn, query)) {
	  printf("<hr><pre>%s</pre><p>\n", mysql_error(conn));
	  return 0;
	}
	
	res = mysql_use_result(conn);
	
	/* output fields 1 and 2 of each row in HTML table*/
	printf("<h1>Ministry of Magic Secret Spell Archive</h1>\n");
	printf("<table><tr><td><strong>Spell</strong></td><td><strong>Description</strong></td></tr>\n");
	while ((row = mysql_fetch_row(res)) != NULL)
	  printf("<tr><td>%s</td><td>%s</td></tr>\n", row[0], row[2]);
	printf("</table>\n");
	
	/* Release memory used to store results and close connection */
	mysql_free_result(res);
	mysql_close(conn);

	display_html_footer();

	return 0;
}
