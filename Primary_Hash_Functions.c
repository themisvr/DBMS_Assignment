#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Primary_Hash_Functions.h"
#include "Secondary_Hash_Utilities.h"


int HT_CreateIndex(char* fileName, char attrType, char* attrName, int attrLength, int buckets) {
	
	int fd, block_number;
	void *block_data;
	HT_FirstBlock *first_block;

	/* Create an empty file o blocks */	
	if(BF_CreateFile(fileName) < 0) {
		BF_PrintError("Error creating file!\n");
		return -1;
	}
	/* Open block file */
	if((fd = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Error opening file!\n");
		return -1;
	}
	/* Allocate first block which is the Hash_Table */
	if(BF_AllocateBlock(fd) < 0 ) {
		BF_PrintError("Error allocating new Block!\n");
		return -1;
	}
	/* Get block number */
	if((block_number = BF_GetBlockCounter(fd) - 1) < 0) {
		BF_PrintError("Error finding block: No block has been allocated!\n");
		return -1;
	}
	/* Read first's block data */
	if(BF_ReadBlock(fd, block_number, &block_data) < 0) {
		BF_PrintError("Cannot read block!\n");
		return -1;
	} 
	/* Initialize first's block data members, which are the information about the Hash_Table */
	first_block = HT_ByteArrayToFirstBlock(block_data);

	first_block->fileDesc = fd;

	first_block->attrType = attrType;

	strcpy(first_block->attrName, attrName);

	first_block->attrLength = attrLength;
	
	first_block->numBuckets = buckets;
	
	strcpy(first_block->hashfileType, "primary");

	first_block->hashTable = malloc(buckets * sizeof(int));
	
	for(int i = 0; i < first_block->numBuckets; i++)
		first_block->hashTable[i] = Null;
	/* Write block to the disk */
	memcpy(block_data, first_block, sizeof(HT_FirstBlock));
	if(BF_WriteBlock(fd, block_number) < 0) {
		BF_PrintError("Error writing block to the disk!\n");
		return -1;
	}
	/* Close file */
	if(BF_CloseFile(fd) < 0) {
		BF_PrintError("Error closing file!\n");
		return -1;
	}
	free(first_block);

	return 0;
}


HT_info *HT_OpenIndex(char *fileName) {

	int fd;
	void *block_data;
	HT_info *rv;
	HT_FirstBlock *first_block;

	/* open block file */
	if( ( fd = BF_OpenFile(fileName) ) < 0 ) {
		BF_PrintError("Error opening file!\n");
		return NULL;
	}
	/* bring to main memory and read the 1st block of the file */
	if( BF_ReadBlock(fd, 0, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return NULL;
	}
	/* 1st block needs to be reconstructed in order to read data from it */
	first_block = HT_ByteArrayToFirstBlock(block_data);
	/* if the block file opened is not an HT file */
	if( strcmp(first_block->hashfileType, "primary") != 0) {
		printf("Block file is not of primary hash type\n");
		return NULL;
	}
	/* copy to HT_info the valuable information from the 1st block of the file */
	rv = HT_copyHeaderInfo(first_block->fileDesc, first_block->numBuckets);

	free(first_block);

	return rv;
}


int HT_CloseIndex(HT_info *header_info) {

	int fd;
	void *block_data;
	HT_FirstBlock *first_block;

	fd = header_info->fileDesc;

	/* bring first block to main memory */
	if(BF_ReadBlock(fd, 0, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return -1;
	}
	first_block = HT_ByteArrayToFirstBlock(block_data);
	
	/* write block back to the disk */
	memcpy(block_data, first_block, sizeof(HT_FirstBlock));
	if(BF_WriteBlock(fd, 0) < 0) {
		BF_PrintError("Error writing block to the disk!\n");
		return -1;
	}
	/* close the hash file */
	if(BF_CloseFile(fd) < 0) {
		BF_PrintError("Error closing file!\n");
		return -1;
	}
	/* free the allocated memory for the header_info */
	free(header_info);
	/* free is not enough, free just marks the memory as unused, the struct data will be there until overwriting. */
	/* For safety, we set the pointer to NULL after free. */
	header_info = NULL;
	free(first_block);

	return 0;
}


int HT_InsertEntry(HT_info header_info, Record record) {

	int blocknum, cur_block;
	void *block_data;
	HT_FirstBlock *first_block;
	Block *block;

	/* the record is mapped to hashtable[hash_value] */
	int hash_value = integerHash(record.id, header_info.numBuckets);

	/* now read the 1st block of the primary hash file, in order to obtain the hash table structure */
	if(BF_ReadBlock(header_info.fileDesc, 0, &block_data) < 0) {
		printf("Error getting block!\n");
		return -1;
	}
	/* reconstruct the 1st block */
	first_block = HT_ByteArrayToFirstBlock(block_data);
	if(first_block->hashTable[hash_value] == Null) {  	/* if hashTable[hash_value] is not pointing to a block */
		/* this bucket does not point to a block 
		 * so allocate a new block and add the argument
		 * record to this block */

		if(BF_AllocateBlock(header_info.fileDesc) < 0) {
			BF_PrintError("Error allocating new block!\n");
			return -1;
		}
		/* get the number of the newly allocated block */
		if( (blocknum = BF_GetBlockCounter(header_info.fileDesc) - 1 ) < 0) {
			BF_PrintError("Error getting number of blocks!\n");
			return -1;
		}
		/* add a "link" between the bucket and the new block */
		first_block->hashTable[hash_value] = blocknum;
		/* write the 1st block to disk again */
		memcpy(block_data, first_block, sizeof(HT_FirstBlock));
		if(BF_WriteBlock(header_info.fileDesc, 0) < 0) {
			BF_PrintError("Error writing block!\n");
			return -1;
		}
		/* read the newly allocated block */
		if(BF_ReadBlock(header_info.fileDesc, blocknum, &block_data) < 0) {
			printf("Error getting block!\n");
			return -1;
		}
		/* reconstruct the block */
		block = HT_ByteArrayToBlock(block_data);
		addRecordToBlock(&block, record);
		block->next_block = Null;		/* this block points to Null */
		/* now that the record is added, convert the block to byte array again */
		memcpy(block_data, block, sizeof(Block));
		/* write the updated block to disk */
		if(BF_WriteBlock(header_info.fileDesc, blocknum) < 0) {
			printf("Error writing block!\n");
	 	    return -1;
		}
		/* free the allocated memory */
		free(first_block);
		free(block);	
		
		return blocknum;
	}
	else {
		/* this bucket points to a block atm
		 * traverse the "list" of blocks until a non-full
		 * block is found->add the record there or 
		 * traverse until the end of the list is reached
		 * and allocate a new block->add the record there */


		if(BF_ReadBlock(header_info.fileDesc, first_block->hashTable[hash_value], &block_data) < 0) {
			BF_PrintError("Error getting block!\n");
			return -1;
		}
		/* reconstruct the block */
		block = HT_ByteArrayToBlock(block_data);
		if(addRecordToBlock(&block, record) == -1) {	/* the block is full */
		
			cur_block = first_block->hashTable[hash_value];

			while(block->next_block != Null) {	/* there is a next block */
				
				cur_block = block->next_block;
				free(block);
				block = NULL;
				/* get the next block */
				if(BF_ReadBlock(header_info.fileDesc, cur_block, &block_data) < 0) {
					BF_PrintError("Error getting block!\n");
					return -1;
				}
				/* reconstruct the block */
				block = HT_ByteArrayToBlock(block_data);
				if(addRecordToBlock(&block, record) == 0) {	/* there is space in the block that we are currently examining */
					/* add the record to this block */
					memcpy(block_data, block, sizeof(Block));
					if(BF_WriteBlock(header_info.fileDesc, cur_block) < 0) {
						printf("Error writing block!\n");
	 			       	return -1;
					}	
					/* free the allocated memory */
					free(first_block);
					free(block);

					return cur_block;
				}
			}
			/* all the blocks in the list were full, so we need to allocate a new block and add the record there */

			/* allocate a new block */	
			if(BF_AllocateBlock(header_info.fileDesc) < 0) {
				BF_PrintError("Error allocating block!\n");
				return -1;
			}
			/* get the number of the new block */
			if( (blocknum = BF_GetBlockCounter(header_info.fileDesc) - 1 ) < 0) {
				BF_PrintError("Error getting number of blocks!\n");
				return -1;
			}
			/* create a "link" between the last block of the bucket block list and the new block */
			block->next_block = blocknum;
			/* write the last block to disk, after its link was updated */
			memcpy(block_data, block, sizeof(Block));
			if(BF_WriteBlock(header_info.fileDesc, cur_block) < 0) {
				printf("Error writing block!\n");
	 			return -1;
			}	
			free(block);
			block = NULL;
			/* read the newly allocated block */
			if(BF_ReadBlock(header_info.fileDesc, blocknum, &block_data) < 0) {
				printf("Error getting block!\n");
				return -1;
			}
			/* reconstruct the block */
			block = HT_ByteArrayToBlock(block_data);
			addRecordToBlock(&block, record);
			block->next_block = Null;		/* this block points to Null */
			/* now that the record is added, convert the block to byte array again */
			memcpy(block_data, block, sizeof(Block));
			/* write the updated block to disk */
			if(BF_WriteBlock(header_info.fileDesc, blocknum) < 0) {
				printf("Error writing block to the disk!\n");
	 			return -1;
			}	
			free(first_block);
			free(block);

			return blocknum;
		}
		/* record was added at the first block of the bucket block list */	
		else {
			blocknum = first_block->hashTable[hash_value];
			memcpy(block_data, block, sizeof(Block));
			if(BF_WriteBlock(header_info.fileDesc, blocknum) < 0) {
				printf("Error writing block to the disk!\n");
	 			return -1;
			}	
			/* free allocated memory */
			free(first_block);
			free(block);

			return blocknum;
		}
	}
}


int HT_DeleteEntry(HT_info header_info, void *value) {

	int val = *(int *)value;
	int hash_value = integerHash(val, header_info.numBuckets);
	int blocknum;
	void *block_data;
	HT_FirstBlock* first_block;
	Block *block;

	if(BF_ReadBlock(header_info.fileDesc, 0, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return -1;
	}

	first_block = HT_ByteArrayToFirstBlock(block_data);
	/* check if that value exists */
	if(first_block->hashTable[hash_value] == Null)
		return -1;
	else {
		blocknum = first_block->hashTable[hash_value];
		/* bring the block with number "blocknum" to memory */
		if(BF_ReadBlock(header_info.fileDesc, blocknum, &block_data) < 0) {
			BF_PrintError("Error getting block!\n");
			return -1;
		}
		block = HT_ByteArrayToBlock(block_data);
		/* search the block list until we find the record we want to delete */
		while(1) {
			for(int i = 0; i < block->records_Counter; i++) {
				if(block->records[i].id == val) {
					/* we actually put the last record in that spot */
					block->records[i] = block->records[(block->records_Counter) - 1];
					/* set the last record as invalid */
					block->records[(block->records_Counter) - 1].id = Null;
					/* decrease the records_Counter by 1 */
					block->records_Counter--;
					/* write the block back to the disk */
					memcpy(block_data, block, sizeof(Block));
					if(BF_WriteBlock(header_info.fileDesc, blocknum) < 0) {
						BF_PrintError("Error writing block to the disk!\n");
						return -1;
					}

					/* free the allocated memory */
					free(first_block);
					free(block);

					return 0;
				}
			}
			blocknum = block->next_block;
			free(block);
			block = NULL;
			if(BF_ReadBlock(header_info.fileDesc, blocknum, &block_data) < 0) {
				BF_PrintError("Error getting block!\n");
				return -1;
			}
			block = HT_ByteArrayToBlock(block_data);
		}
	}

}


int HT_GetAllEntries(HT_info header_info, void *value) {

	int val = *(int *)value;
	int hash_value = integerHash(val, header_info.numBuckets);
	int blocknum, block_counter = 1;
	void *block_data;
	HT_FirstBlock *first_block;
	Block *block;

	/* get the 1st block of the primary hash file to memory */
	if(BF_ReadBlock(header_info.fileDesc, 0, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return -1;
	}
	/* reconstruct block_data */
	first_block = HT_ByteArrayToFirstBlock(block_data);
	if(first_block->hashTable[hash_value] == Null)
		return -1;
	/* locate the 1st block of the bucket linked list */
	blocknum = first_block->hashTable[hash_value];
	/* get this block to memory and reconstruct it */
	if(BF_ReadBlock(header_info.fileDesc, blocknum, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return -1;
	}
	block = HT_ByteArrayToBlock(block_data);
	
	/* traverse the bucket's block list until you find the block containing a record with record.id = value */
	while(1) {
		block_counter++;
		if(locateRecord(block, val) == 0) break;
		blocknum = block->next_block;
		free(block);
		block = NULL;
		/* check if this is the last block in the bucket list */
		if(blocknum == Null)
			return -1;
		if(BF_ReadBlock(header_info.fileDesc, blocknum, &block_data) < 0) {
			BF_PrintError("Error getting block!\n");
			return -1;
		}
		block = HT_ByteArrayToBlock(block_data);
	}
	/* free the allocated memory */
	free(first_block);
	free(block);

	return block_counter;
}


int HashStatistics(char *filename) {

	int i, fd, next_block, loop = 0, total_overflows = 0, total_blocks = 0, blocks_cnt = 0, records_cnt = 0, hflag = 0, *recs_per_bucket, *blocks_per_bucket, *overflow_buckets;
	void *block_data;
	HT_FirstBlock *first_block;
	SHT_FirstBlock *sht_first_block;
	Block *block;
	SecondaryBlock *sht_block;
	HT_info *hi;


	hi = HT_OpenIndex(filename);
	if(hi == NULL) hflag = 1;

	/* open file */
	if( (fd = BF_OpenFile(filename) ) < 0) {
		BF_PrintError("Error opening file!\n");
		return -1;
	}
	/* access the first block of the file to obtain info */
	if(BF_ReadBlock(fd, 0, &block_data) < 0) {
		BF_PrintError("1)Error getting block!\n");
		return -1;
	}

	/* primary hash file */
	if(hflag == 0) {
		/* reconstruct the first block */
		first_block = HT_ByteArrayToFirstBlock(block_data);
		recs_per_bucket = malloc( first_block->numBuckets * sizeof(int) );
		blocks_per_bucket = malloc( first_block->numBuckets * sizeof(int) );
		overflow_buckets = malloc( first_block->numBuckets * sizeof(int) );
		/* traverse each bucket block list and gather the appropriate info */
		for(i = 0; i < first_block->numBuckets; i++) {

			/* bucket is empty */
			if( (next_block = first_block->hashTable[i]) == Null) {
				recs_per_bucket[i] = Null;
				blocks_per_bucket[i] = Null;
				overflow_buckets[i] = Null;

				continue;
			}

			/* if the bucket's block list is not Null, start traversing it */
			while(next_block != Null) {
				/* read the next block of the bucket block list */
				if(BF_ReadBlock(fd, next_block, &block_data) < 0) {
					BF_PrintError("2)Error getting block!");
					return -1;
				}
				blocks_cnt++;
				block = HT_ByteArrayToBlock(block_data);
				records_cnt += block->records_Counter;
				next_block = block->next_block;
				++loop;
				free(block);
				block = NULL;
			}
			printf("Bucket %d has %d blocks and %d records\n", i, blocks_cnt, records_cnt);
			total_blocks += blocks_cnt;
			recs_per_bucket[i] = records_cnt;
			blocks_per_bucket[i] = blocks_cnt;
			if(loop > 1) { 
				total_overflows++;
				overflow_buckets[i] = loop - 1;
			}
			else
				overflow_buckets[i] = Null;
			blocks_cnt = 0;
			records_cnt = 0;
			loop = 0;
			
		}

		printf("\nTotal blocks in %s: %d\n", filename, total_blocks);
		int min = find_min(recs_per_bucket, first_block->numBuckets);
		int max = find_max(recs_per_bucket, first_block->numBuckets);
		int avg = find_avg(recs_per_bucket, first_block->numBuckets);
		printf("Minimum number of records in a bucket: %d\n", min);
		printf("Maximum number of records in a bucket: %d\n", max);
		printf("Average number of records in a bucket: %d\n", avg);
		avg = find_avg(blocks_per_bucket, first_block->numBuckets);
		printf("Average number of blocks in each bucket: %d\n", avg);
		printf("Number of buckets that have been overflowed: %d\n", total_overflows);
		for(i = 0; i < first_block->numBuckets; i++) {
			if(overflow_buckets[i] == Null) continue;
			printf("Bucket %d has %d overflow blocks\n", i, overflow_buckets[i]);
		}

		/* free allocated memory */
		free(recs_per_bucket);
		free(blocks_per_bucket);
		free(overflow_buckets);
		free(block);
		free(hi);

		/* free hashtable? */
		free(first_block->hashTable);
		free(first_block);
		if(BF_CloseFile(fd) < 0) {
			BF_PrintError("Error closing file!");
			return -1;
		}
	}
	/* secondary hash file */
	else {
		/* reconstruct the first block */
		sht_first_block = SHT_ByteArrayToFirstBlock(block_data);
		recs_per_bucket = malloc( sht_first_block->numBuckets * sizeof(int) );
		blocks_per_bucket = malloc( sht_first_block->numBuckets * sizeof(int) );
		overflow_buckets = malloc( sht_first_block->numBuckets * sizeof(int) );
		/* traverse each bucket block list and gather the appropriate info */
		for(i = 0; i < sht_first_block->numBuckets; i++) {

			/* bucket is empty */
			if( (next_block = sht_first_block->SecondaryHashTable[i]) == Null) {
				recs_per_bucket[i] = Null;
				blocks_per_bucket[i] = Null;
				overflow_buckets[i] = Null;

				continue;
			}

			/* if the bucket's block list is not Null, start traversing it */
			while(next_block != Null) {
				/* read the next block of the bucket block list */
				if(BF_ReadBlock(fd, next_block, &block_data) < 0) {
					BF_PrintError("2)Error getting block!");
					return -1;
				}
				blocks_cnt++;
				sht_block = SHT_ByteArrayToBlock(block_data);
				records_cnt += sht_block->records_Counter;
				next_block = sht_block->next_block;
				++loop;
				free(sht_block);
				sht_block = NULL;
			}
			printf("Bucket %d has %d blocks and %d records\n", i, blocks_cnt, records_cnt);
			total_blocks += blocks_cnt;
			recs_per_bucket[i] = records_cnt;
			blocks_per_bucket[i] = blocks_cnt;
			if(loop > 1) { 
				total_overflows++;
				overflow_buckets[i] = loop - 1;
			}
			else
				overflow_buckets[i] = Null;
			blocks_cnt = 0;
			records_cnt = 0;
			loop = 0;
			
		}

		printf("\nTotal blocks in %s: %d\n", filename, total_blocks);
		int min = find_min(recs_per_bucket, sht_first_block->numBuckets);
		int max = find_max(recs_per_bucket, sht_first_block->numBuckets);
		int avg = find_avg(recs_per_bucket, sht_first_block->numBuckets);
		printf("Minimum number of records in a bucket: %d\n", min);
		printf("Maximum number of records in a bucket: %d\n", max);
		printf("Average number of records in a bucket: %d\n", avg);
		avg = find_avg(blocks_per_bucket, sht_first_block->numBuckets);
		printf("Average number of blocks in each bucket: %d\n", avg);
		printf("Number of buckets that have been overflowed: %d\n", total_overflows);
		for(i = 0; i < sht_first_block->numBuckets; i++) {
			if(overflow_buckets[i] == Null) continue;
			printf("Bucket %d has %d overflow blocks\n", i, overflow_buckets[i]);
		}

		/* free allocated memory */
		free(recs_per_bucket);
		free(blocks_per_bucket);
		free(overflow_buckets);
		free(sht_block);

		/* free hashtable? */
		free(sht_first_block->SecondaryHashTable);
		free(sht_first_block);
		if(BF_CloseFile(fd) < 0) {
			BF_PrintError("Error closing file!");
			return -1;
		}

	}
	
	return 0;
}
