#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Secondary_Hash_Utilities.h"

SHT_FirstBlock *SHT_ByteArrayToFirstBlock(void *blockdata) {

	/* allocate memory for a SecondaryFirstBlock */
	SHT_FirstBlock *secondaryfirst_block = malloc( sizeof(SHT_FirstBlock) );
	/* reconstruct the blockdata(byte array) to a readable SecondaryFirstBlock struct */
	memcpy(secondaryfirst_block, blockdata, sizeof(SHT_FirstBlock));

	return secondaryfirst_block;
}


SecondaryBlock *SHT_ByteArrayToBlock(void *blockdata) {

	SecondaryBlock *Secblock = malloc( sizeof(SecondaryBlock) );
	
	memcpy(Secblock, blockdata, sizeof(SecondaryBlock));

	return Secblock;
}


int CheckAttrName(SHT_info header_info) {

	if( strcmp(header_info.attrName, "name") == 0)
		return 1;
	else if( strcmp(header_info.attrName, "surname") == 0)
		return 2;
	else if( strcmp(header_info.attrName, "address") == 0)
		return 3;
	else 
		return -1;
}


int StringHash(char *key, long int buckets) {

	int hv = 0, a = 33, p = 5381; 
	
	while(*key != '\0') {
		hv = ( (hv * a) + *key ) % p;	
		key++;	
	}

	return (hv % buckets);
}


int SHT_addRecordToBlock(SecondaryBlock **block, SecondaryRecord rec) {

	/* in case block is full */
	if((*block)->records_Counter == MAX_RECORDS)	return -1;
	/* add record to block */
	(*block)->SecRecords[(*block)->records_Counter++] = rec;

	return 0;
}
