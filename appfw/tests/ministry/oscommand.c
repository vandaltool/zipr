#include <stdio.h>

void display_html_header()
{
	printf("Content-type: text/html\n\n");
	printf("\n<html><head></head><body>\n");
	printf("<h1>File Content Viewer</h1>\n");
	fflush(stdout);
}

void display_html_footer()
{
	printf("</body></html>\n");
}

void get_filename(char *filename)
{
  char name[1024];
  char value[1024];
  char *query = getenv("QUERY_STRING");
  char *phrase;

  filename[0]='\0';

  phrase = strtok(query,"&");
  while( phrase ) {
      parse(phrase, name, value);
	  if (strcasecmp(name, "filename") == 0)
	  {
        strcpy(filename, value);
	  }
      phrase = strtok((char *)0,"&");
  }
}

int main(int argc, char **argv)
{
	char filename[1024];
	char cmd[2048];

	display_html_header();


	get_filename(&filename);


	printf("<pre>");

	printf("<hr>");
	if (strlen(filename) > 0)
	{
		sprintf(cmd, "/bin/cat %s", filename);
		int code = system(cmd);
		if (code != 0)
		{
			printf("ERROR IN ISSUING CMD: %s\n", cmd);
		}
	}
	printf("</pre>");

	display_html_footer();
}
