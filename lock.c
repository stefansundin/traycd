#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <winioctl.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Not enough arguments!\n");
		printf("Usage: %s <drive> [unlock]\n", argv[0]);
		printf("Example: %s d\n", argv[0]);
		printf("Example: %s d unlock\n", argv[0]);
		printf("Note: You must have a CD in the CD-rom drive for this to work.\n", GetLastError());
		printf("Note: If you lock a drive multiple times, you must unlock it just as many times.\n", GetLastError());
		return 0;
	}
	
	// Unlock?
	BOOL lock = TRUE;
	if (argc > 2) {
		lock = FALSE;
	}
	
	// Get handle to drive
	char drive[] = "\\\\.\\X:";
	drive[4] = argv[1][0];
	HANDLE device = CreateFile(drive, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (device == INVALID_HANDLE_VALUE) {
		printf("CreateFile() failed. GetLastError(): %d.\n", GetLastError());
		printf("Query string: %s\n", drive);
		return 1;
	}
	
	// Lock/Unlock the drive
	printf("%s cd-rom drive ... ", (lock?"Locking":"Unlocking")); fflush(stdout);
	DWORD bytesReturned; //Not used
	PREVENT_MEDIA_REMOVAL pmr; //This is really just a BOOL
	pmr.PreventMediaRemoval = lock;
	BOOL result = DeviceIoControl(device, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(PREVENT_MEDIA_REMOVAL), NULL, 0, &bytesReturned, NULL);
	if (result == 0) {
		printf("DeviceIoControl() failed. GetLastError(): %d.\n", GetLastError());
	}
	else {
		printf("Success!\n");
	}
	
	// Clean up
	CloseHandle(device);
	return 0;
}

// Further reading:
// http://techsupt.winbatch.com/TS/T000001010F28.html
// https://arkeon.dyndns.org/svn-scol/trunk/plugins/lib2d%20os%2024/tmp/source/script2.cpp
// IOCTL_STORAGE_MEDIA_REMOVAL on MSDN
