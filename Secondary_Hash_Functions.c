#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Secondary_Hash_Utilities.h"
#include "Secondary_Hash_Functions.h"
#include "Primary_Hash_Utilities.h"

int SHT_CreateSecondaryIndex(char *sfileName, char *attrName, int attrLength, int buckets, char *fileName) {

	int fd1, fd2,  block_number, next_block;
	void *block_data;
	HT_FirstBlock *ht_first_block;
	Block* block;
	SHT_FirstBlock *secondary_FirstBlock;
	SHT_info shi;
	SecondaryRecord srec;

	/* create an empty file of block */
	if(BF_CreateFile(sfileName) < 0) {
		BF_PrintError("Error creating block\n");
		return -1;
	}
	/* Open sht block file */
	if((fd1 = BF_OpenFile(sfileName)) < 0) {
		BF_PrintError("Error opening file!\n");
		return -1;
	}
	/* Allocate first block which is the Secondary Hash Table */
	if(BF_AllocateBlock(fd1) < 0 ) {
		BF_PrintError("Error allocating new Block!\n");
		return -1;
	}
	/* Get block number */
	if((block_number = BF_GetBlockCounter(fd1) - 1) < 0) {
		BF_PrintError("Error finding block: No block has been allocated!\n");
		return -1;
	}
	/* bring first's block data to main memory */
	if(BF_ReadBlock(fd1, block_number, &block_data) < 0) {
		BF_PrintError("Cannot read block!\n");
		return -1;
	}
	/* Initialize first's block data members, which are the information about the Secondary_Hash_Table */
	secondary_FirstBlock = SHT_ByteArrayToFirstBlock(block_data);

	secondary_FirstBlock->fileDesc = fd1;

	strcpy(secondary_FirstBlock->attrName, attrName);

	secondary_FirstBlock->attrLength = attrLength;

	secondary_FirstBlock->numBuckets = buckets;

	secondary_FirstBlock->SecondaryHashTable = malloc(buckets * sizeof(int));

	strcpy(secondary_FirstBlock->hashfileType, "secondary");

	strcpy(secondary_FirstBlock->fileName, fileName);

	/* set -1 all the buckets of the secondary hash table (invalid) */
	for(int i = 0; i < buckets; i++)
		secondary_FirstBlock->SecondaryHashTable[i] = Null;

	/* Write block to the disk */
	memcpy(block_data, secondary_FirstBlock, sizeof(SHT_FirstBlock));
	if(BF_WriteBlock(fd1, block_number) < 0) {
		BF_PrintError("Error writing block to the disk!\n");
		return -1;
	}

	shi.fileDesc = fd1;
	strcpy(shi.attrName, attrName);
	shi.attrLength = attrLength;
	shi.numBuckets = buckets;
	strcpy(shi.fileName, fileName);

	/* open ht block file */
	if( (fd2 = BF_OpenFile(fileName) ) < 0) {
		BF_PrintError("Error opening file!\n");
		return -1;
	}
	if(BF_ReadBlock(fd2, 0, &block_data) < 0) {
		BF_PrintError("Cannot read block!\n");
		return -1;
	}
	ht_first_block = HT_ByteArrayToFirstBlock(block_data);

	/* Sync step */
	/* for each bucket in the primary hash table:
	 * traverse its block list and for every block
	 * construct a SecondaryRecord based on every record of this specific block
	 * next, insert the SecondaryRecord to the secondary hash table
	 */
	for(int i = 0; i < ht_first_block->numBuckets; i++) {

		if( (next_block = ht_first_block->hashTable[i]) == Null) continue;
		while(next_block != Null) {
			if(BF_ReadBlock(fd2, next_block, &block_data) < 0) {
				BF_PrintError("Error getting block!");
				return -1;
			}
			block = HT_ByteArrayToBlock(block_data);
			for(int i = 0; i < block->records_Counter; i++) {
				srec.record = block->records[i];
				srec.blockId = next_block;
				SHT_SecondaryInsertEntry(shi, srec);
			}
			next_block = block->next_block;
			free(block);
			block = NULL;
		}
	}

	/* Close file */
	if(BF_CloseFile(fd1) < 0) {
		BF_PrintError("Error closing file!\n");
		return -1;
	}

	free(secondary_FirstBlock);
	free(ht_first_block);
	free(block);

	return 0;
}


SHT_info* SHT_OpenSecondaryIndex(char *sfileName) {

	int fd;
	void *block_data;
	SHT_info *rv;
	SHT_FirstBlock *secondary_FirstBlock;

	/* open block file */
	if( ( fd = BF_OpenFile(sfileName) ) < 0 ) {
		BF_PrintError("Error opening file!\n");
		return NULL;
	}
	/* bring to main memory and read the 1st block of the file */
	if( BF_ReadBlock(fd, 0, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return NULL;
	}
	/* 1st block needs to be reconstructed in order to read data from it */
	secondary_FirstBlock = SHT_ByteArrayToFirstBlock(block_data);
	/* if the block file opened is not an SHT file */
	if( strcmp(secondary_FirstBlock->hashfileType, "secondary") != 0) {
		printf("Block file is not of secondary hash type\n");
		return NULL;
	}

	 rv = malloc( sizeof(SHT_info) ); 

	/* copy to SHT_info the valuable information from the 1st block of the file */
	rv->fileDesc = secondary_FirstBlock->fileDesc;

	strcpy(rv->attrName, secondary_FirstBlock->attrName);

	rv->attrLength = secondary_FirstBlock->attrLength;

	rv->numBuckets = secondary_FirstBlock->numBuckets;

	strcpy(rv->fileName, secondary_FirstBlock->fileName);

	free(secondary_FirstBlock);

	return rv;
}


int SHT_CloseSecondaryIndex(SHT_info* header_info) {

	int fd;
	void *block_data;
	SHT_FirstBlock *Secfirst_block;

	fd = header_info->fileDesc;

	/* bring first block to main memory */
	if(BF_ReadBlock(fd, 0, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return -1;
	}
	Secfirst_block = SHT_ByteArrayToFirstBlock(block_data);

	/* write block back to the disk */
	memcpy(block_data, Secfirst_block, sizeof(SHT_FirstBlock));
	if(BF_WriteBlock(fd, 0) < 0) {
		BF_PrintError("Error writing block to the disk!\n");
		return -1;
	}

	if(BF_CloseFile(fd) < 0) {
		BF_PrintError("Error closing file!\n");
		return -1;
	}
	/* free the allocated memory for the header_info */
	free(header_info);
	/* free is not enough, free just marks the memory as unused, the struct data will be there until overwriting. */
	/* For safety, we set the pointer to NULL after free. */
	header_info = NULL;
	free(Secfirst_block);

	return 0;
}


int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record) {

	int blocknum, cur_block, hash_value;
	void *block_data;
	SHT_FirstBlock *sht_first_block;
	SecondaryBlock *sht_block;

	/* hashing based on the "name" of the record */
	if(CheckAttrName(header_info) == 1)
		hash_value = StringHash(record.record.name, header_info.numBuckets);
	else
		/* hashing based on the "surname" of the record */
		if(CheckAttrName(header_info) == 2)
			hash_value = StringHash(record.record.surname, header_info.numBuckets);
		/* hashing based on the "address" of the record */
		else
			hash_value = StringHash(record.record.address, header_info.numBuckets);

	/* now read the 1st block of the secondary hash file, in order to obtain the secondary hash table structure */
	if(BF_ReadBlock(header_info.fileDesc, 0, &block_data) < 0) {
		printf("Error getting block!\n");
		return -1;
	}
	/* reconstruct the 1st block */
	sht_first_block = SHT_ByteArrayToFirstBlock(block_data);
	if(sht_first_block->SecondaryHashTable[hash_value] == Null) {  	/* if SecondaryHashTable[hash_value] is not pointing to a block */
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
		sht_first_block->SecondaryHashTable[hash_value] = blocknum;
		/* write the 1st block to disk again */
		memcpy(block_data, sht_first_block, sizeof(SHT_FirstBlock));
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
		sht_block = SHT_ByteArrayToBlock(block_data);
		SHT_addRecordToBlock(&sht_block, record);
		sht_block->next_block = Null;		/* this block points to Null */
		/* now that the record is added, convert the block to byte array again */
		memcpy(block_data, sht_block, sizeof(SecondaryBlock));
		/* write the updated block to disk */
		if(BF_WriteBlock(header_info.fileDesc, blocknum) < 0) {
			printf("Error writing block!\n");
	 	       	return -1;
		}
		/* free the allocated memory */
		free(sht_first_block);
		free(sht_block);	
		
		return 0;
	}
	else {
		/* this bucket points to a block atm
		 * traverse the "list" of blocks until a non-full
		 * block is found->add the record there or 
		 * traverse until the end of the list is reached
		 * and allocate a new block->add the record there */


		if(BF_ReadBlock(header_info.fileDesc, sht_first_block->SecondaryHashTable[hash_value], &block_data) < 0) {
			BF_PrintError("Error getting block!\n");
			return -1;
		}
		/* reconstruct the block */
		sht_block = SHT_ByteArrayToBlock(block_data);
		if(SHT_addRecordToBlock(&sht_block, record) == -1) {	/* the block is full */
		
			cur_block = sht_first_block->SecondaryHashTable[hash_value];

			while(sht_block->next_block != Null) {	/* there is a next block */
				
				cur_block = sht_block->next_block;
				free(sht_block);
				sht_block = NULL;
				/* get the next block */
				if(BF_ReadBlock(header_info.fileDesc, cur_block, &block_data) < 0) {
					BF_PrintError("Error getting block!\n");
					return -1;
				}
				/* reconstruct the block */
				sht_block = SHT_ByteArrayToBlock(block_data);
				if(SHT_addRecordToBlock(&sht_block, record) == 0) {	/* there is space in the block that we are currently examining */
					/* add the record to this block */
					memcpy(block_data, sht_block, sizeof(SecondaryBlock));
					if(BF_WriteBlock(header_info.fileDesc, cur_block) < 0) {
						printf("Error writing block!\n");
	 			       	return -1;
					}	
					/* free the allocated memory */
					free(sht_first_block);
					free(sht_block);

					return 0;
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
			sht_block->next_block = blocknum;
			/* write the last block to disk, after its link was updated */
			memcpy(block_data, sht_block, sizeof(SecondaryBlock));
			if(BF_WriteBlock(header_info.fileDesc, cur_block) < 0) {
				printf("Error writing block!\n");
	 			return -1;
			}	
			free(sht_block);
			sht_block = NULL;
			/* read the newly allocated block */
			if(BF_ReadBlock(header_info.fileDesc, blocknum, &block_data) < 0) {
				printf("Error getting block!\n");
				return -1;
			}
			/* reconstruct the block */
			sht_block = SHT_ByteArrayToBlock(block_data);
			SHT_addRecordToBlock(&sht_block, record);
			sht_block->next_block = Null;		/* this block points to Null */
			/* now that the record is added, convert the block to byte array again */
			memcpy(block_data, sht_block, sizeof(SecondaryBlock));
			/* write the updated block to disk */
			if(BF_WriteBlock(header_info.fileDesc, blocknum) < 0) {
				printf("Error writing block to the disk!\n");
	 			return -1;
			}	
			free(sht_first_block);
			free(sht_block);

			return 0;
		}
		/* record was added at the first block of the bucket block list */	
		else {
			blocknum = sht_first_block->SecondaryHashTable[hash_value];
			memcpy(block_data, sht_block, sizeof(Block));
			if(BF_WriteBlock(header_info.fileDesc, blocknum) < 0) {
				printf("Error writing block to the disk!\n");
	 			return -1;
			}	
			/* free allocated memory */
			free(sht_first_block);
			free(sht_block);

			return 0;
		}
	}
}


int SHT_SecondaryGetAllEntries(SHT_info header_info_sht, HT_info header_info_ht, void *value) {

	char *val = (char *)value;
	int blocknum, block_counter = 1, flag = 0;
	void *block_data;
	SHT_FirstBlock *SecFirst_Block;
	SecondaryBlock *SecBlock;
	Block *PrimaryBlock;

	/* convert string to integer hash value */
	int hash_value = StringHash(val, header_info_sht.numBuckets);
	/* bring first block of the secondary hash file to the main memory */
	if(BF_ReadBlock(header_info_sht.fileDesc, 0, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return -1;
	}

	SecFirst_Block = SHT_ByteArrayToFirstBlock(block_data);
	/* locate the first block of the bucket linked list */
	blocknum = SecFirst_Block->SecondaryHashTable[hash_value];

	if(blocknum == Null)
		return -1;

	if(BF_ReadBlock(header_info_sht.fileDesc, blocknum, &block_data) < 0) {
		BF_PrintError("Error getting block!\n");
		return -1;
	}

	SecBlock = SHT_ByteArrayToBlock(block_data);

	switch ( CheckAttrName(header_info_sht) ) {
		/* if function returns 1, the attrName is "name", so we search for name-field */
		case 1:
			while(1) {
				block_counter++;
				/* all records of that block have the same "name",  so we check each record */
				for(int i = 0; i < SecBlock->records_Counter; i++) {
					if( strcmp(SecBlock->SecRecords[i].record.name, val) == 0) {
						/* flag = 1 means that there is record with the name = "val" which is field that we are searching */
						flag = 1;
						if(BF_ReadBlock(header_info_ht.fileDesc, SecBlock->SecRecords[i].blockId, &block_data) < 0) {
							BF_PrintError("Error getting block!\n");
							return -1;
						}
						/* go to primary hash file and print the whole record */
						PrimaryBlock = HT_ByteArrayToBlock(block_data);
						if(locateRecord(PrimaryBlock, SecBlock->SecRecords[i].record.id) == 0) continue;
						free(PrimaryBlock);
						PrimaryBlock = NULL;
					}
				}
				blocknum = SecBlock->next_block;
				free(SecBlock);
				SecBlock = NULL;
				/* blocknum == Null means that there are no other blocks to check */
				if(blocknum == Null)
					break;
				if(BF_ReadBlock(header_info_sht.fileDesc, blocknum, &block_data) < 0) {
					BF_PrintError("Error getting block!\n");
					return -1;
				}
				SecBlock = SHT_ByteArrayToBlock(block_data);
			}
			break;
		/* if function returns 2, the attrName is "surname", so we search for surname-field */
		case 2:
			while(1) {
				block_counter++;
				for(int i = 0; i < SecBlock->records_Counter; i++) {
					if( strcmp(SecBlock->SecRecords[i].record.surname, val) == 0) {
						flag = 1;
						if(BF_ReadBlock(header_info_ht.fileDesc, SecBlock->SecRecords[i].blockId, &block_data) < 0) {
							BF_PrintError("Error getting block!\n");
							return -1;
						}
						PrimaryBlock = HT_ByteArrayToBlock(block_data);
						if(locateRecord(PrimaryBlock, SecBlock->SecRecords[i].record.id) == 0) continue;
						free(PrimaryBlock);
						PrimaryBlock = NULL;
					}
				}
				blocknum = SecBlock->next_block;
				free(SecBlock);
				SecBlock = NULL;
				if(blocknum == Null)
					break;
				if(BF_ReadBlock(header_info_sht.fileDesc, blocknum, &block_data) < 0) {
					BF_PrintError("Error getting block!\n");
					return -1;
				}
				SecBlock = SHT_ByteArrayToBlock(block_data);
			}
			break;
		/* if function returns 3, the attrName is "address", so we search for address-field */
		case 3:
			while(1) {
				block_counter++;
				for(int i = 0; i < SecBlock->records_Counter; i++) {
					if( strcmp(SecBlock->SecRecords[i].record.address, val) == 0) {
						flag = 1;
						if(BF_ReadBlock(header_info_ht.fileDesc, SecBlock->SecRecords[i].blockId, &block_data) < 0) {
							BF_PrintError("Error getting block!\n");
							return -1;
						}
						PrimaryBlock = HT_ByteArrayToBlock(block_data);
						if(locateRecord(PrimaryBlock, SecBlock->SecRecords[i].record.id) == 0) continue;
						free(PrimaryBlock);
						PrimaryBlock = NULL;
					}
				}
				blocknum = SecBlock->next_block;
				free(SecBlock);
				SecBlock = NULL;
				if(blocknum == Null)
					break;
				if(BF_ReadBlock(header_info_sht.fileDesc, blocknum, &block_data) < 0) {
					BF_PrintError("Error getting block!\n");
					return -1;
				}
				SecBlock = SHT_ByteArrayToBlock(block_data);
			}
			break;
		default:
			break;
	}

	free(SecFirst_Block);

	/* if flag = 0 means that there are no records with "val" value */
	if(flag == 0)
		return -1;

	return block_counter;
}
