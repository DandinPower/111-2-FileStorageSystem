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

#include "filehdr.h"

#include "copyright.h"
#include "debug.h"
#include "main.h"
#include "synchdisk.h"

DataPointerInterface::~DataPointerInterface() {
    // not necessary to do anything
}

DataPointerInterface* GetNewPointerByLevel(int level) {
    switch (level) {
        case LEVEL_1:
            return new DirectPointer();
            break;
        case LEVEL_2:
            return new SingleIndirectPointer();
            break;
        case LEVEL_3:
            return new DoubleIndirectPointer();
            break;
        case LEVEL_4:
            return new TripleIndirectPointer();
            break;
        default:
            return nullptr; // impossible case
            break;
    }
}

DirectPointer::~DirectPointer() {
    // not necessary to do anything
}

bool DirectPointer::Allocate(PersistentBitmap *freeMap, int numSectors) {
    ASSERT(numSectors == 1);  // because directPointer must  only have 1
    if (freeMap->NumClear() < numSectors) {
        return false;  // not enough space for pointer
    }
    dataSector = freeMap->FindAndSet();
    ASSERT(dataSector >= 0);
    return true;
}

void DirectPointer::Deallocate(PersistentBitmap *freeMap) {
    ASSERT(freeMap->Test((int)dataSector));  // ought to be marked!
    freeMap->Clear((int)dataSector);
}

void DirectPointer::FetchFrom(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    kernel->synchDisk->ReadSector(sectorNumber, (char *)cache);
    dataSector = cache[0];
}

void DirectPointer::WriteBack(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    cache[0] = dataSector;
    kernel->synchDisk->WriteSector(sectorNumber, (char *)cache);
}

int DirectPointer::ByteToSector(int offset) {
    ASSERT(offset <= SectorSize);
    return dataSector;  // because for direct pointer, only have one dataSector
}

SingleIndirectPointer::~SingleIndirectPointer() {
    // not necessary to do anything
}

bool SingleIndirectPointer::Allocate(PersistentBitmap *freeMap, int numSectors) {
    ASSERT(numSectors <= LEVEL_1_SECTOR_NUM);  // because directPointer must  only have 1
    numPointer = numSectors;
    if (freeMap->NumClear() < numPointer) {
        return false;  // not enough space for pointer
    }
    for (int i = 0; i < numPointer; i++) {
        pointerSectors[i] = freeMap->FindAndSet();
        ASSERT(pointerSectors[i] >= 0);
    }

    // int remainSector = numSectors;
    // for (int i = 0; i < numPointer; i++) {
    //     ASSERT(remainSector > 0);
    //     int allocateSectors = min(remainSector, SECTOR_NUM_IN_LEVEL[0]);
    //     ASSERT(table[i].Allocate(freeMap, allocateSectors));
    //     remainSector -= allocateSectors;
    // }
    // ASSERT(remainSector == 0);
    return true;
}

void SingleIndirectPointer::Deallocate(PersistentBitmap *freeMap) {
    for (int i = 0; i < numPointer; i++) {
        // table[i].Deallocate(freeMap);
        ASSERT(freeMap->Test((int)pointerSectors[i]));  // ought to be marked!
        freeMap->Clear((int)pointerSectors[i]);
    }
}

void SingleIndirectPointer::FetchFrom(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    kernel->synchDisk->ReadSector(sectorNumber, (char *)cache);
    numPointer = cache[0];
    for(int i = 0; i < NUM_INDIRECT_POINTER; i++) {
        pointerSectors[i] = cache[1 + i];
    }
    // for(int i = 0; i < numPointer; i++) {
    //     ASSERT(pointerSectors[i] >= 0);
    //     table[i].FetchFrom(pointerSectors[i]);
    // }
}

void SingleIndirectPointer::WriteBack(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    cache[0] = numPointer;
    for(int i = 0; i < NUM_INDIRECT_POINTER; i++) {
        cache[1 + i] = pointerSectors[i];
    }
    // for(int i = 0; i < numPointer; i++) {
    //     ASSERT(pointerSectors[i] >= 0);
    //     table[i].WriteBack(pointerSectors[i]);
    // }
    kernel->synchDisk->WriteSector(sectorNumber, (char *)cache);
}

int SingleIndirectPointer::ByteToSector(int offset) {
    int pointerIndex = divRoundDown(offset, SIZE_IN_LEVEL[0]);
    int newOffset = offset % SIZE_IN_LEVEL[0];
    ASSERT(pointerIndex < NUM_INDIRECT_POINTER);
    ASSERT(pointerSectors[pointerIndex] >= 0);
    // return table[pointerIndex].ByteToSector(newOffset);
    return pointerSectors[pointerIndex];
}

DoubleIndirectPointer::~DoubleIndirectPointer() {
    // not necessary to do anything
}

bool DoubleIndirectPointer::Allocate(PersistentBitmap *freeMap, int numSectors) {
    ASSERT(numSectors <= LEVEL_2_SECTOR_NUM);  // because directPointer must  only have 1
    numPointer = divRoundUp(numSectors, LEVEL_1_SECTOR_NUM);
    if (freeMap->NumClear() < numPointer) {
        return false;  // not enough space for pointer
    }
    for (int i = 0; i < numPointer; i++) {
        pointerSectors[i] = freeMap->FindAndSet();
        ASSERT(pointerSectors[i] >= 0);
    }

    int remainSector = numSectors;
    for (int i = 0; i < numPointer; i++) {
        ASSERT(remainSector > 0);
        int allocateSectors = min(remainSector, SECTOR_NUM_IN_LEVEL[1]);
        ASSERT(table[i].Allocate(freeMap, allocateSectors));
        remainSector -= allocateSectors;
    }
    ASSERT(remainSector == 0);
    return true;
}

void DoubleIndirectPointer::Deallocate(PersistentBitmap *freeMap) {
    for (int i = 0; i < numPointer; i++) {
        table[i].Deallocate(freeMap);
    }
}

void DoubleIndirectPointer::FetchFrom(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    kernel->synchDisk->ReadSector(sectorNumber, (char *)cache);
    numPointer = cache[0];
    for(int i = 0; i < NUM_INDIRECT_POINTER; i++) {
        pointerSectors[i] = cache[1 + i];
    }
    for(int i = 0; i < numPointer; i++) {
        ASSERT(pointerSectors[i] >= 0);
        table[i].FetchFrom(pointerSectors[i]);
    }
}

void DoubleIndirectPointer::WriteBack(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    cache[0] = numPointer;
    for(int i = 0; i < NUM_INDIRECT_POINTER; i++) {
        cache[1 + i] = pointerSectors[i];
    }
    for(int i = 0; i < numPointer; i++) {
        ASSERT(pointerSectors[i] >= 0);
        table[i].WriteBack(pointerSectors[i]);
    }
    kernel->synchDisk->WriteSector(sectorNumber, (char *)cache);
}

int DoubleIndirectPointer::ByteToSector(int offset) {
    int pointerIndex = divRoundDown(offset, SIZE_IN_LEVEL[1]);
    int newOffset = offset % SIZE_IN_LEVEL[1];
    ASSERT(pointerIndex < NUM_INDIRECT_POINTER);
    ASSERT(pointerSectors[pointerIndex] >= 0);
    return table[pointerIndex].ByteToSector(newOffset);
}

TripleIndirectPointer::~TripleIndirectPointer() {
    // not necessary to do anything
}

bool TripleIndirectPointer::Allocate(PersistentBitmap *freeMap, int numSectors) {
    ASSERT(numSectors <= LEVEL_3_SECTOR_NUM);  // because directPointer must  only have 1
    numPointer = divRoundUp(numSectors, LEVEL_2_SECTOR_NUM);
    if (freeMap->NumClear() < numPointer) {
        return false;  // not enough space for pointer
    }
    for (int i = 0; i < numPointer; i++) {
        pointerSectors[i] = freeMap->FindAndSet();
        ASSERT(pointerSectors[i] >= 0);
    }

    int remainSector = numSectors;
    for (int i = 0; i < numPointer; i++) {
        ASSERT(remainSector > 0);
        int allocateSectors = min(remainSector, SECTOR_NUM_IN_LEVEL[2]);
        ASSERT(table[i].Allocate(freeMap, allocateSectors));
        remainSector -= allocateSectors;
    }
    ASSERT(remainSector == 0);
    return true;
}

void TripleIndirectPointer::Deallocate(PersistentBitmap *freeMap) {
    for (int i = 0; i < numPointer; i++) {
        table[i].Deallocate(freeMap);
    }
}

void TripleIndirectPointer::FetchFrom(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    kernel->synchDisk->ReadSector(sectorNumber, (char *)cache);
    numPointer = cache[0];
    for(int i = 0; i < NUM_INDIRECT_POINTER; i++) {
        pointerSectors[i] = cache[1 + i];
    }
    for(int i = 0; i < numPointer; i++) {
        ASSERT(pointerSectors[i] >= 0);
        table[i].FetchFrom(pointerSectors[i]);
    }
}

void TripleIndirectPointer::WriteBack(int sectorNumber) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    cache[0] = numPointer;
    for(int i = 0; i < NUM_INDIRECT_POINTER; i++) {
        cache[1 + i] = pointerSectors[i];
    }
    for(int i = 0; i < numPointer; i++) {
        ASSERT(pointerSectors[i] >= 0);
        table[i].WriteBack(pointerSectors[i]);
    }
    kernel->synchDisk->WriteSector(sectorNumber, (char *)cache);
}

int TripleIndirectPointer::ByteToSector(int offset) {
    int pointerIndex = divRoundDown(offset, SIZE_IN_LEVEL[2]);
    int newOffset = offset % SIZE_IN_LEVEL[2];
    ASSERT(pointerIndex < NUM_INDIRECT_POINTER);
    ASSERT(pointerSectors[pointerIndex] >= 0);
    return table[pointerIndex].ByteToSector(newOffset);
}

//----------------------------------------------------------------------
// FileHeader::~FileHeader
// 	delete the singleIndirectpointer table if it allocated
//----------------------------------------------------------------------

FileHeader::~FileHeader() {
    for (int i = 0; i < NUM_FILE_HEADER_POINTER; i++) {
        if (table[i] != nullptr) {
            delete table[i];
            table[i] = nullptr;
        }
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

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize) {
    numBytes = fileSize;
    int numSectors = divRoundUp(fileSize, SectorSize);

    if (fileSize <= LEVEL_1_SIZE)
        level = LEVEL_1;
    else if (fileSize <= LEVEL_2_SIZE)
        level = LEVEL_2;
    else if (fileSize <= LEVEL_3_SIZE)
        level = LEVEL_3;
    else if (fileSize <= LEVEL_4_SIZE)
        level = LEVEL_4;
    else
        return false;  // even the maximum level can't have capacity to store all the data

    numPointer = divRoundUp(numSectors, SECTOR_NUM_IN_LEVEL[level - 1]);
    ASSERT(numPointer <= NUM_FILE_HEADER_POINTER);
    if (freeMap->NumClear() < numPointer) {
        return false;  // not enough space for pointer
    }
    for (int i = 0; i < numPointer; i++) {
        pointerSectors[i] = freeMap->FindAndSet();
        // since we checked that there was enough free space,
        // we expect this to succeed
        ASSERT(pointerSectors[i] >= 0);
    }

    int remainSector = numSectors;
    for (int i = 0; i < numPointer; i++) {
        if (table[i] != nullptr) delete table[i];
        table[i] = GetNewPointerByLevel(level);
        ASSERT(table[i] != nullptr);
        // let each pointer to allocate their space
        ASSERT(remainSector > 0);
        int allocateSectors = min(remainSector, SECTOR_NUM_IN_LEVEL[level - 1]);
        ASSERT(table[i]->Allocate(freeMap, allocateSectors));
        remainSector -= allocateSectors;
    }
    ASSERT(remainSector == 0);
    return true;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap) {
    for (int i = 0; i < NUM_FILE_HEADER_POINTER; i++) {
        if (table[i] != nullptr) {
            table[i]->Deallocate(freeMap);
            delete table[i];
            table[i] = nullptr;

            ASSERT(freeMap->Test((int)pointerSectors[i]));  // ought to be marked!
            freeMap->Clear((int)pointerSectors[i]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    kernel->synchDisk->ReadSector(sector, (char *)cache);
    numBytes = cache[0];
    numPointer = cache[1];
    for (int i = 0; i < NUM_FILE_HEADER_POINTER; i++) {
        pointerSectors[i] = cache[2 + i];
    }

    bool decideLevel = true;
    if (numBytes <= LEVEL_1_SIZE)
        level = LEVEL_1;
    else if (numBytes <= LEVEL_2_SIZE)
        level = LEVEL_2;
    else if (numBytes <= LEVEL_3_SIZE)
        level = LEVEL_3;
    else if (numBytes <= LEVEL_4_SIZE)
        level = LEVEL_4;
    else
        decideLevel = false;  // even the maximum level can't have capacity to store all the data
    ASSERT(decideLevel);

    for (int i = 0; i < numPointer; i++) {
        if (table[i] != nullptr) delete table[i];
        table[i] = GetNewPointerByLevel(level);
        ASSERT(table[i] != nullptr);
        ASSERT(pointerSectors[i] >= 0);
        table[i]->FetchFrom(pointerSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector) {
    int cacheArraySize = SectorSize / sizeof(int);
    int cache[cacheArraySize];
    memset(cache, -1, sizeof(cache));
    cache[0] = numBytes;
    cache[1] = numPointer;
    for (int i = 0; i < NUM_FILE_HEADER_POINTER; i++) {
        cache[2 + i] = pointerSectors[i];
    }

    for (int i = 0; i < numPointer; i++) {
        ASSERT(pointerSectors[i] >= 0);
        ASSERT(table[i] != nullptr);
        table[i]->WriteBack(pointerSectors[i]);
    }
    kernel->synchDisk->WriteSector(sector, (char *)cache);
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

int FileHeader::ByteToSector(int offset) {
    int pointerIndex = divRoundDown(offset, SIZE_IN_LEVEL[level - 1]);
    int newOffset = offset % SIZE_IN_LEVEL[level - 1];
    ASSERT(pointerIndex < NUM_FILE_HEADER_POINTER);
    ASSERT(table[pointerIndex] != nullptr);
    return table[pointerIndex]->ByteToSector(newOffset);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength() {
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print() {
    int i, j, k;
    // char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = k = 0; i < numPointer; i++) {
        // printf("\nFile contents in Sector %d:\n", dataSectors[i]);
        // kernel->synchDisk->ReadSector(dataSectors[i], data);
        // for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
        //     if ('\040' <= data[j] && data[j] <= '\176') {
        //         printf("%c", data[j]);
        //     } else {
        //         printf("\\%x", (unsigned char)data[j]);
        //     }
        // }
        // printf("\n");
        // delete[] data;
    }
}