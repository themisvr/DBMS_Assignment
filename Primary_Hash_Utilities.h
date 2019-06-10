#ifndef PHU_H
#define PHU_H

#include "Records.h"

int integerHash(int, long int);
int addRecordToBlock(Block **, Record);
int locateRecord(Block *, int);
int find_min(int *, long int);
int find_max(int *, long int);
int find_avg(int *, long int);
HT_FirstBlock *HT_ByteArrayToFirstBlock(void *);
HT_info *HT_copyHeaderInfo(int, long int);
Block *HT_ByteArrayToBlock(void *);

#endif
