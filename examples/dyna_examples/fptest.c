#ident "$Id: fptest.c,v 1.1 2008-01-11 17:05:47 jdh8d Exp $"

#include <stdio.h>

int main (int argc, char *argv[]) {
	int i, *p;
	double d=3.1415;


	fprintf(stdout, "When Strata controls me I say: %g\n",d);


	fflush(stdout);
	fprintf(stdout, "When I control me I still say: %g\n",d);
}
