// filesys.h 
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system. 
//	The "STUB" version just re-defines the Nachos file system 
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.
//
//	The other version is a "real" file system, built on top of 
//	a disk simulator.  The disk is simulated using the native UNIX 
//	file system (in a file named "DISK"). 
//
//	In the "real" implementation, there are two key data structures used 
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.  
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized. 
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include <map>
#include "copyright.h"
#include "sysdep.h"
#include "openfile.h"
#include "directory.h"

typedef int OpenFileId;

#ifdef FILESYS_STUB 		// Temporarily implement file system calls as 
				// calls to UNIX, until the real file system
				// implementation is available

#define FS_OPENFILE_NUMS 20



class FileSystem {
  public:
    FileSystem() { for (int i = 0; i < 20; i++) fileDescriptorTable[i] = NULL; }

    bool Create(char *name) {
		int fileDescriptor = OpenForWrite(name);
		if (fileDescriptor == -1) return FALSE;
		Close(fileDescriptor); 
		return TRUE; 
	}

    OpenFile* Open(char *name) {
	  int fileDescriptor = OpenForReadWrite(name, FALSE);
	  if (fileDescriptor == -1) return NULL;
	  return new OpenFile(fileDescriptor);
      }
	
	OpenFileId OpenAFile(char *name){
		// 檢查開檔
		OpenFile* file = Open(name);
		if (!file) {
			return -1;
		}
		// 檢查並找出空的位置來放OpenFile, 後續如有需要可運用倍增法來實現超出長度的情況; 效率的部分可以透過維護一個freeIndexSet來加速查找freeIndex的時間
		int freeIndex = NULL;
		for (int i=0; i < FS_OPENFILE_NUMS; i++) {
			if (!fileDescriptorTable[i]) {
				freeIndex = i;
			}
		}
		if (!freeIndex) {
			return -1;
		}
		OpenFileId fd = file->GetFileDescriptor();
		fileDescriptorTable[freeIndex] = file;
		return fd;
	}

	// Linear search找到對應的OpenFile來Write -> 後續可用HashTable等辦法來優化
	int WriteFile(char *buffer, int size, OpenFileId fd){
		for (int i=0; i < FS_OPENFILE_NUMS; i++) {
			if (!fileDescriptorTable[i]) continue;
			if (fileDescriptorTable[i]->GetFileDescriptor() == fd) {
				return fileDescriptorTable[i]->Write(buffer, size);
			}
		}
		return -1;
	}

	// Linear search找到對應的OpenFile來Read -> 後續可用HashTable等辦法來優化
	int ReadFile(char *buffer, int size, OpenFileId fd){
		for (int i=0; i < FS_OPENFILE_NUMS; i++) {
			if (!fileDescriptorTable[i]) continue;
			if (fileDescriptorTable[i]->GetFileDescriptor() == fd) {
				return fileDescriptorTable[i]->Read(buffer, size);
			}
		}
		return -1;
	}

	// Linear search找到對應的OpenFile來Close -> 後續可用HashTable等辦法來優化
	int CloseFile(OpenFileId fd){
		for (int i=0; i < FS_OPENFILE_NUMS; i++) {
			if (!fileDescriptorTable[i]) continue;
			if (fileDescriptorTable[i]->GetFileDescriptor() == fd) {
				delete fileDescriptorTable[i];
				fileDescriptorTable[i] = NULL;
				return 1;
			}
		}
		return 0;
	}

    bool Remove(char *name) { return Unlink(name) == 0; }

	OpenFile *fileDescriptorTable[FS_OPENFILE_NUMS];
	
};

#else // FILESYS

#define PATH_DEPTH 25
#define PATH_MAX_LEN PATH_DEPTH * (FileNameMaxLen) + 1	// max path length

class FileSystem {
  public:
    FileSystem(bool format);		// Initialize the file system.
					// Must be called *after* "synchDisk" 
					// has been initialized.
    					// If "format", there is nothing on
					// the disk, so initialize the directory
    					// and the bitmap of free blocks.
	~FileSystem();

	bool CreateDirectory(char *name);	// Create a directory on currentDirectory

	bool ChangeCurrentDirectory(char *name);	// change current directory based on current directory by name
	
	bool ChangeCurrentDirectoryByWholePath(char *path, char *currentPath, char *filename);	// change current directory based on root by path, currentPath is /test if path is /test/test1, filename is test1 if path is /test/test1

	void ResetToRootDirectory();	// change current directory to root

	bool CreateFile(char *name, int initialSize);

    bool Create(char *name, int initialSize);  	
					// Create a file (UNIX creat)

    OpenFile* Open(char *name); 	// Open a file (UNIX open)

	OpenFileId OpenAFile(char *filename);

	int WriteFile(char *buffer, int size, OpenFileId fd);

	int ReadFile(char *buffer, int size, OpenFileId fd);

	int CloseFile(OpenFileId fd);

    bool Remove(char *name);  		// Delete a file (UNIX unlink)
	
	void List(char *path);			// List all the files in the file system path

	void ListRecursive(char *path);		// Recursively List all the files.

    void Print();			// List all the files and their contents

  private:
	OpenFile* freeMapFile;		// Bit map of free disk blocks,
					// represented as a file
	OpenFile* directoryFile;		// "Root" directory -- list of 
					// file names, represented as a file

	OpenFile* currentDirectoryFile;

	Directory* currentDirectory;

	std::map<OpenFileId, OpenFile*> table;
};

#endif // FILESYS

#endif // FS_H
