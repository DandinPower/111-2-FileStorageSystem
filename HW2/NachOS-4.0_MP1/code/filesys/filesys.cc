// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "filesys.h"

#include <string.h>

#include "copyright.h"
#include "debug.h"
#include "directory.h"
#include "disk.h"
#include "filehdr.h"
#include "pbitmap.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format) {
    DEBUG(dbgFile, "Initializing the file system.");
    if (format) {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);

        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!
        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.
        
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile);  // flush changes to disk
        directory->WriteBack(directoryFile);

        if (debug->IsEnabled('f')) {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
        currentDirectoryFile = new OpenFile(DirectorySector);  // root directory
        currentDirectory = new Directory(NumDirEntries);
        currentDirectory->FetchFrom(currentDirectoryFile);
    } else {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        currentDirectoryFile = new OpenFile(DirectorySector);  // root directory
        currentDirectory = new Directory(NumDirEntries);
        currentDirectory->FetchFrom(currentDirectoryFile);
    }
}

FileSystem::~FileSystem() {
    if (currentDirectoryFile != NULL)
        delete currentDirectoryFile;
    if (currentDirectory != NULL)
        delete currentDirectory;
    delete freeMapFile;
    delete directoryFile;
}

bool FileSystem::ChangeCurrentDirectory(char *name) {
    currentDirectory->FetchFrom(currentDirectoryFile);
    int dirSector = currentDirectory->Find(name);
    if (dirSector == -1) {
        // std::cout << "Dir " << name << "Not found" << std::endl;
        return false;
    } else {
        currentDirectory->WriteBack(currentDirectoryFile);
        delete currentDirectoryFile;
        delete currentDirectory;
        currentDirectoryFile = new OpenFile(dirSector);
        currentDirectory = new Directory(NumDirEntries);
        currentDirectory->FetchFrom(currentDirectoryFile);
        return true;
    }
}

bool FileSystem::ChangeCurrentDirectoryByWholePath(char *path, char *currentPath, char *filename) {
    ResetToRootDirectory();
    memset(currentPath, 0, sizeof(char) * PATH_MAX_LEN);
    memset(filename, 0, sizeof(char) * FileNameMaxLen);
    if (path[0] != '/') {
        // std::cout << "path didn't start with '/'" << std::endl;
        return false;
    } else {
        char splitPath[PATH_MAX_LEN];
        memset(splitPath, 0, sizeof(char) * PATH_MAX_LEN);
        strcpy(splitPath, path);

        char currentParsePath[PATH_MAX_LEN] = "/";

        char *pathArr[PATH_DEPTH];
        int pathLength = 0;

        char *p;
        p = strtok(splitPath, "/");
        while (p != NULL) {
            pathArr[pathLength] = p;
            pathLength += 1;
            p = strtok(NULL, "/");
        }
        // traverse dir if not found, cout error
        for (int i = 0; i < pathLength - 1; i++) {
            if (!ChangeCurrentDirectory(pathArr[i])) {
                // std::cout << "didn't found dir: " << pathArr[i] << " in dir: " << currentParsePath << std::endl;
                return false;
            }
            strcat(currentParsePath, pathArr[i]);
            strcat(currentParsePath, "/");
        }
        strcpy(currentPath, currentParsePath);
        if (pathLength > 0)
            strcpy(filename, pathArr[pathLength - 1]);
        return true;
    }
}

void FileSystem::ResetToRootDirectory() {
    if (currentDirectoryFile != NULL)
        delete currentDirectoryFile;
    if (currentDirectory != NULL)
        delete currentDirectory;
    currentDirectoryFile = new OpenFile(DirectorySector);
    currentDirectory = new Directory(NumDirEntries);
    currentDirectory->FetchFrom(currentDirectoryFile);
}

bool FileSystem::CreateDirectory(char *name) {
    // std::cout << "Creating directory " << name << " by filesystem" << std::endl;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
    currentDirectory->FetchFrom(currentDirectoryFile);
    if (currentDirectory->Find(name) != -1) {
        success = FALSE;  // dir is already in directory
        // std::cout << "dir \"" << name << "\" is already in directory" << std::endl;
    } else {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet();  // find a sector to hold the file header
        if (sector == -1) {
            success = FALSE;  // no free block for file header
            // std::cout << "no free block for file header" << std::endl;
        } else if (!currentDirectory->Add(name, sector, DIR_TYPE)) {
            success = FALSE;
            // std::cout << "no space in directory" << std::endl;
        } else {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, DIR_SIZE)) {
                success = FALSE;  // no space on disk for data
                // std::cout << "no space on disk for data" << std::endl;
            } else {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                OpenFile *newDirFile = new OpenFile(sector);
                Directory *newDir = new Directory(NumDirEntries);
                newDir->WriteBack(newDirFile);
                delete newDirFile;
                delete newDir;

                currentDirectory->WriteBack(currentDirectoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool FileSystem::Create(char *path, int initialSize) {
    bool success = false;
    char currentPath[PATH_MAX_LEN];
    char filename[FileNameMaxLen];
    memset(currentPath, 0, sizeof(char) * PATH_MAX_LEN);
    memset(filename, 0, sizeof(char) * FileNameMaxLen);
    if (ChangeCurrentDirectoryByWholePath(path, currentPath, filename)) {
        // create dir on the based on current dir
        if (CreateFile(filename, initialSize)) {
            // std::cout << "create file: " << filename << " on dir \"" << currentPath << "\" success!" << std::endl;
            success = true;
        } else {
            // std::cout << "create file: " << filename << " on dir \"" << currentPath << "\" fail!" << std::endl;
        }
    }
    ResetToRootDirectory();
    return success;
}

bool FileSystem::CreateFile(char *name, int initialSize) {
    // std::cout << "Creating file " << name << " by filesystem" << std::endl;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
    currentDirectory->FetchFrom(currentDirectoryFile);
    if (currentDirectory->Find(name) != -1) {
        success = FALSE;  // dir is already in directory
        // std::cout << "file \"" << name << "\" is already in directory" << std::endl;
    } else {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet();  // find a sector to hold the file header
        if (sector == -1) {
            success = FALSE;  // no free block for file header
            // std::cout << "no free block for file header" << std::endl;
        } else if (!currentDirectory->Add(name, sector, FILE_TYPE)) {
            success = FALSE;
            // std::cout << "no space in directory" << std::endl;
        } else {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize)) {
                success = FALSE;  // no space on disk for data
                // std::cout << "no space on disk for data" << std::endl;
            } else {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                currentDirectory->WriteBack(currentDirectoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *path) {
    OpenFile *openFile = NULL;
    int sector;
    DEBUG(dbgFile, "Opening file" << path);

    char currentPath[PATH_MAX_LEN];
    char filename[FileNameMaxLen];
    memset(currentPath, 0, sizeof(char) * PATH_MAX_LEN);
    memset(filename, 0, sizeof(char) * FileNameMaxLen);
    if (ChangeCurrentDirectoryByWholePath(path, currentPath, filename)) {
        sector = currentDirectory->Find(filename);
        if (sector >= 0) {
            openFile = new OpenFile(sector);  // name was found in directory
            // std::cout << "open file: " << filename << " on dir \"" << currentPath << "\" success!" << std::endl;
        } else {
            // std::cout << "open file: " << filename << " on dir \"" << currentPath << "\" fail!" << std::endl;
        }
    }
    ResetToRootDirectory();
    return openFile;
}

OpenFileId
FileSystem::OpenAFile(char *path) {
    OpenFile *openFile = NULL;
    int sector;
    DEBUG(dbgFile, "Opening file" << path);

    char currentPath[PATH_MAX_LEN];
    char filename[FileNameMaxLen];
    memset(currentPath, 0, sizeof(char) * PATH_MAX_LEN);
    memset(filename, 0, sizeof(char) * FileNameMaxLen);
    if (ChangeCurrentDirectoryByWholePath(path, currentPath, filename)) {
        sector = currentDirectory->Find(filename);
        if (sector >= 0) {
            openFile = new OpenFile(sector);  // name was found in directory
            table.insert({sector, openFile});
            // std::cout << "open file: " << filename << " on dir \"" << currentPath << "\" success!" << std::endl;
        } else {
            // std::cout << "open file: " << filename << " on dir \"" << currentPath << "\" fail!" << std::endl;
        }
    }
    ResetToRootDirectory();
    return sector;
}

int FileSystem::WriteFile(char *buffer, int size, OpenFileId fd) {
    OpenFile *openFile = table[fd];
    if (!openFile) return -1;
    return openFile->Write(buffer, size);
}

int FileSystem::ReadFile(char *buffer, int size, OpenFileId fd) {
    OpenFile *openFile = table[fd];
    if (!openFile) return -1;
    return openFile->Read(buffer, size);
}

int FileSystem::CloseFile(OpenFileId fd) {
    OpenFile *openFile = table[fd];
    if (!openFile) return 0;
    OpenFile *eraseFile = openFile;
    table.erase(fd);
    delete eraseFile;
    return 1;
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool FileSystem::Remove(char *path) {
    int sector;
    char currentPath[PATH_MAX_LEN];
    char filename[FileNameMaxLen];
    memset(currentPath, 0, sizeof(char) * PATH_MAX_LEN);
    memset(filename, 0, sizeof(char) * FileNameMaxLen);
    if (ChangeCurrentDirectoryByWholePath(path, currentPath, filename)) {
        if (strcmp(filename, "")==0) {
            // cout << "Can't remove root dir" << endl;
            return FALSE;
        }
        sector = currentDirectory->Find(filename);
        if (sector == -1) {
            return FALSE;  // file not found
        }
        if (currentDirectory->IsDirectory(filename)) {
            RemoveDir(sector, filename);
        }
        else {
            RemoveFile(sector, filename);
        }
    }
    ResetToRootDirectory();
    return TRUE;
}

bool FileSystem::RemoveDir(int sector, char *dirName) {
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);
    OpenFile *removeDirFile = new OpenFile(sector);
    Directory *removeDir = new Directory(NumDirEntries);
    removeDir->FetchFrom(removeDirFile);
    removeDir->RemoveRecursive(freeMap);
    removeDir->WriteBack(removeDirFile);
    delete removeDirFile;
    delete removeDir;

    FileHeader *fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);
    fileHdr->Deallocate(freeMap);  // remove data blocks
    freeMap->Clear(sector);        // remove header block
    currentDirectory->Remove(dirName);
    freeMap->WriteBack(freeMapFile);                    // flush to disk
    currentDirectory->WriteBack(currentDirectoryFile);  // flush to disk
    delete fileHdr;
    delete freeMap;
}

bool FileSystem::RemoveFile(int sector, char *fileName) {
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);
    freeMap = new PersistentBitmap(freeMapFile, NumSectors);
    fileHdr->Deallocate(freeMap);  // remove data blocks
    freeMap->Clear(sector);        // remove header block
    currentDirectory->Remove(fileName);
    freeMap->WriteBack(freeMapFile);                    // flush to disk
    currentDirectory->WriteBack(currentDirectoryFile);  // flush to disk
    delete fileHdr;
    delete freeMap;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List(char *path) {
    char currentPath[PATH_MAX_LEN];
    char filename[FileNameMaxLen];
    memset(currentPath, 0, sizeof(char) * PATH_MAX_LEN);
    memset(filename, 0, sizeof(char) * FileNameMaxLen);
    if (ChangeCurrentDirectoryByWholePath(path, currentPath, filename)) {
        if (strcmp(filename, "")) {
            if (ChangeCurrentDirectory(filename))
                strcat(currentPath, filename);
        }
        // std::cout << "List file in dir \"" << currentPath << "\"" << std::endl;
        currentDirectory->List();
    }
    ResetToRootDirectory();
}

//----------------------------------------------------------------------
// FileSystem::ListRecursive
// 	List all the files in the file system directory recursively.
//----------------------------------------------------------------------

void FileSystem::ListRecursive(char *path) {
    char currentPath[PATH_MAX_LEN];
    char filename[FileNameMaxLen];
    memset(currentPath, 0, sizeof(char) * PATH_MAX_LEN);
    memset(filename, 0, sizeof(char) * FileNameMaxLen);
    if (ChangeCurrentDirectoryByWholePath(path, currentPath, filename)) {
        if (strcmp(filename, "")) {
            if (ChangeCurrentDirectory(filename))
                strcat(currentPath, filename);
        }
        int offset = 0;
        // std::cout << "List file in dir \"" << currentPath << "\"" << std::endl;
        currentDirectory->ListRecursive(offset);
    }
    ResetToRootDirectory();
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print() {
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    currentDirectory->FetchFrom(directoryFile);
    currentDirectory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
}

#endif  // FILESYS_STUB
