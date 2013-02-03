/*
	Copyright (C) 2013  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

#define UNICODE
#define _UNICODE
#define _WIN32_IE 0x0600

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#include <mmsystem.h>

//App
#define APP_NAME            "TrayCD"
#define APP_VERSION         "1.3b1"
#define APP_URL             L"http://code.google.com/p/traycd/"
#define APP_UPDATE_STABLE   L"http://traycd.googlecode.com/svn/wiki/latest-stable.txt"
#define APP_UPDATE_UNSTABLE L"http://traycd.googlecode.com/svn/wiki/latest-unstable.txt"

//Messages
#define WM_TRAY                WM_USER+1
#define SWM_AUTOSTART_ON       WM_APP+1
#define SWM_AUTOSTART_OFF      WM_APP+2
#define SWM_SETTINGS           WM_APP+3
#define SWM_CHECKFORUPDATE     WM_APP+4
#define SWM_UPDATE             WM_APP+5
#define SWM_SPIN               WM_APP+6
#define SWM_ABOUT              WM_APP+7
#define SWM_EXIT               WM_APP+8
#define SWM_TOGGLE             WM_APP+10 //10-36 reserved for toggle

//Stuff missing in MinGW
#ifndef NIIF_USER
#define HWND_MESSAGE ((HWND)-3)
#define NIIF_USER 4
#define NIN_BALLOONSHOW        WM_USER+2
#define NIN_BALLOONHIDE        WM_USER+3
#define NIN_BALLOONTIMEOUT     WM_USER+4
#define NIN_BALLOONUSERCLICK   WM_USER+5
#endif

//Boring stuff
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hinst = NULL;
HWND g_hwnd = NULL;
UINT WM_TASKBARCREATED = 0;
char inipath[MAX_PATH];

//Cool stuff
char cdrom[27] = "";
short open[26] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//Include stuff
#include "localization/strings.h"
#include "include/error.c"
#include "include/autostart.c"
#include "include/tray.c"
#include "include/update.c"

//Entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	g_hinst = hInst;
	
	//Create window
	WNDCLASSEX wnd = {sizeof(WNDCLASSEX), 0, WindowProc, 0, 0, hInst, NULL, NULL, NULL, NULL, TEXT(APP_NAME), NULL};
	RegisterClassEx(&wnd);
	g_hwnd = CreateWindowEx(0, wnd.lpszClassName, TEXT(APP_NAME), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, NULL);
	
	//Load settings
	GetModuleFileNameA(NULL, inipath, sizeof(inipath)/sizeof(char));
	PathRemoveFileSpecA(inipath);
	strcat(inipath, "\\"APP_NAME".ini");
	char txt[30];
	GetPrivateProfileStringA(APP_NAME, "Language", "en-US", txt, sizeof(txt)/sizeof(char), inipath);
	int i;
	for (i=0; languages[i].code != NULL; i++) {
		if (!strcmp(txt,languages[i].code)) {
			l10n = languages[i].strings;
			break;
		}
	}
	
	//Tray icon
	InitTray();
	UpdateTray();
	
	//Check for update
	GetPrivateProfileStringA("Update", "CheckOnStartup", "0", txt, sizeof(txt)/sizeof(char), inipath);
	if (atoi(txt)) {
		CheckForUpdate(0);
	}
	
	//Message loop
	MSG msg;
	while (GetMessage(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void DetectDrives() {
	cdrom[0] = '\0';
	
	//Load static list
	char drives[30];
	GetPrivateProfileStringA(APP_NAME, "Drives", "", drives, sizeof(drives)/sizeof(char), inipath);
	if (drives[0] != '\0') {
		int i = 0;
		char *x;
		for (x=drives; *x != '\0'; x++) {
			*x = toupper(*x);
			if (*x >= 'A' && *x <= 'Z') {
				strncat(cdrom, x, 1); // Append character to list
				i++;
			}
		}
		if (i > 0) {
			return;
		}
	}
	
	//Check for CD drives
	char drive[] = "X:\\";
	char x;
	for (x='A'; x <= 'Z'; x++) {
		drive[0] = x;
		if (GetDriveTypeA(drive) == DRIVE_CDROM) {
			strncat(cdrom, &x, 1); // Append character to list
		}
	}
}

DWORD WINAPI _ToggleCD(LPVOID arg) {
	char x = *(char*)arg;
	free(arg);
	
	//Try to figure out if the CD tray is ejected
	wchar_t drive[10];
	swprintf(drive, L"%hc:", x); // X:
	//drive[0] = x;
	wchar_t name[MAX_PATH+1];
	if (GetVolumeInformation(drive,name,MAX_PATH+1,NULL,NULL,NULL,NULL,0) == 1) {
		//The tray is closed and has a CD in it
		open[x-'A'] = 0;
	}
	
	//Unlock the drive
	swprintf(drive, L"\\\\.\\%hc:", x); // \\.\X:
	HANDLE device = CreateFile(drive, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (device == INVALID_HANDLE_VALUE) {
		#ifdef DEBUG
		Error(L"CreateFile()", L"Unlocking CD-ROM failed.", GetLastError(), TEXT(__FILE__), __LINE__);
		#endif
	}
	else {
		DWORD bytesReturned; //Not used
		PREVENT_MEDIA_REMOVAL pmr = { FALSE }; //This is really just a BOOL
		BOOL result = DeviceIoControl(device, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(PREVENT_MEDIA_REMOVAL), NULL, 0, &bytesReturned, NULL);
		if (result == 0) {
			#ifdef DEBUG
			Error(L"DeviceIoControl()", L"Unlocking CD-ROM failed.", GetLastError(), TEXT(__FILE__), __LINE__);
			#endif
		}
		CloseHandle(device);
	}
	
	//Toggle drive
	MCI_OPEN_PARMS mci;
	ZeroMemory(&mci, sizeof(MCI_OPEN_PARMS));
	mci.lpstrDeviceType = (LPCWSTR)MCI_DEVTYPE_CD_AUDIO;
	swprintf(drive, L"%hc", x);
	mci.lpstrElementName = drive;
	if (!mciSendCommandA(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_NOTIFY|MCI_OPEN_ELEMENT,(DWORD_PTR)&mci)) {
		open[x-'A'] ^= 1;
		mciSendCommand(mci.wDeviceID, MCI_SET, (open[x-'A']?MCI_SET_DOOR_OPEN:MCI_SET_DOOR_CLOSED), 0);
		mciSendCommand(mci.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
	}
	
	return 0;
}

void ToggleCD(int n) {
	//Check if out-of-bounds
	if (n >= strlen(cdrom)) {
		return;
	}
	char *x = malloc(sizeof(char));
	*x = cdrom[n];
	HANDLE thread = CreateThread(NULL, 0, _ToggleCD, x, 0, NULL);
	CloseHandle(thread);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_TRAY) {
		if (lParam == WM_LBUTTONDOWN) {
			DetectDrives();
			ToggleCD(0);
			SpinIcon(1.5);
		}
		else if (lParam == WM_MBUTTONDOWN) {
			DetectDrives();
			ToggleCD(1);
			SpinIcon(1.5);
		}
		else if (lParam == WM_RBUTTONDOWN) {
			DetectDrives();
			ShowContextMenu(hwnd);
		}
		else if (lParam == NIN_BALLOONUSERCLICK) {
			SendMessage(hwnd, WM_COMMAND, SWM_UPDATE, 0);
		}
	}
	else if (msg == WM_TASKBARCREATED) {
		tray_added = 0;
		UpdateTray();
	}
	else if (msg == WM_COMMAND) {
		int wmId=LOWORD(wParam), wmEvent=HIWORD(wParam);
		if (wmId >= SWM_TOGGLE && wmId <= SWM_TOGGLE+26) {
			int n = wmId-SWM_TOGGLE-20; //This is really weird, but for some reason wmId-SWM_TOGGLE needs to be subtracted by 20
			ToggleCD(n);
			SpinIcon(1.5);
		}
		else if (wmId == SWM_AUTOSTART_ON) {
			SetAutostart(1);
		}
		else if (wmId == SWM_AUTOSTART_OFF) {
			SetAutostart(0);
		}
		else if (wmId == SWM_SETTINGS) {
			ShellExecuteA(NULL, "open", inipath, NULL, NULL, SW_SHOWNORMAL);
		}
		else if (wmId == SWM_CHECKFORUPDATE) {
			CheckForUpdate(1);
		}
		else if (wmId == SWM_UPDATE) {
			if (MessageBox(NULL,l10n->update_dialog,TEXT(APP_NAME),MB_ICONINFORMATION|MB_YESNO|MB_TOPMOST|MB_SETFOREGROUND) == IDYES) {
				OpenUrl(APP_URL);
			}
		}
		else if (wmId == SWM_SPIN) {
			SpinIcon(5);
		}
		else if (wmId == SWM_ABOUT) {
			MessageBox(NULL, l10n->about, l10n->about_title, MB_ICONINFORMATION|MB_OK);
		}
		else if (wmId == SWM_EXIT) {
			DestroyWindow(hwnd);
		}
	}
	else if (msg == WM_DESTROY) {
		showerror = 0;
		RemoveTray();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
