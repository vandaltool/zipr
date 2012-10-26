#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "appfw.h"

#define MAX_SIGNATURE_SIZE 1024

static const char *sigFileEnv = "APPFW_SIGNATURE_FILE";
static int fw_numPatterns = 0;
static char **fw_sigs = NULL;
static int appfw_initialized = 0;

// read in signature file
// environment variable specifies signature file location
void appfw_init()
{
  int numSigs = 0;

  if (appfw_isInitialized()) return;

  char *signatureFile = getenv(sigFileEnv);
  if (!signatureFile)
  {
    appfw_error("no signature file found");
  }

  fw_sigs = malloc(sizeof(char*) * 20000); // allow for 20000 signature patterns

  FILE *sigF = fopen(signatureFile, "r");
  if (sigF)
  {
    char buf[MAX_SIGNATURE_SIZE];
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
  }
  else
  {
    appfw_error("could not open signature file");
    appfw_initialized = 0;
  }
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
  int i;
  for (i = from; i < from+len; ++i)
    taint[i] = taintValue;
}

void appfw_establish_taint(const char *input, char *taint)
{
  int i, j, pos;
  int tainted_marking = APPFW_TAINTED;
  int not_tainted_marking = APPFW_BLESSED;
  int patternFound;
  char **fw_sigs = appfw_getSignatures();

  if (!fw_sigs || !sqlfw_isInitialized())
    return;

  // set taint markings to 'tainted' by default
  appfw_taint_range(taint, tainted_marking, 0, strlen(input));

  // use simple linear scan for now // list of signature patterns are sorted in reverse length order already
  // unset taint when match is found
  pos = 0;
  while (pos < strlen(input))
  {
	// when we'll have reg. expressions for patterns
	// we'd need to make sure we go through all the regex patterns first
	patternFound = 0;
    for (i = 0; i < appfw_getNumSignatures() && !patternFound; ++i)
	{
	  int length_signature = strlen(fw_sigs[i]);
	  if (length_signature >= 1 && length_signature <= (strlen(input) - pos))
	  {
	    if (strncasecmp(&input[pos], fw_sigs[i], strlen(fw_sigs[i])) == 0)
	    {
		  appfw_taint_range(taint, not_tainted_marking, pos, strlen(fw_sigs[i]));
	      patternFound = 1;
		}
      }
	}

    pos++;
  }
}

void appfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint)
{
	int i;
	fprintf(stderr,"%s: %s\n", p_msg, p_query);
	fprintf(stderr,"%s: ", p_msg);
	for (i = 0; i < strlen(p_query); ++i)
	{
		if (p_taint[i] == APPFW_BLESSED)
			fprintf(stderr," ");
		else if (p_taint[i] == APPFW_SECURITY_VIOLATION)
			fprintf(stderr,"X");
		else
			fprintf(stderr,"-");
	}
	fprintf(stderr,"\n");
}
