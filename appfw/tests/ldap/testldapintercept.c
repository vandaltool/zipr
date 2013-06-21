// Test LDAP interception layer

#include <stdio.h>
#include <ldap.h>

int main(int argc, char **argv)
{
	LDAP *ld = NULL;
	char *filter_data = getenv("LDAP_DATA");
	char base[1024];
	int scope = LDAP_SCOPE_BASE;
	char filter[1024];
	int attrsonly = 0;

	sprintf(base,"virginia.edu"); // this doesn't really matter
	sprintf(filter,"(cn=%s)", filter_data);

	if (ldap_search(ld,base,scope,filter,NULL,attrsonly) == -1)
		fprintf(stderr,"(1) ldap_search returned an error\n");

	if (ldap_search_ext(ld,base,scope,filter,NULL,attrsonly,NULL,NULL,NULL,25,NULL) == -1)
		fprintf(stderr,"(2) ldap_search returned an error\n");

	if (ldap_search_ext_s(ld,base,scope,filter,NULL,attrsonly,NULL,NULL,NULL,25,NULL) == -1)
		fprintf(stderr,"(3) ldap_search returned an error\n");

	if (ldap_search_s(ld,base,scope,filter,NULL,attrsonly,NULL) == -1)
		fprintf(stderr,"(1) ldap_search returned an error\n");

}
