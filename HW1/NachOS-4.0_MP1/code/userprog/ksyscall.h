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

int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}

int SysOpen(char *filename)
{
	return kernel->interrupt->OpenFile(filename);
}


#endif /* ! __USERPROG_KSYSCALL_H__ */
