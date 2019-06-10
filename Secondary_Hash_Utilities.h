#ifndef SHU_H
#define SHU_H

#include "Records.h"

SHT_FirstBlock *SHT_ByteArrayToFirstBlock(void *);
SecondaryBlock *SHT_ByteArrayToBlock(void *);
int StringHash(char *, long int);
int CheckAttrName(SHT_info header_info);
int SHT_addRecordToBlock(SecondaryBlock **, SecondaryRecord);


#endif
