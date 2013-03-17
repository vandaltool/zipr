/* querystring.c
 
Break up a QUERY_STRING variable.

Output is in SETENV format, ready to be eval'd

*/

#include <stdio.h>
#include <string.h>

parse( char *s, char *name, char *value )
/* we cant use strtok since we are already using it at the higher level */
{
	char *vname;
	char *val;
	int c;
	char buf[3];

	if(!s) {
		printf("X_ERR='Null phrase'\n");
		return;
	}
	vname = s;
	while(*s && (*s != '=')) s++;
	if(!*s) {
		printf("X_ERR='Null assignment'\n");
		return;
	}
	*(s++) = '\0';
	
	strcpy(name, vname);

 	value[0] = '\0';
	int pos = 0;
	for(val=s; *val; val++) {	
		switch( *val ) {
			case '%':
				buf[0]=*(++val); buf[1]=*(++val); 
				buf[2]='\0';
				sscanf(buf,"%2x",&c);
				break;
			case '+':
				c = ' ';
				break;
			default:
				c = *val;
		}

		value[pos++] = c;
	}

	value[pos] = '\0';
}

/*
main()
{
	char *query_string;
	char *phrase;

	query_string = (char *)getenv("QUERY_STRING");

	if(!query_string) { 
		printf("X_ERR='QUERY_STRING not set'\n");
		exit(0);
	}

	phrase = strtok(query_string,"&");
	while( phrase ) {
		parse(phrase);
		phrase = strtok((char *)0,"&");
	}
}
*/
