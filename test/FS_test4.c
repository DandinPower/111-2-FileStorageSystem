#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz\n";
	int iterations = 20;
	int length = 27 * iterations;
	int success = Create("/t20000/t20004/file3", length);
	OpenFileId fid;
	int i = 0;
	if (success != 1) MSG("Failed on creating file");
	fid = Open("/t20000/t20004/file3");
	if (fid <= 0) MSG("Failed on opening file");
	for (i = 0; i < length; ++i) {
		int arrayOffset = (i % 27);		
		int count = Write(test + arrayOffset, 1, fid);
		if (count != 1) MSG("Failed on writing file");
	}
	success = Close(fid);
	if (success != 1) MSG("Failed on closing file");
	Halt();
}

