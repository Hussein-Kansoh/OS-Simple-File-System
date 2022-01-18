#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef SFS_API_H
#define SFS_API_H
// You can add more into this file.

void mksfs(int);

int sfs_getnextfilename(char*);

int sfs_getfilesize(const char*);

int sfs_fopen(char*);

int sfs_fclose(int);

int sfs_fwrite(int, const char*, int);

int sfs_fread(int, char*, int);

int sfs_fseek(int, int);

int sfs_remove(char*);

typedef struct{
    uint64_t magic;
    int blockSize;
    int fileSystemSize;
    int iNodeTableLength;
    int rootDirINodeIndex;
}superblock;

typedef struct {
    int indirectPointerBlock;
    int indirectPointers [1024 / sizeof(int)];
}IndirectPointer;

typedef struct{
    int mode;
    int linkCnt;
    int uid;
    int gid;
    int size;
    int dataPointers[12];
    IndirectPointer indirectBlock;
    int used;
}iNode;

typedef struct {
    int containsFile;
    int indexOfINode;
    char* fileName;
} directoryEntry;

typedef struct {
    int used;
    int iNodeIndex;
    int count;
    int rwPointer;
} fileDescriptor;
#endif
