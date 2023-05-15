// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

//----------------------------------------------------------------------
// FileHeader::~FileHeader
// 	delete the singleIndirectpointer table if it allocated
//----------------------------------------------------------------------

FileHeader::~FileHeader() {
    if (table != nullptr) {
        delete[] table;
        table = nullptr;
    }
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    int totalNumOfSectors = divRoundUp(fileSize, SectorSize);
    numDirectSectors = min(MAX_NUM_OF_DIRECT_POINTER, totalNumOfSectors);
    if (freeMap->NumClear() < numDirectSectors){
	    return FALSE;		// not enough space for direct sector
    }
    for (int i = 0; i < numDirectSectors; i++) {
        dataSectors[i] = freeMap->FindAndSet();
        // since we checked that there was enough free space,
        // we expect this to succeed
        ASSERT(dataSectors[i] >= 0);
    }
    totalNumOfSectors -= numDirectSectors;
    // need indirect pointer
    if (totalNumOfSectors > 0) {
        numIndirectPointerSectors = divRoundUp(totalNumOfSectors, MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER); // Ex. 32 / 31 = 2
        if(!AllocateIndirect(freeMap, totalNumOfSectors)) return FALSE;
    }
    else {
        numIndirectPointerSectors = 0;
    }
    return TRUE;
}

bool 
FileHeader::AllocateIndirect(PersistentBitmap *freeMap, int totalNumOfSectors)
{
    // ensure the pointer is null
    if (table != nullptr) {
        delete[] table;
        table = nullptr;
    }

    table = new SingleIndirectPointer[numIndirectPointerSectors];

    if (freeMap->NumClear() < numIndirectPointerSectors){
	    return FALSE;
    }
    for (int i = 0; i < numIndirectPointerSectors; i++) {
        singleIndirectSectors[i] = freeMap->FindAndSet();
        // since we checked that there was enough free space,
        // we expect this to succeed
        ASSERT(singleIndirectSectors[i] >= 0);
    }

    for (int i = 0; i < numIndirectPointerSectors; i++) {
        if (totalNumOfSectors > 0) {
            int allocateSectors = min(MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER, totalNumOfSectors);
            if (!table[i].Allocate(freeMap, allocateSectors)) return FALSE;
            totalNumOfSectors -= allocateSectors;
        }
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(PersistentBitmap *freeMap)
{
    for (int i = 0; i < numDirectSectors; i++) {
        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
        freeMap->Clear((int) dataSectors[i]);
    }
    for (int i = 0; i < numIndirectPointerSectors; i++) {
        table[i].Deallocate(freeMap);
    }
    delete[] table;
    table = nullptr;
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    kernel->synchDisk->ReadSector(sector, (char*)cache);
    numBytes = cache[0];
    numDirectSectors = cache[1];
    numIndirectPointerSectors = cache[2];
    for (int i=0; i < MAX_NUM_OF_DIRECT_POINTER; i++){
        dataSectors[i] = cache[3+i];
    }
    
    // ensure the pointer is null
    if (table != nullptr) {
        delete[] table;
        table = nullptr;
    }
    table = new SingleIndirectPointer[numIndirectPointerSectors];
    for (int i = 0; i < NUM_OF_INDIRECT_POINTER; i++){
        singleIndirectSectors[i] = cache[3 + MAX_NUM_OF_DIRECT_POINTER + i];
        if (i < numIndirectPointerSectors) {
            table[i].FetchFrom(singleIndirectSectors[i]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    cache[0] = numBytes;
    cache[1] = numDirectSectors;
    cache[2] = numIndirectPointerSectors;
    for (int i = 0; i < MAX_NUM_OF_DIRECT_POINTER; i++){
        cache[3 + i] = dataSectors[i];
    }
    for (int i = 0; i < NUM_OF_INDIRECT_POINTER; i++){
        cache[3 + MAX_NUM_OF_DIRECT_POINTER + i] = singleIndirectSectors[i];
        if (i < numIndirectPointerSectors) {
            table[i].WriteBack(singleIndirectSectors[i]);
        }
    }

    kernel->synchDisk->WriteSector(sector, (char*)cache); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int sectorIndex = offset / SectorSize;
    if (sectorIndex < MAX_NUM_OF_DIRECT_POINTER) 
    {
        return dataSectors[sectorIndex];
    }
    // count the sector in indirect pointer
    else {
        int sectorIndexInWholeIndirect = sectorIndex - MAX_NUM_OF_DIRECT_POINTER;   // Ex. 21 - 21 = 0, 22 - 21 = 1
        int indirectIndex = sectorIndexInWholeIndirect / MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER;   // Ex. 0 / 31 = 0, 1 / 31 = 0, 31 / 31 = 1
        sectorIndex = sectorIndexInWholeIndirect % MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER;   // 0 % 31 = 0, 1 % 31 = 1, 31 % 31  = 0
        return table[indirectIndex].GetSectorByIndex(sectorIndex);
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = k = 0; i < numDirectSectors; i++) {
        printf("\nFile contents in Sector %d:\n", dataSectors[i]);
        kernel->synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
            if ('\040' <= data[j] && data[j] <= '\176') {
                printf("%c", data[j]);
            }
            else {
                printf("\\%x", (unsigned char)data[j]);
            }
        }
        printf("\n"); 
        delete [] data;
    }
}

bool 
SingleIndirectPointer::Allocate(PersistentBitmap *freeMap, int needNumSectors)
{
    numSectors = needNumSectors;
    if (freeMap->NumClear() < numSectors){
	    return FALSE;		// not enough space
    }
    for (int i = 0; i < numSectors; i++) {
        dataSectors[i] = freeMap->FindAndSet();
        // since we checked that there was enough free space,
        // we expect this to succeed
        ASSERT(dataSectors[i] >= 0);
    }
    return TRUE;
}

void 
SingleIndirectPointer::Deallocate(PersistentBitmap *freeMap)
{
    for (int i = 0; i < numSectors; i++) {
        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
        freeMap->Clear((int) dataSectors[i]);
    }
}

void 
SingleIndirectPointer::FetchFrom(int sector)
{
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    kernel->synchDisk->ReadSector(sector, (char*)cache);
    numSectors = cache[0];
    for (int i=0; i < MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER; i++){
        dataSectors[i] = cache[1+i];
    }
}

void 
SingleIndirectPointer::WriteBack(int sector)
{
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    cache[0] = numSectors;
    for (int i=0; i < MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER; i++){
        cache[1+i] = dataSectors[i];
    }
    kernel->synchDisk->WriteSector(sector, (char*)cache); 
}

int
SingleIndirectPointer::GetSectorByIndex(int sectorIndex)
{
    ASSERT(numSectors > sectorIndex); // prevent out of range access
    return dataSectors[sectorIndex];
}