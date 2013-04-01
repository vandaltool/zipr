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

static  void reset_sig_file_env_var()
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
  int numSigs = 0;

  if (appfw_isInitialized()) return;

  char *signatureFile = getenv(sigFileEnv);
  if (!signatureFile)
  {
	if(getenv("APPFW_VERBOSE"))
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
      fw_sigs[numSigs] = (char *) malloc(strlen(buf) + 128);
      strncpy(fw_sigs[numSigs], buf, strlen(buf));
      fw_sigs[numSigs][strlen(buf)-1] = '\0';

      numSigs++;
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

// buffers must be big enough
void appfw_establish_taint(const char *command, char *taint)
{
  int i, j, pos;
  int patternFound;
  char **fw_sigs = appfw_getSignatures();
  int commandLength = strlen(command);
  taint[commandLength] = '\0';

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
  pos = 0;

  int numSignatures =appfw_getNumSignatures();  
  while (pos < commandLength)
  {
    for (i = 0; i < numSignatures; ++i)
	{
	    int length_signature = strlen(fw_sigs[i]);
	    if (strncasecmp(&command[pos], fw_sigs[i], length_signature) == 0)
	    {
		  appfw_taint_range(taint, APPFW_BLESSED, pos, length_signature);
		  break;
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
