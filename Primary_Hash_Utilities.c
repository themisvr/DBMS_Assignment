#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "Primary_Hash_Utilities.h"


/* function which converts a HT_FirstBlock byte array(blockdata) to a readable FirstBlock struct */
HT_FirstBlock *HT_ByteArrayToFirstBlock(void *blockdata) {

	/* allocate memory for a FirstBlock */
	HT_FirstBlock *first_block = malloc( sizeof(HT_FirstBlock) );
	/* reconstruct the blockdata(byte array) to a readable FirstBlock struct */
	memcpy(first_block, blockdata, sizeof(HT_FirstBlock));

	return first_block;
}

/* function which converts an (HT_)Block byte array(blockdata) to a readable Block struct */
Block *HT_ByteArrayToBlock(void *blockdata) {

	/* allocate memory for a Block */
	Block *block = malloc( sizeof(Block) );
	/* reconstruct the blockdata(byte array) to a readable Block struct */
	memcpy(block, blockdata, sizeof(Block));

	return block;
}


/* function which copies HT_info */
HT_info *HT_copyHeaderInfo(int fd, long int buckets) {

	HT_info *header_info = malloc( sizeof(HT_info) );
	header_info->fileDesc = fd;
	header_info->attrType = 'i';	/* the primary key is the field (int) id */
	strcpy(header_info->attrName, "id");
	header_info->attrLength = 2;	/* 'i', 'd' */
	header_info->numBuckets = buckets;

	return header_info;
}


int addRecordToBlock(Block **block, Record rec) {

	/* in case block is full */
	if((*block)->records_Counter == MAX_RECORDS)	return -1;
	/* add record to block */
	(*block)->records[(*block)->records_Counter++] = rec;

	return 0;
}


int locateRecord(Block *block, int key) {

	for(int i = 0; i < block->records_Counter; i++) {
		if(block->records[i].id == key) {
			printf("%d\t%s\t%s\t%s\n", block->records[i].id, block->records[i].name, block->records[i].surname, block->records[i].address);

			return 0;
		}
	}

	return -1;
}


int integerHash(int key, long int buckets) {

	int hv = 0, a = 33, b = 71, p = 5381; 
	
	hv = ( ( (a * key) + b) % p) % buckets;

	return hv;
}


int find_min(int arr[], long int cols) {

	int i, min = arr[0];

	for(i = 1; i < cols; i++)
		if(min > arr[i])
			min = arr[i];

	return min;
}


int find_max(int arr[], long int cols) {

	int i, max = arr[0];

	for(i = 1; i < cols; i++)
		if(max < arr[i])
			max = arr[i];

	return max;
}


int find_avg(int arr[], long int cols) {

	int i, sum = 0;

	for(i = 0; i < cols; i++)
		sum += arr[i];

	return sum / cols;
}
