 typedef struct entry {char *key; void *data;} ENTRY;
 typedef enum {FIND, ENTER} ACTION;
 int hcreate(size_t nel);
 void hdestroy(void);
 ENTRY *hsearch(ENTRY item, ACTION action);
