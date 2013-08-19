#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "appfw.h"

#define MAX_NUM_SIGNATURES 40000
#define MAX_SIGNATURE_SIZE 1024

static const char *sigFileEnv = "APPFW_SIGNATURE_FILE";
static int fw_numPatterns = 0;
static char **fw_sigs = NULL;
static int appfw_initialized = 0;

static void delete_matched_records(int pos);
static void delete_matched_record(matched_record *r);
static void record_matched_signature(matched_record** matched_signatures, int startPos, int endPos, int signatureId);

static void reset_sig_file_env_var()
{
	extern char **environ;
	int i;
	for(i=0;(environ[i]!=0);i++)
	{
		if(getenv("APPFW_ENV_VERBOSE"))
			fprintf(stderr,"environ[i]=%s\n",environ[i]);
		/* check that the environ has the key followed by an equal */
		if(strncmp(sigFileEnv,environ[i],strlen(sigFileEnv))==0 && 
			environ[i][strlen(sigFileEnv)]=='=')
		{
			environ[i][0]='B';
		}
	}
}

// read in signature file
// environment variable specifies signature file location
void appfw_init()
{
	int i;
	int verbose=0;
	if(getenv("APPFW_VERBOSE"))
		verbose=1;
	int numSigs = 0;

	if (appfw_isInitialized()) return;

	char *signatureFile = getenv(sigFileEnv);
	if (!signatureFile)
	{
		if(verbose)
			appfw_error("no signature file found");
	}

	reset_sig_file_env_var();

	FILE *sigF = fopen(signatureFile, "r");
	if (sigF)
	{
		char buf[MAX_SIGNATURE_SIZE];

		fw_sigs = malloc(sizeof(char*) * MAX_NUM_SIGNATURES);

		while (fgets(buf, MAX_SIGNATURE_SIZE, sigF) != NULL)
		{
			if (strlen(buf) > 1) // don't want "\n" by itself
			{
				fw_sigs[numSigs] = (char *) malloc(strlen(buf) + 128);
				strncpy(fw_sigs[numSigs], buf, strlen(buf));
				fw_sigs[numSigs][strlen(buf)-1] = '\0';

				if(verbose && getenv("VERY_VERBOSE"))
					fprintf(stderr,"read sig[%d]: %s (%d)\n", numSigs, fw_sigs[numSigs], (int)strlen(fw_sigs[numSigs]));

				numSigs++;
			}
		}

		fw_numPatterns = numSigs;
		fclose(sigF);
		appfw_initialized = 1;
		if(getenv("APPFW_VERBOSE"))
			fprintf(stderr, "appfw init finished\n");

	}
	else
	{
		if(getenv("APPFW_VERBOSE"))
			appfw_error("could not open signature file");
			appfw_initialized = 0;
	}
	fflush(stderr);
}

int appfw_isInitialized()
{
	return appfw_initialized;
}

// returns # of signature patterns
int appfw_getNumSignatures()
{
		return fw_numPatterns;
}

// returns signature patterns
char **appfw_getSignatures()
{
	return fw_sigs;
}

// generic error message
void appfw_error(const char *msg)
{
	fprintf(stderr,"[appfw]: %s\n", msg);
}

// mark parts of string as tainted
void appfw_taint_range(char *taint, char taintValue, int from, int len)
{
	memset(&taint[from], taintValue, len);
}

void appfw_display_signatures(char **fw_sigs, int numSigs)
{
	int i;
	for (i = 0; i < numSigs; ++i)
	{
		fprintf(stderr,"sig[%d]: [%s]\n", i, fw_sigs[i]);
	}
}

// buffers must be big enough
void appfw_establish_taint(const char *command, char *taint, matched_record** matched_signatures, int case_sensitive)
{
	int j, pos, sigId;
	int patternFound;
	char **fw_sigs = appfw_getSignatures();
	int commandLength = strlen(command);
	taint[commandLength] = '\0';
	int verbose=getenv("APPFW_VERBOSE")!=NULL;
	verbose+=getenv("VERY_VERBOSE")!=NULL;

	if (!fw_sigs)
	{
		appfw_taint_range(taint, APPFW_BLESSED, 0, commandLength);
		return;
	}

	// set taint markings to 'tainted' by default
	appfw_taint_range(taint, APPFW_TAINTED, 0, commandLength);

	// use simple linear scan for now 
	// list of signature patterns are sorted in reverse length order already
	// unset taint when match is found
	int numSignatures = appfw_getNumSignatures();  
	pos = 0;

	while (pos < commandLength)
	{
		for (sigId = 0; sigId < numSignatures; ++sigId)
		{
			int length_signature = strlen(fw_sigs[sigId]);
			if(length_signature==1 && isalpha(*fw_sigs[sigId]))
				continue;
			if (((case_sensitive  && strncmp    (&command[pos], fw_sigs[sigId], length_signature) == 0)) || 
			    ((!case_sensitive && strncasecmp(&command[pos], fw_sigs[sigId], length_signature) == 0)) )
			{
				appfw_taint_range(taint, APPFW_BLESSED, pos, length_signature);

				// need to record which signature fragments matched at position pos
				record_matched_signature(matched_signatures, pos, pos+length_signature-1, sigId); 
			}
		}
		pos++;
	}
}

// enum { APPFW_BLESSED, APPFW_TAINTED, APPFW_SECURITY_VIOLATION, APPFW_BLESSED_KEYWORD };
void appfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint)
{
		int i;
		fprintf(stderr,"%s: %s\n", p_msg, p_query);
		fprintf(stderr,"%s: ", p_msg);
		for (i = 0; i < strlen(p_query); ++i)
		{
				if (p_taint[i] == APPFW_BLESSED)
						fprintf(stderr,"o");
				else if (p_taint[i] == APPFW_SECURITY_VIOLATION)
						fprintf(stderr,"v");
				else if (p_taint[i] == APPFW_BLESSED_KEYWORD)
						fprintf(stderr,"k");
				else // APPFW_TAINTED
						fprintf(stderr,"d");
		}
		fprintf(stderr,"\n");
		fflush(stderr);
}

// record which signature fragment matched at position <pos>
void record_matched_signature(matched_record** matched_signatures, int startPos, int endPos, int signatureId)
{
	int pos;

	for (pos = startPos; pos <= endPos; ++pos)
	{
		matched_record *matched = (matched_record*) malloc(sizeof(matched_record));
		matched->signatureId = signatureId;
		matched->next = NULL;

		// insert into head of linked list
		matched_record *tmp = matched_signatures[pos];
		matched_signatures[pos] = matched;
		matched->next = tmp;
	}
}

static int look_for_signature(matched_record** matched_signatures, int sigId, int pos)
{
	matched_record *r = matched_signatures[pos];

	while (r)
	{
		if (r->signatureId == sigId)
		{
			return 1;
		}
		r = r->next; 
	}

	return 0;
}

//
// pre: [startPos..endPos] is vetted
//
int appfw_is_from_same_signature(matched_record** matched_signatures, int startPos, int endPos)
{
	char **fw_sigs = appfw_getSignatures();
	int tokenLength = endPos-startPos+1;

	matched_record *tmp = matched_signatures[startPos];
	while (tmp)
	{
		int sigId = tmp->signatureId;
		int pos;
		int found = 1;

		for (pos = startPos + 1; pos <= endPos; ++pos)
		{
			if (look_for_signature(matched_signatures, sigId, pos))
				found++;
			else
				goto signature_not_found;
		}

		if (found == tokenLength)
		{
		/*
			// not an issue right now b/c sqlite3 parser strips out comments
			// so we don't get to examine -- to see whether it's vetted

			// handle the case where the signature is '-'
			// and so we would end up matchng '--' (comment in sql)
			fprintf(stderr,"sigId[%d] length[%d] tokenLength[%d]\n", sigId, strlen(fw_sigs[sigId]), tokenLength);
			if (strlen(fw_sigs[sigId]) == 1 && tokenLength > 1)
				goto signature_not_found;
		*/

			return 1;
		}

signature_not_found:
		tmp = tmp->next; // try the next signature id
	}

	return 0;
}

matched_record** appfw_allocate_matched_signatures(int length)
{
	return calloc(length, sizeof(matched_record*)); 
}

void appfw_deallocate_matched_signatures(matched_record** matched_signatures, int length)
{
	int pos;
	for (pos = 0; pos < length; ++pos)
	{
		delete_matched_record(matched_signatures[pos]);
		matched_signatures[pos] = NULL;
	}
}

// recursively delete
void delete_matched_record(matched_record *r)
{
	if (r == NULL)
	{
		return;
	}
	else
	{
		delete_matched_record(r->next);
		r->next = NULL;
		
		free(r);
	}
}
