#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <list>
#include <sys/time.h>


extern "C" 
{
#include "appfw.h"
}

using namespace std;

#define MAX_SIGNATURE_SIZE 1024

static const char *sigFileEnv = "APPFW_SIGNATURE_FILE";
static int fw_numPatterns = 0;
static unsigned int fw_size = 512;
static char **fw_sigs = NULL;
static int appfw_initialized = 0;

static void reset_sig_file_env_var()
{
	extern char **environ;
	int i;
	for(i=0;(environ[i]!=0);i++)
	{
		if(getenv("APPFW_ENV_VERBOSE"))
			fprintf(stderr,"environ[i]=%s\n",environ[i]);
		/* check that the environ has the key followed by an equal */
		if(strncmp(environ[i],"LD_PRELOAD=",strlen("LD_PRELOAD="))==0)
		{
			if(getenv("APPFW_VERBOSE"))
				fprintf(stderr,"Found ld_preload=\n");
			char* start=strstr(environ[i],"libappfw.so");
			if(start)
			{
				/* example1: 	s=start, e=end (at memmove)
				 * LD_PRELOAD=/home/stuff/libappfw.so 
				 *            s                      e
				 * example 2:
				 * LD_PRELOAD=/foo/bar/libc.so /home/stuff/libappfw.so 
				 *                             s                      e
				 * example 3:
				 * LD_PRELOAD=/home/stuff/libappfw.so /lib/libc.so 
				 *            s                       e
				 * 
				 * example 4:
				 * LD_PRELOAD=/lib/other/libo.so /home/stuff/libappfw.so /lib/libc.so 
				 *             			 s                       e
				 */
				  
				char* end=start+strlen("libappfw.so");
				// back up until we find an equal, space, or colon signifying the start of the libappfw.so filename 
				while( *start!='=' && *start!=' ' && *start!=':' )
					start--;
				// save the equal, space or colon
				start++;

				// don't skip a null terminator.
				if(*end!='\0') 
					end++;	// if there's a space or colon, skip it

				memmove(start, end, strlen(end)+1); 	// make sure you memmove the null terminator too.
				
				// log results
				if(getenv("APPFW_VERBOSE"))
				{
					fprintf(stderr,"Removing ld_preload for sub libraries: %s\n", environ[i]);
				}
			}

		}
	}
}

void appfw_init_ctor() __attribute__(( constructor ));
void appfw_init_ctor()
{
	if(getenv("APPFW_VERBOSE"))
	{
		fprintf(stderr, "Proc %d: library constructor initialization started\n", getpid());
	}

	appfw_init();

	if(getenv("APPFW_VERBOSE"))
	{
		FILE* f=fopen("/proc/self/cmdline", "r");
		if(f)
		{
			char c;
			fprintf(stderr, "Proc %d: Command is: ", getpid());
			while(!feof(f))
			{
				int res=fscanf(f,"%c", &c);
				assert(res);
				fprintf(stderr, "%c", c);
			}
			fclose(f);
			fprintf(stderr, "\n");
		}
		else
		{
			fprintf(stderr, "Prod %d: Unable to print command line\n", getpid());
		}	
		fprintf(stderr, "Proc %d: library constructor initialization finished\n", getpid());
	}
}

extern "C" int _read_signatures_from_file(const char *p_file)
{
	int numSigs = 0;
	int verbose=0;
	if(getenv("APPFW_VERBOSE"))
		verbose=1;

	FILE *sigF = fopen(p_file, "r");
	if (sigF)
	{
		char buf[MAX_SIGNATURE_SIZE];

		fw_sigs = (char**)malloc(sizeof(char*) * fw_size);
		assert( fw_sigs != NULL );

		while (fgets(buf, MAX_SIGNATURE_SIZE, sigF) != NULL)
		{
			if (strlen(buf) > 1) // don't want "\n" by itself
			{
				if (numSigs == fw_size)
				{
					char **new_p;
					fw_size *= 2;
					new_p = (char**)realloc(fw_sigs, sizeof(char*) * fw_size);
					assert( new_p != NULL );
					fw_sigs = new_p;
				}
				fw_sigs[numSigs] = (char *) malloc(strlen(buf) + 1);
				assert( fw_sigs[numSigs] != NULL );
				strncpy(fw_sigs[numSigs], buf, strlen(buf));
				fw_sigs[numSigs][strlen(buf)-1] = '\0';

				if(verbose && getenv("VERY_VERBOSE"))
					fprintf(stderr,"read sig[%d]: %s (%d)\n", numSigs, fw_sigs[numSigs], (int)strlen(fw_sigs[numSigs]));

				numSigs++;
			}
		}

		fw_numPatterns = numSigs;
		fclose(sigF);
		if(verbose)
			fprintf(stderr, "Proc %d: appfw init finished, nsigs=%d\n", getpid(),numSigs);
	}
	else
	{
		fprintf(stderr, "no signature file found from proc %d\n", getpid());
	}

	return numSigs;
}

// read in signature file
// environment variable specifies signature file location
extern "C" void appfw_init()
{
	if (appfw_isInitialized()) return;

	int i;
	int verbose=0;
	if(getenv("APPFW_VERBOSE"))
		verbose=1;
	int numSigs = 0;

	char *signatureFile = getenv(sigFileEnv);
	if (!signatureFile)
	{
		if(verbose)
			fprintf(stderr, "no signature file found from proc %d\n", getpid());
		return;
	}

	reset_sig_file_env_var();

	FILE *sigF = fopen(signatureFile, "r");
	if (sigF)
	{
		char buf[MAX_SIGNATURE_SIZE];

		fw_sigs = (char**)malloc(sizeof(char*) * fw_size);
		assert( fw_sigs != NULL );

		while (fgets(buf, MAX_SIGNATURE_SIZE, sigF) != NULL)
		{
			if (strlen(buf) > 1) // don't want "\n" by itself
			{
				if (numSigs == fw_size)
				{
					char **new_p;
					fw_size *= 2;
					new_p = (char**)realloc(fw_sigs, sizeof(char*) * fw_size);
					assert( new_p != NULL );
					fw_sigs = new_p;
				}
				fw_sigs[numSigs] = (char *) malloc(strlen(buf) + 1);
				assert( fw_sigs[numSigs] != NULL );
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
		if(verbose)
			fprintf(stderr, "Proc %d: appfw init finished, nsigs=%d\n", getpid(),numSigs);
	}
	else
	{
		if(verbose)
			fprintf(stderr,"(proc: %d): Could not open signature file: %s\n", getpid(), signatureFile);
		appfw_initialized = 0;
	}

	fflush(stderr);
}

extern "C" void appfw_init_from_file(const char *p_file)
{
	if (appfw_isInitialized()) return;

	reset_sig_file_env_var();

	if (_read_signatures_from_file(p_file) > 0)
		appfw_initialized = 1;
}


extern "C" int appfw_isInitialized()
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

void appfw_taint_range_by_pos(char *taint, char taintValue, int beg, int end)
{
	memset(&taint[beg], taintValue, end-beg+1);
}

void appfw_display_signatures(char **fw_sigs, int numSigs)
{
	int i;
	for (i = 0; i < numSigs; ++i)
	{
		fprintf(stderr,"sig[%d]: [%s]\n", i, fw_sigs[i]);
	}
}

extern "C" void appfw_empty_taint(const char *command, char *taint)
{
	int commandLength = strlen(command);
	// set taint markings to 'tainted' by default
	appfw_taint_range(taint, APPFW_TAINTED, 0, commandLength);
}

//
// determine which parts of the command are blessed
// warning: buffers must be big enough
//
void appfw_establish_blessed(const char *command, char *taint, int case_sensitive)
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
		if(getenv("APPFW_VERBOSE"))
			fprintf(stderr,"No appfw signatures loaded.  Blessing entire range. proc:%d \n", getpid());
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
			}
		}
		pos++;
	}
}

// enum { APPFW_BLESSED, APPFW_TAINTED, APPFW_SECURITY_VIOLATION, APPFW_SECURITY_VIOLATION2, APPFW_BLESSED_KEYWORD };
extern "C" void appfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint)
{
		int i;
		fprintf(stderr,"proc %d: %s: %s\n", getpid(), p_msg, p_query);
		fprintf(stderr,"proc %d: %s: ", getpid(), p_msg);
		for (i = 0; i < strlen(p_query); ++i)
		{
				if (p_taint[i] == APPFW_BLESSED)
						fprintf(stderr,"b");
				else if (p_taint[i] == APPFW_SECURITY_VIOLATION)
						fprintf(stderr,"v");
				else if (p_taint[i] == APPFW_SECURITY_VIOLATION2)
						fprintf(stderr,"w");
				else if (p_taint[i] == APPFW_BLESSED_KEYWORD)
						fprintf(stderr,"k");
				else if (p_taint[i] == APPFW_CRITICAL_TOKEN)
						fprintf(stderr,"c");
				else if (p_taint[i] == APPFW_UNKNOWN)
						fprintf(stderr,"-");
				else // APPFW_TAINTED
						fprintf(stderr,"t");
		}
		fprintf(stderr,"\n");
		fflush(stderr);
}

extern "C" 
int is_security_violation(char c)
{
  return (c==APPFW_SECURITY_VIOLATION || c==APPFW_SECURITY_VIOLATION2);
}

extern "C" 
int is_blessed(char c)
{
  return (c==APPFW_BLESSED || c==APPFW_BLESSED_KEYWORD);
}

int count_violations(char* taint,int len)
{
	int count=0;
	for(int i=0;i<len;i++)
	{
		if(is_security_violation(*taint))
			count++;
		++taint;
	}	
	return count;
}

int fix_violations(char* taint, int value, int start, int len)
{
	int count=0;
	for(int i=start;i<start+len;i++)
	{
		if(is_security_violation(taint[i]))
		{
			count++;
			taint[i]=value;
		}	
	}
	return count;
}

//
// mark critical tokens as blessed if wholly contained in the signature
// (implement same-fragment-origin-policy)
//
int fix_violations_sfop(char *taint, int value, int start, const char *sig)
{
	int verbose = getenv("APPFW_VERBOSE") ? TRUE : FALSE;
	int veryverbose = getenv("VERY_VERBOSE") ? TRUE : FALSE;
	int count=0;
	int siglen = strlen(sig);
	int lastpos = start + siglen - 1;
 	int beforefirstpos = start - 1;
	int i;
	char v;
	int beg, end;

	 if (is_security_violation(taint[lastpos+1]) &&
	         beforefirstpos >= 0 && is_security_violation(taint[beforefirstpos]))
	{
		// security violation both before and after
		// e.g.: query:   SELECT * FROM
		//         sig:    ELE
		return 0;
	}

// 012345678901234567890
// SELECT * FROM users where userid = 'sdfadfadfafd';
// vvvvvv---wwww-------vvvvv--------w---------------vb
//        * FRO
//           ROM users
// fixed 1 violations at 7 sig[* FRO]
// Considering sig * FRO
//sig[* FRO] orig[7..11] effective[7..9]
//blessing 9
//fixed 1 violations at 7 sig[* FRO]

	end = lastpos;
	if (is_security_violation(taint[lastpos]) &&
	    is_security_violation(taint[lastpos+1]) &&
	    taint[lastpos] == taint[lastpos+1]) 
	{
		// partial match on last critical token
		end = start - 1; 
		for (i = lastpos, v = taint[lastpos]; i >= start; i--)
		{
			if (taint[i] != v)
			{
				end = i;
				break;
			}
		}
	}

// sig[';] orig[64..65] effective[66..65]
// SELECT * from us/**/ers where userid='adfadssssssssssssssssssss';
//                                                                -v
// sig[';] orig[39..40] effective[41..40]
//  SELECT * from us/**/ers where userid='';
// 01234567890123456789012345678901234567890
// partial match at beginning bfs[38] start[39] [5..5]


	beg = start;
	if (beforefirstpos >= 0 && 
	     is_security_violation(taint[start]) &&
	     is_security_violation(taint[beforefirstpos]) &&
	     taint[start] == taint[beforefirstpos])
	{
		// partial match on first critical token
		beg = end + 1; // set past the end on purpose
		for (i = start+1, v = taint[start]; i <= lastpos; ++i)
		{
			if (taint[i] != v)
			{
				beg = i + 1;
				break;
			}
		}
	}

	// only bless those critical tokens that are fully
	// contained in the signature
	if (veryverbose)
		fprintf(stderr,"sig[%s] orig[%d..%d] effective[%d..%d]\n", sig, start, lastpos, beg, end);

	for(i=beg;i<=end;i++)
	{
		if(is_security_violation(taint[i]))
		{
			count++;
			taint[i]=value;
			if (veryverbose)
				fprintf(stderr,"blessing %d\n", i);
		}	
	}

	return count;
}

static list<char*> *mru_sigs=NULL;  // sorted signatures (most-recently used)

extern "C" void appfw_dump_signatures(FILE *fp)
{
	if (mru_sigs)
	{
		for(list<char*>::iterator it=mru_sigs->begin(); it!=mru_sigs->end();  it++)
		{
			char *sig = *it;
			fprintf(fp,"%s\n", sig);
		}
	}
	else
	{
		static char **fw_sigs = appfw_getSignatures();
		for (int sigId = 0; sigId < appfw_getNumSignatures(); ++sigId)
		{
			fprintf(fp,"%s\n", fw_sigs[sigId]);
		}
	}
}


/*
 * appfw_establish_taint_fast2 - quickly establish taint markings to verify the command is OK
 * taint array should have all critical tokens identified
 * need to known boundaries of each critical token to implement same-fragment-origin
 * policy
 *
 * We use 'v' and 'w' to denote critical tokens
 * SELECT * from users WHERE userid = 'john';# hello
 * vvvvvv---wwww-------vvvvv--------w-------vwwwwwww
 *
 * Algo:
 *   Iterate over taint markings
 *      grab next critical token (consisting of all 'v' or 'c')
 *      Iterate over signatures 
 *         when found, move-to-front
 *         bless the token
 *   
 * Algo 2:
 *   Iterate over all signatures
 *      Do we match the query?
 *      Are all keywords in the matched portion of the query completely covered?
 *          Yes: bless it! move signature fragment to front
 *           No: next signature
 *
 * return TRUE if command is OK.
 */
extern "C" int appfw_establish_taint_fast2(const char *command, char *taint, int case_sensitive)
{
	static char **fw_sigs = appfw_getSignatures();

	int j, pos, sigId;
	int patternFound;
	int commandLength = strlen(command);
	taint[commandLength] = '\0';
	int verbose=getenv("APPFW_VERBOSE")!=NULL;
	int very_verbose=getenv("VERY_VERBOSE")!=NULL;
	verbose+=very_verbose;

	struct timeval blah, blah2;
	if(verbose)
	{
		gettimeofday(&blah,NULL);
		fprintf(stdout, "appfw: match: start: %d:%d ", blah.tv_sec, blah.tv_usec);
	}

	if (!fw_sigs)
	{
		if(verbose)
			fprintf(stderr,"No appfw signatures loaded.  Blessing entire range. proc:%d \n", getpid());
		appfw_taint_range(taint, APPFW_BLESSED, 0, commandLength);
		return TRUE;
	}
	int numSignatures = appfw_getNumSignatures();  

	if(!mru_sigs)
	{
//		gettimeofday(&blah,NULL);
		mru_sigs=new list<char*>;
		assert(mru_sigs);

		for (int sigId = 0; sigId < numSignatures; ++sigId)
		{
			mru_sigs->push_back(fw_sigs[sigId]);
		}

//		gettimeofday(&blah2,NULL);
//		fprintf(stderr, "sqlfw: caching sigs: %f\n",
//			((blah2.tv_sec - blah.tv_sec) * 1000000.0 + (blah2.tv_usec - blah.tv_usec)) / 1000.0);
			
	}

	int violations=count_violations(taint, commandLength);
	if(verbose)
		fprintf(stderr,"Found %d violations\n", violations);

	list<char*>::iterator next;

	int list_depth=0;
	
	/* iterate the list */
	for(list<char*>::iterator it=mru_sigs->begin(); it!=mru_sigs->end();  it=next)
	{
		list_depth++;
		
		char* sig=*it;
		if(very_verbose)
			fprintf(stderr,"Considering sig %s\n", sig);
		next=it;
		++next;	
		int length_signature = strlen(sig);
		pos = 0;

		if(length_signature==1 && isalpha(*sig))
			continue;

		bool alreadymoved = false;
		while (pos < commandLength)
		{
			if (((case_sensitive  && strncmp    (&command[pos], sig, length_signature) == 0)) || 
			    ((!case_sensitive && strncasecmp(&command[pos], sig, length_signature) == 0)) )
			{

				/* fix any violations we found with the match */
				int fixed_violations=fix_violations_sfop(taint, APPFW_BLESSED, pos, sig);
		
				if(fixed_violations)
				{
					if(verbose)
					{
						fprintf(stderr,"fixed %d violations at %d sig[%s]\n", fixed_violations,pos,sig);
						fflush(stderr);
					}
					/* move to front */
					if(!alreadymoved && it!=mru_sigs->begin())
					{
						it = mru_sigs->erase(it);
						mru_sigs->push_front(sig);
						alreadymoved = true;
					}

					violations-=fixed_violations;
					if(violations<=0)
					{
						if(verbose)
						{
							fprintf(stderr,"fixed ALL violations, list size=%d, iterated to %d\n", mru_sigs->size(), list_depth);
							fflush(stderr);
							gettimeofday(&blah,NULL);
							fprintf(stdout, "end: %d:%d \n", blah.tv_sec, blah.tv_usec);
						}
						return TRUE;
					}
				}


			}
			pos++;
		} // end while loop over command string
	} // end iterator over sigs
	if(verbose)
	{
		fprintf(stderr,"failed to fix all violations, %d remain\n", violations);
	}
	return FALSE;
}
