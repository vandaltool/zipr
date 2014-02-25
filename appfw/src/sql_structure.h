#ifndef SQL_STRUCTURE_H
#define SQL_STRUCTURE_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

void initQueryStructureCache(const char *p_filename);
void saveQueryStructureCache(const char *p_filename);
void addQueryStructure(char *p_queryStructure);
int findQueryStructure(char *p_queryStructure);

#endif


