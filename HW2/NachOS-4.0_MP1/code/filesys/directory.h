// directory.h
//	Data structures to manage a UNIX-like directory of file names.
//
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "pbitmap.h"
#include "openfile.h"

#define FILE_TYPE 1
#define DIR_TYPE 2
#define FileNameMaxLen 9  // for simplicity, we assume
                          // file names are <= 9 characters long
#define NumDirEntries 64
#define DIR_SIZE NumDirEntries * sizeof(DirectoryEntry)

// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.

class DirectoryEntry {
   public:
    bool inUse;                     // Is this directory entry in use?
    int fileType;                   // Is this directory entry is file or dir
    int sector;                     // Location on disk to find the
                                    //   FileHeader for this file
    char name[FileNameMaxLen + 1];  // Text name for file, with +1 for
                                    // the trailing '\0'
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk.

class Directory {
   public:
    Directory(int size);  // Initialize an empty directory
                          // with space for "size" files
    ~Directory();         // De-allocate the directory

    void FetchFrom(OpenFile *file);  // Init directory contents from disk
    void WriteBack(OpenFile *file);  // Write modifications to
                                     // directory contents back to disk

    int Find(char *name);  // Find the sector number of the
                           // FileHeader for file: "name"

    int Find(int index);  // Find the sector number by entry table index

    char *GetName(int index);  // Get the entry name by entry table index

    int GetTotalNum();  // Get total entry number of the directory

    bool IsDirectory(char *name);  // return directory entry is directory or not by entry name

    bool IsDirectory(int index);  // return directory entry is directory or not by entry index

    bool Add(char *name, int newSector, int type);  // Add a file name into the directory

    bool Remove(char *name);  // Remove a file from the directory

    bool Remove(int index);  // Remove a file from the directory by entry index

    bool RemoveRecursive(PersistentBitmap *freeMap);   // Remove everything inside this dir

    void List();  // Print the names of all the files
                  //  in the directory

    void ListRecursive(int offset);

    void Print();  // Verbose print of the contents
                   //  of the directory -- all the file
                   //  names and their contents.

   private:
    int tableSize;          // Number of directory entries
    DirectoryEntry *table;  // Table of pairs:
                            // <file name, file header location>

    int FindIndex(char *name);  // Find the index into the directory
                                //  table corresponding to "name"
};

#endif  // DIRECTORY_H
