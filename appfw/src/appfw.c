#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "appfw.h"

#define MAX_SIGNATURE_SIZE 1024

static const char *sigFileEnv = "APPFW_SIGNATURE_FILE";
static int fw_numPatterns = 0;
static char **fw_sigs = NULL;

// read in signature file
// environment variable specifies signature file location
void appfw_init()
{
  static int initialized = 0;
  int numSigs = 0;

  if (initialized) return;

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
  }
  else
  {
    appfw_error("could not open signature file");
  }

  initialized = 1;
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
