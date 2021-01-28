/* filesys.c
 * 
 * provides interface to virtual disk
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"



diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
fatentry_t   currentDirIndex         = 0 ;

/* writedisk : writes virtual disk out to physical disk
 * 
 * in: file name of stored virtual disk
 */

void writedisk ( const char * filename )
{
   printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   write( dest, virtualDisk, sizeof(virtualDisk) ) ;
  
   fclose(dest) ;
}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
}


/* the basic interface to the virtual disk
 * this moves memory around
 */

void writeblock ( diskblock_t * block, int block_address )
{
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}

/* read and write FAT
 * 
 * please note: a FAT entry is a short, this is a 16-bit word, or 2 bytes
 *              our blocksize for the virtual disk is 1024, therefore
 *              we can store 512 FAT entries in one block
 * 
 *              how many disk blocks do we need to store the complete FAT:
 *              - our virtual disk has MAXBLOCKS blocks, which is currently 1024
 *                each block is 1024 bytes long
 *              - our FAT has MAXBLOCKS entries, which is currently 1024
 *                each FAT entry is a fatentry_t, which is currently 2 bytes
 *              - we need (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))) blocks to store the
 *                FAT
 *              - each block can hold (BLOCKSIZE / sizeof(fatentry_t)) fat entries
 */

/* implement format()
 */

void writeFAT( fatblock_t * fatblock, int block_address)
{
 memmove(virtualDisk[block_address].data, fatblock, BLOCKSIZE);
}

void writeDir( dirblock_t * dirblock, int block_address)
{
 memmove(virtualDisk[block_address].data, dirblock, BLOCKSIZE);
}
void format ( )
{

   diskblock_t block ;
   direntry_t  rootDir ;
   int         pos             = 0 ;
   int         fatentry        = 0 ;
   int         fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT ) ;

   /* prepare block 0 : fill it with '\0',
    * use strcpy() to copy some text to it for test purposes
	* write block 0 to virtual disk
	*/
	for (int i = 0 ; i< MAXBLOCKS; i++) block.data[i] = '\0';

		
	strcpy(block.data,"CS3026 Operating Systems Assessment");
	writeblock(&block,pos);
	/* prepare FAT table
	 * write FAT blocks to virtual disk
	 */
	for (int i = 0 ; i < MAXBLOCKS; i++)FAT[i] = UNUSED;

	// System Allocation
	FAT[0] = ENDOFCHAIN;
	// FAT
	FAT[1] = 2;
	FAT[2] = ENDOFCHAIN;
	//Dir
	FAT[3] = ENDOFCHAIN;
	
	updateFAT();
	 /* prepare root directory
	  * write root directory block to virtual disk
	  */
	for (int i = 0 ; i< MAXBLOCKS; i++) block.data[i] = '\0';
	dirblock_t rootBlock = block.dir;
	rootBlock.isdir = 1;
	rootBlock.nextEntry = 0;
	rootDirIndex = 3;
	currentDirIndex = 3;

	strcpy(rootBlock.entrylist->name, "root");
	rootBlock.entrylist[0].entrylength = 1;
	writeDir(&rootBlock,3);
}

void updateFAT(){
	fatblock_t fat;
	for(int i =0; i< BLOCKSIZE / sizeof(fatentry_t); i++) fat[i] = FAT[i];
	writeFAT(&fat,1);
	for(int i =0 ; i < BLOCKSIZE / sizeof(fatentry_t); i++) fat[i] = FAT[512 + i];
	writeFAT(&fat,2);	
}
void checkPath(char * filename){
	int start = 0; // start of the analyzed substring
	int len = 0; // size of the substring before the /

	// if it starts with a / , consider it as an absolute path ( go to the root dir and skip the first /)
	printf("Path splitted :");
	while(1){
	 while((filename[start + len] != '/') && (filename[start + len] != '\0')) len++;
	 printf("%.*s ", len, filename + start);

	 if (filename[start + len] == '\0') return 0;
	 start += len + 1; // skip over the '/' too
	}
}


MyFILE myfopen(char * filename,  char  mode){

	MyFILE fileManager;
	strcpy(fileManager.filename, filename);
	int firstBlock = findblock();
	if ( mode == 'w'){	
	 for (int i = 0 ; i < DIRENTRYCOUNT; i++){
		if ( strcmp(virtualDisk[rootDirIndex].dir.entrylist[i].name, filename) == 0){
			int index = virtualDisk[rootDirIndex].dir.entrylist[i].firstblock;
			int prev;
			while ( index != ENDOFCHAIN){
			 prev = index;
			 index = FAT[index];
			}
		   index = findblock();
		   FAT[prev] = index;
		   FAT[index] = ENDOFCHAIN;
		   fileManager.blockno = FAT[prev];
		   fileManager.mode[0] = mode;
	           return fileManager;
		}
	 }
	// find a entrylist with .isdir '\0'
	int rootdirEntry = findRootDirEntry();
	
	
	// update Fat entries in hexdump
	updateFAT();

	// set dir attributes of file
	direntry_t dir;
	dir.entrylength = 1;
	dir.unused = 0;
	dir.isdir = 0;
	dir.filelength = sizeof(filename);
	dir.firstblock = firstBlock;

	strcpy(dir.name,filename);
	// write the dir in root
	virtualDisk[rootDirIndex].dir.entrylist[rootdirEntry] = dir;

	fileManager.mode[0] = mode;
	fileManager.blockno = firstBlock;
	
	return fileManager;
	}
	else if ( mode == 'r'){
	strcpy(fileManager.filename,filename);
	for ( int i = 0 ; i < DIRENTRYCOUNT; i++){
		if (strcmp(virtualDisk[rootDirIndex].dir.entrylist[i].name,filename) == 0)
		{
		 	fileManager.mode[0] = mode;
			fileManager.blockno = virtualDisk[rootDirIndex].dir.entrylist[i].firstblock;
			return fileManager;
		}
	}
	}
}

void myfputc(char * data, MyFILE  fileManager, long int size)
{	
	if (fileManager.mode[0] == 'w'){
	// create entries in FAT
	long int clustersCount = size / 1024;
	if (size % 1024 > 0) clustersCount += 1;
	
	int index = fileManager.blockno;
	int prev = fileManager.blockno;
	
	for (int i = 0; i < clustersCount ; i ++){
		FAT[prev] = index;
		FAT[index] = ENDOFCHAIN;
		prev = index;
		index = findblock();
	}
	for ( int i = 0 ; i < DIRENTRYCOUNT; i++){
		if (strcmp(virtualDisk[rootDirIndex].dir.entrylist[i].name, fileManager.filename) == 0)
		{	
			virtualDisk[rootDirIndex].dir.entrylist[i].filelength += size;
			break;
		}
	}
	updateFAT();
	// WRITE DATA TO FILE MANAGER BUFFER
	// LOAD INTO VIRTUALDISK[FILEMANAGER.BLOCKNO]
	// REPEAT UNTIL ENDOFCHAIN
	index = fileManager.blockno;
	for (int i = 0 ; i < clustersCount ; i++){
		// load each segment of data into buffer
		// if more than one block is required, override the previous values in the buffer
		int z = 0;
		for ( int j = i * BLOCKSIZE; j < (i * BLOCKSIZE) + BLOCKSIZE; j++){
			if (data[j] == NULL){
			 for ( int k = j; k < BLOCKSIZE; k++){
			 fileManager.buffer.data[j + k]= '\0';
			 }
			break;
			}
			fileManager.buffer.data[z] = data[j];	
			z+= 1;
		}
		// write the block and go to next index
		writeblock(&fileManager.buffer, index);
		if( FAT[index] != 0){
		index = FAT[index];
		}
	}
	}
	else{
	printf("Stderr: cannot write to a file opened for read");
	}
}
// read file and write to disk
void myfgetc(MyFILE fileManager){
	int index = fileManager.blockno;
	while (index != ENDOFCHAIN){
		for ( int i = 0 ; i < BLOCKSIZE; i++){

			if(virtualDisk[index].data[i] == NULL) break;

			fileManager.buffer.data[i] = virtualDisk[index].data[i];

			printf("%c ", fileManager.buffer.data[i]);
		}
		FILE * fp = fopen("testfileC3_C1_copy.txt", "a");

		fputs(fileManager.buffer.data,fp);

		fclose(fp);

		index = FAT[index];
	}
	
}

// reset MyFILE attributes
void myfclose(MyFILE  fileManager){
	for ( int i = 0; i < BLOCKSIZE; i++)
	{
		fileManager.buffer.data[i] = '\0';
	}
	fileManager.blockno = UNUSED;
	fileManager.mode[0] = ' ';
}

void mymkdir(char * path){

diskblock_t newblock;
for ( int i = 0; i< BLOCKSIZE; i++) newblock.data[i]= '\0';
int writeToDir;

// if relative start from root
if ( path[0] == '/'){
 writeToDir = currentDirIndex;
}
// absolute, find a new allocation
else{
writeToDir = findblock();
}
char* token = strtok(path, "/");
while(token != NULL) {
 if (strcmp(token, "root") != 0){

	int fatIndex = findblock();

	int dirIndex = findDirEntry(writeToDir);

	FAT[fatIndex] = ENDOFCHAIN;

	updateFAT();
	virtualDisk[writeToDir].dir.entrylist[dirIndex].entrylength = 1;
	virtualDisk[writeToDir].dir.entrylist[dirIndex].isdir = 1;
	virtualDisk[writeToDir].dir.entrylist[dirIndex].firstblock = fatIndex;
	strcpy(virtualDisk[writeToDir].dir.entrylist[dirIndex].name, token);


	dirblock_t newDir = newblock.dir;

	newDir.isdir = 1;
	newDir.nextEntry = 0;

	strcpy(newDir.entrylist->name, token);
	newDir.entrylist[0].entrylength = 1;

	writeDir(&newDir, fatIndex);
	writeToDir = fatIndex;	
 }
 token = strtok(NULL,"/");

}

}
// create array of strings
char ** allocateStrings(int numberOfStrings, int stringsLength){
	char ** retVal = (char **) malloc (sizeof(char*) * numberOfStrings);
	for(int i = 0; i < numberOfStrings; i++){
		retVal[i]= (char*) malloc(sizeof(char) * (stringsLength +1) );
	}
	return retVal;
}

char ** mylistDir(char * path){

 char* token = strtok(path,"/");
 char ** strings = allocateStrings(DIRENTRYCOUNT,MAXNAME);

 int currIndex = rootDirIndex;
  while(token != NULL){

    if ( strcmp(token,"root")== 0){

	for(int i = 1; i < DIRENTRYCOUNT; i++){
	
	strcpy(strings[i],virtualDisk[rootDirIndex].dir.entrylist[i].name);
	}
     }
    else{

	//find dir in entrylist of currentDir
	for( int i = 0; i< DIRENTRYCOUNT; i++){
	strcpy(strings[i]," ");
	  if (strcmp(virtualDisk[currIndex].dir.entrylist[i].name, token)== 0){
		
		currIndex = virtualDisk[currIndex].dir.entrylist[i].firstblock;

		strcpy(strings[i],virtualDisk[currIndex].dir.entrylist[i].name);
		
	  }
	}
     }
  token = strtok(NULL,"/");
  }

  return strings;

}


int findRootDirEntry(){
	for (int i = 0; i < DIRENTRYCOUNT; i++){
		if(virtualDisk[rootDirIndex].dir.entrylist[i].entrylength != 1)
			return i;
	}
}

int findDirEntry(int dirBlockIndex){
	for (int i = 0; i < DIRENTRYCOUNT; i++){
		if(virtualDisk[dirBlockIndex].dir.entrylist[i].entrylength != 1)
			return i;
	}
}
int findblock(){
for(int i =0; i< MAXBLOCKS;i++){
		if (FAT[i] == UNUSED){
			return i;
		}
	}
}
/* use this for testing
 */

void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s", blockIndex, virtualDisk[blockIndex].data ) ;
}
