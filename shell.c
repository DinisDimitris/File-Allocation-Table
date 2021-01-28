#include "filesys.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void main(){
format();


// data to read
int dataSize = 4;
char chars[26] = "ABCDEFGHIJKLMNOPQRSTUVWXWZ";
char data [dataSize * BLOCKSIZE];

// test open again
char data2[] = "Shrek and his friends";

// path for mkdir ( seg fault if strtok receives a string straight from parameter function )
char path[] = "/root/subdir1/subdir2";

char path2[]= "/root";

// load data
int z = 0;
for(int i = 0; i < dataSize * BLOCKSIZE; i++){
 if (z == 26) z = 0;
 data[i] = chars[z];
 z+= 1;
}


mymkdir(path);

//open file
MyFILE srcfile = myfopen("testfile.txt",'w');
myfputc(data,srcfile,sizeof(data));
myfclose(srcfile);

// list file content of root
//  i use another initializer for path here because first path is modified by strtok
char ** content = mylistDir(path2);
for ( int i = 1; i < DIRENTRYCOUNT;i++) printf("Content: %s \n", content[i]);

// open again for write
srcfile = myfopen("testfile.txt",'w');
myfputc(data2,srcfile,sizeof(data2));
myfclose(srcfile);

// split path without strtok
checkPath("/root/subdir1/subdir2");


// read and copy output on real hard disk
MyFILE  readfile = myfopen("testfile.txt",'r');
myfgetc(readfile);
myfclose(readfile);

writedisk("virtualdiskB3_B1");





}
