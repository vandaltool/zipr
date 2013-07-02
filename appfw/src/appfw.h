#ifndef APPFW_INIT
#define APPFW_INIT

#define MAX_COMMAND_LENGTH 65536

enum { APPFW_BLESSED, APPFW_TAINTED, APPFW_SECURITY_VIOLATION, APPFW_BLESSED_KEYWORD };

typedef struct matched_record {
	int                    signatureId;
	struct matched_record  *next;
} matched_record;

extern void appfw_init();               // load/initialize signature patterns off signature file (specified via env. variable)  
extern int appfw_isInitialized();
extern int appfw_getNumSignatures();    // returns number of signature patterns
extern char **appfw_getSignatures();    // returns array containing signature patterns
extern void appfw_error(const char*);   // generic error display routine

extern void appfw_taint_range(char *taint, char taintValue, int from, int len); // mark as tainted
extern void appfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint);
extern void appfw_establish_taint(const char *input, char *taint, matched_record**, int case_sensitive); 
	// return tainted portion of input string
extern matched_record** appfw_allocate_matched_signatures(int size);
extern void appfw_deallocate_matched_signatures(matched_record**, int size);
extern int appfw_is_from_same_signature(matched_record**, int startPos, int endPos);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif


