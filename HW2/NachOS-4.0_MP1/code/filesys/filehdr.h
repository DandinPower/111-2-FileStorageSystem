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

#define NUM_INT_IN_SECTOR (SectorSize / sizeof(int))
#define NUM_FILE_HEADER_POINTER (NUM_INT_IN_SECTOR - 2)
#define NUM_INDIRECT_POINTER (NUM_INT_IN_SECTOR - 1)

#define LEVEL_1_SECTOR_NUM NUM_FILE_HEADER_POINTER
#define LEVEL_2_SECTOR_NUM NUM_FILE_HEADER_POINTER *NUM_INDIRECT_POINTER
#define LEVEL_3_SECTOR_NUM NUM_FILE_HEADER_POINTER *NUM_INDIRECT_POINTER *NUM_INDIRECT_POINTER
#define LEVEL_4_SECTOR_NUM NUM_FILE_HEADER_POINTER *NUM_INDIRECT_POINTER *NUM_INDIRECT_POINTER *NUM_INDIRECT_POINTER

#define LEVEL_1_SIZE SectorSize *LEVEL_1_SECTOR_NUM  // 128bytes * 30 = 3840bytes
#define LEVEL_2_SIZE SectorSize *LEVEL_2_SECTOR_NUM  // 128bytes * 30 * 31 = 119040bytes(116kB)
#define LEVEL_3_SIZE SectorSize *LEVEL_3_SECTOR_NUM  // 128bytes * 30 * 31 * 31 = 3690240bytes(3.5mB)
#define LEVEL_4_SIZE SectorSize *LEVEL_4_SECTOR_NUM  // 128bytes * 30 * 31 * 31 * 31 = 114,397,440bytes(109mB)

#define LEVEL_1 1
#define LEVEL_2 2
#define LEVEL_3 3
#define LEVEL_4 4

const int SECTOR_NUM_IN_LEVEL[5] = {1, LEVEL_1_SECTOR_NUM, LEVEL_2_SECTOR_NUM, LEVEL_3_SECTOR_NUM, LEVEL_4_SECTOR_NUM};

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

class DataPointerInterface {
   public:
    virtual ~DataPointerInterface() = 0;
    virtual bool Allocate(PersistentBitmap *bitMap, int numSectors) = 0;
    virtual void Deallocate(PersistentBitmap *bitMap) = 0;
    virtual void FetchFrom(int sectorNumber) = 0;
    virtual void WriteBack(int sectorNumber) = 0;
    virtual int ByteToSector(int offset) = 0;
};

class DirectPointer : public DataPointerInterface {
   public:
    ~DirectPointer() override;
    bool Allocate(PersistentBitmap *bitMap, int numSectors) override;
    void Deallocate(PersistentBitmap *bitMap) override;
    void FetchFrom(int sectorNumber) override;
    void WriteBack(int sectorNumber) override;
    int ByteToSector(int offset) override;

   private:
    int dataSector;
};

class SingleIndirectPointer : public DataPointerInterface {
   public:
    ~SingleIndirectPointer() override;
    bool Allocate(PersistentBitmap *bitMap, int numSectors) override;
    void Deallocate(PersistentBitmap *bitMap) override;
    void FetchFrom(int sectorNumber) override;
    void WriteBack(int sectorNumber) override;
    int ByteToSector(int offset) override;

   private:
    int numPointer;  // Number of pointer in the file
    int pointerSectors[NUM_INDIRECT_POINTER];
    DirectPointer table[NUM_INDIRECT_POINTER];
};

// class SingleIndirectPointer {
//    public:
//     bool Allocate(PersistentBitmap *bitMap, int numSectors);
//     void Deallocate(PersistentBitmap *bitMap);
//     void FetchFrom(int sectorNumber);
//     void WriteBack(int sectorNumber);
//     int GetSectorByIndex(int sectorIndex);

//    private:
//     int numSectors;  // Number of data sectors;
//     int dataSectors[NUM_INDIRECT_POINTER];
// };

class FileHeader {
   public:
    ~FileHeader();  // Destructor

    bool Allocate(PersistentBitmap *bitMap, int fileSize);  // Initialize a file header,
                                                            //  including allocating space
                                                            //  on disk for the file data

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
    int numBytes;    // Number of bytes in the file
    int numPointer;  // Number of pointer in the file
    int pointerSectors[NUM_FILE_HEADER_POINTER];
    int level;                                             // represent the header level, not necessary to write back to disk
    DataPointerInterface *table[NUM_FILE_HEADER_POINTER];  // it may have direct, singleIndirect...
};

#endif  // FILEHDR_H
