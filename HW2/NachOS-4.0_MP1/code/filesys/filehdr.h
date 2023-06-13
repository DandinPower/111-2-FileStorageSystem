// filehdr.h
//	Data structures for managing a disk file header.
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "pbitmap.h"

#define NUM_OF_POINTER_IN_SECTOR (SectorSize / sizeof(int)) // 128 / 4 = 32
#define INDIRECT_SECTOR_FILE_SIZE (SectorSize * (NUM_OF_POINTER_IN_SECTOR - 1))   // 128 * (32 - 1) = 3968
#define NUM_OF_INDIRECT_POINTER 8
#define NUM_OF_DIRECT_POINTER (NUM_OF_POINTER_IN_SECTOR - NUM_OF_INDIRECT_POINTER - 3)  // 32 - 8 - 3 = 21
#define MAX_FILE_SIZE ((NUM_OF_DIRECT_POINTER * SectorSize) + (NUM_OF_INDIRECT_POINTER * INDIRECT_SECTOR_FILE_SIZE))    // ((21 * 128) + (8 * 3968)) = 2688 + 31744 = 34432

const int MAX_NUM_OF_DIRECT_POINTER = NUM_OF_DIRECT_POINTER;    // 21
const int MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER = NUM_OF_POINTER_IN_SECTOR - 1;   // 32 - 1 = 31

// The following class defines the Nachos "file header" (in UNIX terms,
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks.
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class SingleIndirectPointer {
   public:
    bool Allocate(PersistentBitmap *bitMap, int numSectors);
    void Deallocate(PersistentBitmap *bitMap);
    void FetchFrom(int sectorNumber);
    void WriteBack(int sectorNumber);
    int GetSectorByIndex(int sectorIndex);

   private:
    int numSectors;  // Number of data sectors;
    int dataSectors[MAX_NUM_OF_SECTORS_IN_INDIIRECT_POINTER];
};

class FileHeader {
   public:
    ~FileHeader();  // Destructor

    bool Allocate(PersistentBitmap *bitMap, int fileSize);  // Initialize a file header,
                                                            //  including allocating space
                                                            //  on disk for the file data

    bool AllocateIndirect(PersistentBitmap *bitMap, int needNumSectors);  // allocating space to indirectPointer

    void Deallocate(PersistentBitmap *bitMap);  // De-allocate this file's
                                                //  data blocks

    void FetchFrom(int sectorNumber);  // Initialize file header from disk
    void WriteBack(int sectorNumber);  // Write modifications to file header
                                       //  back to disk

    int ByteToSector(int offset);  // Convert a byte offset into the file
                                   // to the disk sector containing
                                   // the byte

    int FileLength();  // Return the length of the file
                       // in bytes

    void Print();  // Print the contents of the file.

   private:
    int numBytes;                // Number of bytes in the file
    int numDirectSectors;              // Number of direct data sectors in the file
    int dataSectors[MAX_NUM_OF_DIRECT_POINTER];  // Disk sector numbers for each data
    int numIndirectPointerSectors;  // Number of indirect pointer in file
    int singleIndirectSectors[NUM_OF_INDIRECT_POINTER];  // Disk sector numbers for-each singleIndirect sector

    // int numBytes;
    // int numDirectSectors;
    // int numSingleSectors;
    // int numDoubleSectors;
    // int numTripleSectors;
    // int *directSectors;
    // int *singleIndirectSectors;
    // int *doubleIndirectSectors;
    // int *tripleIndirectSectors;



    SingleIndirectPointer *table;
};

#endif  // FILEHDR_H
