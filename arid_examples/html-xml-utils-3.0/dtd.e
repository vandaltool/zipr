#define MAXNAMELEN  10
 typedef struct _ElementType {
  char *name;
  Boolean mixed, empty, stag, etag, pre, break_before, break_after;
  char *parents[60];
} ElementType;
 const ElementType * lookup_element(register const char *str,
       register unsigned int len);
