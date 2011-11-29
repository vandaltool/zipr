 typedef enum { False, True } Boolean;
 typedef unsigned char *string;
 typedef struct _pairlist {
  unsigned char *name, *value;
  struct _pairlist *next;
} *pairlist;
 typedef unsigned int MediaSet;
 enum _Media {
  MediaNone = 0,
  MediaPrint = (1 << 0),
  MediaScreen = (1 << 1),
  MediaTTY = (1 << 2),
  MediaBraille = (1 << 3),
  MediaTV = (1 << 4),
  MediaProjection = (1 << 5),
  MediaEmbossed = (1 << 6),
  MediaAll = 0xFF
};
#define eq(s, t)  (*s == *t && strcmp(s, t) == 0)
#define hexval(c)  ((c) <= '9' ? (c)-'0' : (c) <= 'F' ? 10+(c)-'A' : 10+(c)-'a')
 void pairlist_delete(pairlist p);
 pairlist pairlist_copy(const pairlist p);
 string strapp(string *s,...);
 void chomp(string s);
 inline int min(int a, int b);
 inline int max(int a, int b);
