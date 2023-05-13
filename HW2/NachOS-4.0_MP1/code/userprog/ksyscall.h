/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"

#include "synchconsole.h"

typedef int OpenFileId;

void SysHalt()
{
  kernel->interrupt->Halt();
}

void SysPrintInt(int value)
{
	kernel->interrupt->PrintInt(value);
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysCreate(char *filename, int initialSize)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename, initialSize);
}

// -1: open fail
// fd
OpenFileId SysOpen(char *filename)
{
	return kernel->interrupt->OpenFile(filename);
}

// -1: write fail
// size 
int SysWrite(char *buffer, int size, OpenFileId fd)
{
	return kernel->interrupt->WriteFile(buffer, size, fd);
}

// 1: close success
// 0: close fail
int SysClose(OpenFileId fd)
{
	return kernel->interrupt->CloseFile(fd);
}

// -1: read fail
// size
int SysRead(char *buffer, int size, OpenFileId fd)
{
	return kernel->interrupt->ReadFile(buffer, size, fd);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
