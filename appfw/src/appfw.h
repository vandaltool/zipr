#ifndef APPFW_INIT
#define APPFW_INIT

#define MAX_COMMAND_LENGTH 65536

enum { APPFW_BLESSED, APPFW_TAINTED, APPFW_SECURITY_VIOLATION, APPFW_BLESSED_KEYWORD, APPFW_UNKNOWN, APPFW_CRITICAL_TOKEN };

typedef struct matched_record {
	int                    signatureId;
	struct matched_record  *next;
} matched_record;

void appfw_init();               // load/initialize signature patterns off signature file (specified via env. variable)  
void appfw_init_from_file(const char *p_file);
int appfw_isInitialized();
int appfw_getNumSignatures();    // returns number of signature patterns
char **appfw_getSignatures();    // returns array containing signature patterns
void appfw_error(const char*);   // generic error display routine

void appfw_taint_range(char *taint, char taintValue, int from, int len); // mark as tainted
void appfw_taint_range_by_pos(char *taint, char taintValue, int beg, int end); // mark as tainted
void appfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint);
extern void appfw_establish_taint(const char *input, char *taint, matched_record**, int case_sensitive); 
int appfw_establish_taint_fast(const char *input, char *taint, int case_sensitive); 
void appfw_empty_taint(const char *command, char *taint, matched_record** matched_signatures, int case_sensitive);


	// return tainted portion of input string
matched_record** appfw_allocate_matched_signatures(int size);
void appfw_deallocate_matched_signatures(matched_record**, int size);

int appfw_is_from_same_signature(matched_record**, int startPos, int endPos);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif


