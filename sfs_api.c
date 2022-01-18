#include "sfs_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "disk_emu.c"
#define FILE_NAME "sfs_disk.disk"
#define BLOCK_SIZE 1024 //size of a single block in bytes
#define NUM_BLOCKS 1024 //number of blocks in the disk
#define NUM_INODES 100 //number of inodes in the inode table
#define MIN(x, y) (((x) < (y)) ? (x) : (y)) //returns the minimum of 2 values



superblock superBlock; //this represents the superblock is the first block in our disk
iNode INodeTable[NUM_INODES]; //this represents the Inode table in the form of an array of inodes (starts at the second block of the disk right after the super block)
char bitMap[NUM_BLOCKS]; //this represents bit map as a character array which indicated which blocks in the disk are free to use or not (last block on the )
directoryEntry directoryTable[NUM_INODES-1]; //this is the directory table which stores all the file names (-1 because we don't include the root directory)
fileDescriptor fileDescriptorTable[NUM_INODES-1]; //file descriptor table (-1 because we don't include the root directory)
int curPosition = 0;

//int directoryStart = superBlock.iNodeTableLength;

void initSuperblock(){
    superBlock.magic = 0xACBD0005; //not sure what this means
    superBlock.blockSize = BLOCK_SIZE; //size of an individual block since the super block is only one block
    superBlock.fileSystemSize = NUM_BLOCKS * BLOCK_SIZE; //the file system size is equal to the total number of blocks multiplied by the size of one individual block
    superBlock.iNodeTableLength = NUM_INODES*sizeof(iNode)/BLOCK_SIZE; //how many blocks the INodeTable takes up
    superBlock.rootDirINodeIndex = 0; //the root directory is the first element of the INodeTable
}

void initINodeTable(){
    for (int i=0; i<NUM_INODES; i++){ 
        INodeTable[i].used = 0; //initially no inodes are used
        for (int j=0; j<12; j++){
           INodeTable[i].dataPointers[j] = -1; //initially no files
        }
    }
    INodeTable[0].used=1;
}

void initFileDescriptorTable(){
    for (int i=0; i<NUM_INODES-1; i++){
        fileDescriptorTable[i].used =  0; //initially no files 
        fileDescriptorTable[i].count = 0;
    }
    
}

void initDirectory(){
    for (int i=0; i<NUM_INODES-1; i++){
        directoryTable[i].containsFile =  0; //initially no files
        directoryTable[i].fileName = " ";
    }
    INodeTable[0].dataPointers[0]=superBlock.iNodeTableLength+1;
    INodeTable[0].size=1; //maybe remove this later
}

void initbitMap(){
    for (int i=0; i<NUM_BLOCKS; i++){
        bitMap[i]=1; //initially no blocks are used
    }
}

void initIndirectBlock(int indirectBlock[]){
    for (int i=0; i<BLOCK_SIZE / sizeof(int); i++){
        indirectBlock[i]=-1; //initially no indirect block
    }
}

void mksfs(int fresh){
    if (fresh) {
        //initialize fresh disk
         init_fresh_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS);
         //initialize super block
         initSuperblock();
         //initialize iNode Table
         initINodeTable();
        //initialize the root directory as the first data block in the data blocks section
        initDirectory();
        //initialize bit map
        initFileDescriptorTable();
        //initialize File Descriptor Table
         initbitMap();
         //write the superblock to the disk
         write_blocks(0, 1, &superBlock);
         //indicate first block unavailable on bitmap (used by superblock)
         bitMap[0]=0;
         //write inodetable to the disk
         write_blocks(1, superBlock.iNodeTableLength, &INodeTable);
         //indicate all blocks taken by iNode as unavailable on bitmap
         for(int i = 1; i <= superBlock.iNodeTableLength; i++){
             bitMap[i]=0;
         }
         //write root directory to disk
         write_blocks(superBlock.iNodeTableLength+1, 1, &directoryTable);
         //root directory on bitmap
         bitMap[superBlock.iNodeTableLength+1]=0;
         //write bitmap to the disk
         write_blocks(NUM_BLOCKS - 1, 1, bitMap);
         //indicate last block is taken for bitmap on bitmap
         bitMap[NUM_BLOCKS - 1] = 0;
    } else{
        for (int i=0; i<NUM_INODES-1; i++){
            fileDescriptorTable[i].count = 0; //haven't accessed file descriptor yet
        }
        init_disk(FILE_NAME, BLOCK_SIZE, NUM_BLOCKS);
        //store super block
        read_blocks(0, 1, &superBlock);
        //store iNode Table
        read_blocks(1, superBlock.iNodeTableLength, &INodeTable);
        //store root directory
        read_blocks(superBlock.iNodeTableLength+1, 1, &directoryTable);
        //store bitmap
        read_blocks(NUM_BLOCKS - 1, 1, bitMap);
    }
}

int sfs_getnextfilename(char *fileName){
        for(int i=curPosition; i<NUM_INODES-1; i++){
            if (directoryTable[i].containsFile){ //check non empty file
                strcpy(fileName, directoryTable[i].fileName); //vopy the file name
                curPosition++; //increment which file were pointing to in the directory
                return 1;
            }
            curPosition++; //increment which file were pointing to in the directory
        }
        return 0;
}

int sfs_getfilesize(const char *fileName){
    for(int i=0; i<NUM_INODES-1; i++){
            if (strcmp(directoryTable[i].fileName, fileName)==0){ //check its the correct name
                return INodeTable[directoryTable[i].indexOfINode].size; //return the size of the file
            }
    }
        return -1;
}

//find the index of the INode which points to the file with the the same name
int INodeIndex(char *fileName){
    for(int i=0; i<NUM_INODES-1; i++){
        if (strcmp(directoryTable[i].fileName, fileName)==0 && directoryTable[i].containsFile){
            return directoryTable[i].indexOfINode;
        }
    }
    return -1;
}

//find the index of the directory entry which contains file
int DirectoryIndex(char *fileName){
    for(int i=0; i<NUM_INODES-1; i++){
        if (strcmp(directoryTable[i].fileName, fileName)==0 && directoryTable[i].containsFile){
            return i;
        }
    }
    return -1;
}

//find the index of the file descriptor which contains file
int FdNumFromINode(int index){
    for(int i=0; i<NUM_INODES-1; i++){
        if (fileDescriptorTable[i].iNodeIndex==index){
            return i;
        }
    }
    return -1;
}

//finds an empty spot in the inode table
int findEmptySpotITble(){
    for(int i=0; i<NUM_INODES; i++){
        if (INodeTable[i].used==0){
            return i;
        }
    }
    return -1;
}

//finds an empty spot in the file descriptor table
int findEmptySpotFd(){
    for(int i=0; i<NUM_INODES-1; i++){
        if (fileDescriptorTable[i].used==0){
            return i;
        }
    }
    return -1;
}

//find an empty spot in the directory
int findEmptySpotDir(){
     for(int i=0; i<NUM_INODES-1; i++){
        if (directoryTable[i].containsFile==0){
            return i;
        }
     }
     return -1;
}

//find empty spot in the bitmap
int findEmptySpotBitmap(){
     for(int i=0; i<NUM_BLOCKS; i++){
        if (bitMap[i]==1){
            return i;
        }
     }
     return -1;
}

//open file
int sfs_fopen(char *fileName){
    if (strlen(fileName)>32){
        return -1; 
    }
    int iNodeIndex = INodeIndex(fileName);
    if (iNodeIndex<0){ //file does not exist
        int iNodeIdx = findEmptySpotITble();
        if (iNodeIdx<0){ //iNode table full
            return -1;
        }
        int fileDescriptorNumber = findEmptySpotFd();
        if (fileDescriptorNumber<0){ //file descriptor table full
            return -1;
        }
        int DirIdx = findEmptySpotDir();
        if (DirIdx<0){
            return -1; //directory full
        }
        INodeTable[iNodeIdx].used = 1;
        INodeTable[iNodeIdx].size = 0;
        fileDescriptorTable[fileDescriptorNumber].used = 1;
        fileDescriptorTable[fileDescriptorNumber].iNodeIndex = iNodeIdx;
        fileDescriptorTable[fileDescriptorNumber].rwPointer=0;
        directoryTable[DirIdx].containsFile = 1;
        directoryTable[DirIdx].fileName = fileName;
        directoryTable[DirIdx].indexOfINode = iNodeIdx;
        write_blocks(1, superBlock.iNodeTableLength, &INodeTable);
        write_blocks(superBlock.iNodeTableLength+1, 1, &directoryTable);
        return fileDescriptorNumber;
    } else { //file already exists
        int fileDescriptorNumber = FdNumFromINode(iNodeIndex);
        if (fileDescriptorNumber>-1){ //check if file is already open
            return fileDescriptorNumber;
        } else{  
            int emptySpotInFileDescriptor =-1;
            for (int i=0; i<NUM_INODES-1; i++){
                if (fileDescriptorTable[i].used==0){
                    emptySpotInFileDescriptor = i;
                    break;
                }
            }
            if (emptySpotInFileDescriptor<0){ //no spot available on the file descriptor table
                return -1; 
            } else{
                fileDescriptorTable[emptySpotInFileDescriptor].used = 1; 
                fileDescriptorTable[emptySpotInFileDescriptor].iNodeIndex = iNodeIndex; 
                fileDescriptorTable[emptySpotInFileDescriptor].rwPointer = INodeTable[iNodeIndex].size; //set the file pointer to the end of the file
                return emptySpotInFileDescriptor;
            }
        }
    }
}

int sfs_fclose(int fileDescriptorNumber){
    if (fileDescriptorTable[fileDescriptorNumber].iNodeIndex<0){ //no associated inode
        return -1;
    } else{
        fileDescriptorTable[fileDescriptorNumber].used=0; //slot in fdt no longer used
        fileDescriptorTable[fileDescriptorNumber].rwPointer=0; //remove pointer
        fileDescriptorTable[fileDescriptorNumber].iNodeIndex=-1; //no associated inode
        return 0;
    }
}


int sfs_fwrite(int fileDescriptorNumber, const char* buf, int length){
    if (fileDescriptorNumber < 0) {
        return -1; 
    }
    int TotalAmountToWrite = length; //how much to write to the file
    fileDescriptor* fDescriptor = &fileDescriptorTable[fileDescriptorNumber]; //get the file from the file descriptor
    iNode* INode = &INodeTable[fDescriptor->iNodeIndex]; //which I-Node the file belongs to 
    char *buff = malloc(BLOCK_SIZE); //buffer to contain the string to be written to each individual block
    while(length>0){ //keep going until there is nothing left to write
        memset(buff, 0, BLOCK_SIZE); //empty block buffer to prepare it for new block
        int numberOfBytesToWrite; //will be how many bytes should be written to each block
        int curBlock = fDescriptor->rwPointer/BLOCK_SIZE; //which block in the inode are we at
        int byteOfBlock = fDescriptor->rwPointer%BLOCK_SIZE; //which byte of that block are we at
        int bytesRemainingInBlock = BLOCK_SIZE - byteOfBlock; //how many bytes do we have left in that block
        if (length > bytesRemainingInBlock){
            numberOfBytesToWrite = bytesRemainingInBlock; //number of bytes to write is greater than the number of bytes left in this block so use up the entire block
        } else{
            numberOfBytesToWrite = length; //number of bytes to write is less than the number of bytes left in this block so write only required amount
        }
        if (curBlock<12){ //must be direct pointer
            if (INode->dataPointers[curBlock] > -1){ //the direct pointer inode is pointing to a non empty block
                read_blocks(INode->dataPointers[curBlock], 1, buff); //Read the contents from the disk to buff
                memcpy(buff + byteOfBlock, buf, numberOfBytesToWrite); //copy the string from the buffer to the buff which will contain only the string for one block  
                write_blocks(INode->dataPointers[curBlock], 1, buff); //write the file info to the block
                length-=numberOfBytesToWrite; //we've already written to that block so the total amount we have to write is less
                fDescriptor->rwPointer+=numberOfBytesToWrite; //shift the read/write pointer to after the amount we've just written
                if (fDescriptor->rwPointer > INode->size){
                        INode->size = fDescriptor->rwPointer; //if we write more than what was already in the file we have to indicate the increase in file size
                }    
                buf+=numberOfBytesToWrite; //shift the pointer
            } else{ //the inode is pointing to an empty block
                if (findEmptySpotBitmap()==-1){ //no empty spots
                    return -1;  
                } else{
                    int curBlockIndex = findEmptySpotBitmap(); //find a block that's  not used
                    if (curBlockIndex<0){
                        return -1; //no empty block
                    }
                    INode->dataPointers[curBlock]=curBlockIndex; //make the inode's direct pointer point to that block
                    INode->used=1; //indicate that the inode is being used
                    bitMap[curBlockIndex]=0; //indicate that block 
                    memcpy(buff + byteOfBlock, buf, numberOfBytesToWrite); //copy the string from the buffer to the buff which will contain only the string for one block  
                    write_blocks(curBlockIndex, 1, buff); //write the file info to the block of the disk
                    length-=numberOfBytesToWrite; //we've already written to that block so the total amount we have to write is less
                    fDescriptor->rwPointer+=numberOfBytesToWrite; //shift the read/write pointer to after the amount we've just written
                    if (fDescriptor->rwPointer > INode->size){
                        INode->size = fDescriptor->rwPointer; //if we write more than what was already in the file we have to indicate the increase in file size
                    }
                    buf+=numberOfBytesToWrite; //shift the pointer
                }
            }
        } else{ //must be indirect pointer
            int blockOfIndirect; //the block which the indirect pointer will take
            int blockPointedTobyCurIndirect;
             if (fDescriptor->count==0){ //first time using indirect block
                 INode->indirectBlock.indirectPointerBlock=-1;
             }
             fDescriptor->count++; //indicate file descriptor has been used
            if (INode->indirectBlock.indirectPointerBlock>-1){ //indirect block has been used before
                read_blocks(INode->indirectBlock.indirectPointerBlock, 1, &INode->indirectBlock.indirectPointers); //store the content already stored from the disk in the indirect block
            } else{  //indirect block has not been used before
                if (findEmptySpotBitmap()==-1){
                    return -1; // no empty spot for the indirect pointer
                } else{
                    blockOfIndirect = findEmptySpotBitmap(); //this is the block for the indirect pointer
                }
                INode->indirectBlock.indirectPointerBlock = blockOfIndirect; //set it in the inode
                initIndirectBlock(INode->indirectBlock.indirectPointers); //initialize the pointers to all unused since this is the first time
                //fDescriptor->count++; 
                bitMap[blockOfIndirect]=0; //indicate its used in the bitmap
            }
            int indirectNumber = curBlock-12; //index of indirect pointer
            blockPointedTobyCurIndirect = INode->indirectBlock.indirectPointers[indirectNumber]; //block belonging to that specific indirect pointer
            if (blockPointedTobyCurIndirect>0){ //already used
                read_blocks(blockPointedTobyCurIndirect, 1, buff);
                memcpy(buff + byteOfBlock, buf, numberOfBytesToWrite); //copy the string from the buffer to the buff which will contain only the string for one block  
                write_blocks(blockPointedTobyCurIndirect, 1, buff); //write the file info to the block
                length-=numberOfBytesToWrite; //we've already written to that block so the total amount we have to write is less
                fDescriptor->rwPointer+=numberOfBytesToWrite; //shift the read/write pointer to after the amount we've just written
                if(fDescriptor->rwPointer > INode->size){
                   INode->size = fDescriptor->rwPointer;
                }
                buf+=numberOfBytesToWrite; //shift the pointer 
            } else{
                if (findEmptySpotBitmap()==-1){ //no empty spots
                    return -1;  
                } else{
                    blockPointedTobyCurIndirect = findEmptySpotBitmap(); //find a block that's  not used
                    INode->indirectBlock.indirectPointers[indirectNumber] = blockPointedTobyCurIndirect; //set that block as pointed to by this indirect pointer 
                    INode->used=1; //indicate that the inode is being used
                    bitMap[blockPointedTobyCurIndirect]=0; //indicate that block 
                    write_blocks(INode->indirectBlock.indirectPointerBlock, 1, &INode->indirectBlock.indirectPointers); //write indirect block to the disk
                    memcpy(buff + byteOfBlock, buf, numberOfBytesToWrite); //copy the string from the buffer to the buff which will contain only the string for one block  
                    write_blocks(blockPointedTobyCurIndirect, 1, buff); //write the file info to the block
                    length-=numberOfBytesToWrite; //we've already written to that block so the total amount we have to write is less
                    fDescriptor->rwPointer+=numberOfBytesToWrite; //shift the read/write pointer to after the amount we've just written
                    if(fDescriptor->rwPointer > INode->size){
                        INode->size = fDescriptor->rwPointer;
                    }
                    buf+=numberOfBytesToWrite; //shift the pointer
                }
        }

    }
    }
    write_blocks(NUM_BLOCKS - 1, 1, &bitMap); //update the bitmap on the disk
    write_blocks(1, superBlock.iNodeTableLength, &INodeTable); //update the inode to the disk
    return MIN(TotalAmountToWrite, INode->size);
}

int sfs_fread(int fileDescriptorNumber, char* buf, int length){
    if (fileDescriptorNumber<0){
        return -1;
    }
    if (fileDescriptorTable[fileDescriptorNumber].used == 0) {
        return 0; //if file is not used
    }
    int TotalAmountToRead = 0; 
    int numberOfBytesToRead; //will be how many bytes should be read from each block
    fileDescriptor *fDescriptor = &fileDescriptorTable[fileDescriptorNumber]; //get the file from the file descriptor
    iNode *INode = &INodeTable[fDescriptor->iNodeIndex]; //which I-Node the file belongs to 
    if (INode->size==0){
        return 0;
    }
    if (INode->size<length){
        length = INode->size;
    }
    char *buff = malloc(BLOCK_SIZE); //buffer to contain the string to be read from each individual block
    while(length>0){
        memset(buff, 0, BLOCK_SIZE); //empty block buffer to prepare it for new block
        int curBlock = fDescriptor->rwPointer/BLOCK_SIZE; //which block in the inode are we at
        int byteOfBlock = fDescriptor->rwPointer%BLOCK_SIZE; //which byte of that block are we at
        int bytesRemainingInBlock = BLOCK_SIZE - byteOfBlock; //how many bytes do we have left in that block
        if (curBlock<12){ //must be direct pointer
            int curBlockIndex = INode->dataPointers[curBlock];
            if (length > bytesRemainingInBlock){
                    numberOfBytesToRead = bytesRemainingInBlock; //number of bytes to read is greater than the number of bytes left in this block so use up the entire block
                } else{
                    numberOfBytesToRead = length; //number of bytes to read is less than the number of bytes left in this block so write only required amount
                } 
            read_blocks(curBlockIndex, 1, buff); //put the string in the current block into the buffer
            memcpy(buf, buff+byteOfBlock, numberOfBytesToRead); //copy the string from the buffer to the buff which will contain only the string for one block  
            length-=numberOfBytesToRead; //we've already read that block so the total amount we have to write is less
            fDescriptor->rwPointer+=numberOfBytesToRead; //shift the read/write pointer to after the amount we've just read
            buf+=numberOfBytesToRead; //shift the pointer
            TotalAmountToRead+=numberOfBytesToRead;
        } else{ //indirect
                if (length > bytesRemainingInBlock){
                    numberOfBytesToRead = bytesRemainingInBlock; //number of bytes to read is greater than the number of bytes left in this block so use up the entire block
                } else{
                    numberOfBytesToRead = length; //number of bytes to read is less than the number of bytes left in this block so write only required amount
                }
            int curBlockIndex = INode->indirectBlock.indirectPointers[curBlock - 12];
            read_blocks(curBlockIndex, 1, buff); //put the string in the current block into the buffer
            memcpy(buf, buff+byteOfBlock, numberOfBytesToRead); //copy the string from the buffer to the buff which will contain only the string for one block  
            length-=numberOfBytesToRead; //we've already read that block so the total amount we have to write is less
            fDescriptor->rwPointer+=numberOfBytesToRead; //shift the read/write pointer to after the amount we've just written
            buf+=numberOfBytesToRead; //shift the pointer
            TotalAmountToRead+=numberOfBytesToRead;
        }
    }
    return MIN(TotalAmountToRead, INode->size);
}

int sfs_fseek(int fileDescriptorNumber, int location){
    if (fileDescriptorNumber<0 || location<0 || location>(12*BLOCK_SIZE + BLOCK_SIZE/4*BLOCK_SIZE) 
    || fileDescriptorTable[fileDescriptorNumber].used==0){ //check we are within bounds
        return -1;
    }
    fileDescriptorTable[fileDescriptorNumber].rwPointer = location; //set the pointer to the appropriate location
    return 0;
}

int sfs_remove(char* fileName){
    int flag = 0; //indicates if the file used the indirect pointers
    int iNodeIndex = INodeIndex(fileName); //find inode of file
    if (iNodeIndex<0){
        return -1;
    }
    int directoryIndex = DirectoryIndex(fileName); //get the entry of the directory which contains the file
    int fileDescriptorNumber = directoryIndex; //the file descriptor and directory are parallel
    fileDescriptor* fDescriptor = &fileDescriptorTable[fileDescriptorNumber]; //get the file from the file descriptor
    iNode* INode = &INodeTable[fDescriptor->iNodeIndex]; //which I-Node the file belongs to 
    if (INode->size > 12288){
        flag = 1;  //check if the file used the indirect pointers
    }
    for (int i=0; i<12; i++){ 
            char* freeSpace = malloc(BLOCK_SIZE); 
            memset(freeSpace, 0, BLOCK_SIZE); 
            write_blocks(INodeTable[iNodeIndex].dataPointers[i], 1, freeSpace); 
            INodeTable[iNodeIndex].dataPointers[i]=-1; //dissacoiate indoe
            bitMap[INodeTable[iNodeIndex].dataPointers[i]] = 1; //indicate free on bitmap
             free(freeSpace);
    }
    if (flag){
        if (INode->indirectBlock.indirectPointerBlock >= 0) {
            char* freeSpace = malloc(BLOCK_SIZE); 
            memset(freeSpace, 0, BLOCK_SIZE);
            bitMap[INode->indirectBlock.indirectPointerBlock] = 1; //indicate free on bitmap
            for (int i=0; i<BLOCK_SIZE / sizeof(int); i++){
                bitMap[INode->indirectBlock.indirectPointers[i]] = 1; //indicate all the blocks taken up by pointers as free on bitmap
            
            write_blocks(INode->indirectBlock.indirectPointers[i], 1, freeSpace);
            initIndirectBlock(INode->indirectBlock.indirectPointers);
            INode->indirectBlock.indirectPointerBlock=-1;
            free(freeSpace);
            }
            write_blocks(INode->indirectBlock.indirectPointerBlock, 1, &INode->indirectBlock.indirectPointers);
        }
        char* freeSpace = malloc(BLOCK_SIZE);
        memset(freeSpace, 0, BLOCK_SIZE);
        write_blocks(INode->indirectBlock.indirectPointerBlock, 1, freeSpace);
    }
    INode->size=0; //file size now empty
    INode->used=0; //file is not used
    fDescriptor->used=0; 
    fDescriptor->rwPointer=0; //reset read/write pointer 
    fDescriptor->count--; //one less instance
    fDescriptor->iNodeIndex = -1; //no associated i node
    directoryTable[directoryIndex].containsFile = 0; //directory slot no longer contains file
    directoryTable[directoryIndex].fileName = " "; //empty file
    directoryTable[directoryIndex].indexOfINode = -1; //no associated inode
    INodeTable[iNodeIndex].used = 0; //inode not used
    write_blocks(1, superBlock.iNodeTableLength, &INodeTable);
    write_blocks(superBlock.iNodeTableLength+1, 1, &directoryTable);
    write_blocks(NUM_BLOCKS - 1, 1, bitMap); //write everything to the disk
    return 0;
}