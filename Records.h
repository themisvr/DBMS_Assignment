#ifndef RECORDS_H
#define RECORDS_H

#include "BF.h"

#define MAX_RECORDS BLOCK_SIZE/sizeof(Record) 
#define Null -1

typedef struct {
	int id;
	char name[15];
	char surname[20];
	char address[40];
} Record;

typedef struct {
	int records_Counter;
	int next_block;
	Record records[MAX_RECORDS];
} Block;

typedef struct {
	int fileDesc;
	char attrType;
	char attrName[3]; /* {'i', 'd', '\0'} */
	int attrLength;
	int *hashTable;
	char hashfileType[8];  /* "primary" */
	long int numBuckets;
} HT_FirstBlock;
	

typedef struct {
	int fileDesc;
	char attrType;
	char attrName[3];
	int attrLength;
	long int numBuckets;
} HT_info;

////////////////////////////////////////////////////////////////////////////////////////////////////////

/* these structs are for the secondary hash table */

typedef struct {
	Record record;
	int blockId;
} SecondaryRecord;

typedef struct {
	int records_Counter;
	int next_block;
	SecondaryRecord SecRecords[MAX_RECORDS];
} SecondaryBlock;

typedef struct {
	int fileDesc;
	char attrName[20];
	int attrLength;
	char hashfileType[10];  /* "secondary" */
	long int numBuckets;
	int *SecondaryHashTable;
	char fileName[100];
} SHT_FirstBlock;

typedef struct {	
	int fileDesc;
	char attrName[20];
	int attrLength;
	long int numBuckets;
	char fileName[100];
} SHT_info;


#endif
