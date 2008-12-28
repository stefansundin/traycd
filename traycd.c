/*
	TrayCD - Eject and insert the cd tray via a tray icon
	Copyright (C) 2008  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define _WIN32_IE 0x0600
#include <windows.h>
#include <shlwapi.h>
#include <wininet.h>
#include <mmsystem.h>

//App
#define APP_NAME      L"TrayCD"
#define APP_VERSION   "0.4"
#define APP_URL       L"http://traycd.googlecode.com/"
#define APP_UPDATEURL L"http://traycd.googlecode.com/svn/wiki/latest-stable.txt"
//#define DEBUG

//Localization
#ifndef L10N_FILE
#define L10N_FILE "localization/en-US/strings.h"
#endif
#include L10N_FILE
#if L10N_VERSION != 1
#error Localization not up to date!
#endif

//Messages
#define WM_ICONTRAY            WM_USER+1
#define SWM_AUTOSTART_ON       WM_APP+1
#define SWM_AUTOSTART_OFF      WM_APP+2
#define SWM_SPIN               WM_APP+3
#define SWM_UPDATE             WM_APP+4
#define SWM_ABOUT              WM_APP+5
#define SWM_EXIT               WM_APP+6
#define SWM_TOGGLE             WM_APP+10 //10-36 reserved for toggle

//Balloon stuff missing in MinGW
#define NIIF_USER 4
#define NIN_BALLOONSHOW        WM_USER+2
#define NIN_BALLOONHIDE        WM_USER+3
#define NIN_BALLOONTIMEOUT     WM_USER+4
#define NIN_BALLOONUSERCLICK   WM_USER+5

//Boring stuff
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
static HICON icon[15];
static NOTIFYICONDATA traydata;
static UINT WM_TASKBARCREATED=0;
static int tray_added=0;
static int update=0;
struct {
	int CheckForUpdate;
} settings={0};
static wchar_t txt[1000];

//Cool stuff
static wchar_t cdrom[27]=L"";
static int open[26];
static int iconpos=0;

//Error message handling
static int showerror=1;

LRESULT CALLBACK ErrorMsgProc(INT nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HCBT_ACTIVATE) {
		//Edit the caption of the buttons
		SetDlgItemText((HWND)wParam,IDYES,L"Copy error");
		SetDlgItemText((HWND)wParam,IDNO,L"OK");
	}
	return 0;
}

void Error(wchar_t *func, wchar_t *info, int errorcode, int line) {
	if (showerror) {
		//Format message
		wchar_t errormsg[100];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,errorcode,0,errormsg,sizeof(errormsg)/sizeof(wchar_t),NULL);
		errormsg[wcslen(errormsg)-2]='\0'; //Remove that damn newline at the end of the formatted error message
		swprintf(txt,L"%s failed in file %s, line %d.\nError: %s (%d)\n\n%s", func, TEXT(__FILE__), line, errormsg, errorcode, info);
		//Display message
		HHOOK hhk=SetWindowsHookEx(WH_CBT, &ErrorMsgProc, 0, GetCurrentThreadId());
		int response=MessageBox(NULL, txt, APP_NAME" Error", MB_ICONERROR|MB_YESNO|MB_DEFBUTTON2);
		UnhookWindowsHookEx(hhk);
		if (response == IDYES) {
			//Copy message to clipboard
			OpenClipboard(NULL);
			EmptyClipboard();
			wchar_t *data=LocalAlloc(LMEM_FIXED,(wcslen(txt)+1)*sizeof(wchar_t));
			memcpy(data,txt,(wcslen(txt)+1)*sizeof(wchar_t));
			SetClipboardData(CF_UNICODETEXT,data);
			CloseClipboard();
		}
	}
}

//Check for update
DWORD WINAPI _CheckForUpdate() {
	//Open connection
	HINTERNET http, file;
	if ((http=InternetOpen(APP_NAME" - "APP_VERSION,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0)) == NULL) {
		#ifdef DEBUG
		Error(L"InternetOpen()",L"Could not establish connection.\nPlease check for update manually at "APP_URL,GetLastError(),__LINE__);
		#endif
		return;
	}
	if ((file=InternetOpenUrl(http,APP_UPDATEURL,NULL,0,INTERNET_FLAG_NO_AUTH|INTERNET_FLAG_NO_AUTO_REDIRECT|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_COOKIES|INTERNET_FLAG_NO_UI,0)) == NULL) {
		#ifdef DEBUG
		Error(L"InternetOpenUrl()",L"Could not establish connection.\nPlease check for update manually at "APP_URL,GetLastError(),__LINE__);
		#endif
		return;
	}
	//Read file
	char data[20];
	DWORD numread;
	if (InternetReadFile(file,data,sizeof(data),&numread) == FALSE) {
		#ifdef DEBUG
		Error(L"InternetReadFile()",L"Could not read file.\nPlease check for update manually at "APP_URL,GetLastError(),__LINE__);
		#endif
		return;
	}
	data[numread]='\0';
	//Get error code
	wchar_t code[4];
	DWORD len=sizeof(code);
	HttpQueryInfo(file,HTTP_QUERY_STATUS_CODE,&code,&len,NULL);
	//Close connection
	InternetCloseHandle(file);
	InternetCloseHandle(http);
	
	//Make sure the server returned 200
	if (wcscmp(code,L"200")) {
		#ifdef DEBUG
		swprintf(txt,L"Server returned %s error when checking for update.\nPlease check for update manually at "APP_URL,code);
		MessageBox(NULL, txt, APP_NAME, MB_ICONWARNING|MB_OK);
		#endif
		return;
	}
	
	//New version available?
	if (strcmp(data,APP_VERSION)) {
		update=1;
		traydata.uFlags|=NIF_INFO;
		UpdateTray();
		traydata.uFlags^=NIF_INFO;
	}
}

void CheckForUpdate() {
	CreateThread(NULL,0,_CheckForUpdate,NULL,0,NULL);
}

//Entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	//Create window class
	WNDCLASSEX wnd;
	wnd.cbSize=sizeof(WNDCLASSEX);
	wnd.style=0;
	wnd.lpfnWndProc=WindowProc;
	wnd.cbClsExtra=0;
	wnd.cbWndExtra=0;
	wnd.hInstance=hInst;
	wnd.hIcon=NULL;
	wnd.hIconSm=NULL;
	wnd.hCursor=LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR|LR_SHARED);
	wnd.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
	wnd.lpszMenuName=NULL;
	wnd.lpszClassName=APP_NAME;
	
	//Register class
	RegisterClassEx(&wnd);
	
	//Create window
	HWND hwnd=CreateWindowEx(0, wnd.lpszClassName, APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	
	//Load icons
	wchar_t iconname[]=L"trayXX";
	for (iconpos=0; iconpos < 14; iconpos++) {
		swprintf(iconname+4, L"%02d", iconpos);
		if ((icon[iconpos] = LoadImage(hInst, iconname, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)) == NULL) {
			Error(L"LoadImage()",iconname,GetLastError(),__LINE__);
			PostQuitMessage(1);
		}
	}
	//Seed iconpos
	srand(time(NULL));
	iconpos=rand()%14;
	
	//Create icondata
	traydata.cbSize=sizeof(NOTIFYICONDATA);
	traydata.uID=0;
	traydata.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
	traydata.hWnd=hwnd;
	traydata.uCallbackMessage=WM_ICONTRAY;
	wcsncpy(traydata.szTip,APP_NAME,sizeof(traydata.szTip)/sizeof(wchar_t));
	//Balloon tooltip
	traydata.uTimeout=10000;
	wcsncpy(traydata.szInfoTitle,APP_NAME,sizeof(traydata.szInfoTitle)/sizeof(wchar_t));
	wcsncpy(traydata.szInfo,L10N_UPDATE_BALLOON,sizeof(traydata.szInfo)/sizeof(wchar_t));
	traydata.dwInfoFlags=NIIF_USER;
	
	//Register TaskbarCreated so we can re-add the tray icon if explorer.exe crashes
	WM_TASKBARCREATED=RegisterWindowMessage(L"TaskbarCreated");
	
	//Update tray icon
	UpdateTray();
	
	//Check for CD drives
	DWORD drives=GetLogicalDrives();
	int bitmask=1;
	wchar_t driveletters[]=L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;
	for (i=0; i < wcslen(driveletters); i++) {
		open[i]=0;
		if (drives&bitmask) {
			wchar_t drive[]=L"X:\\";
			drive[0]=driveletters[i];
			if (GetDriveType(drive) == DRIVE_CDROM) {
				swprintf(cdrom,L"%s%c",cdrom,driveletters[i]);
			}
		}
		bitmask=bitmask << 1;
	}
	
	//Load settings
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL,path,sizeof(path)/sizeof(wchar_t));
	PathRenameExtension(path,L".ini");
	GetPrivateProfileString(L"Update",L"CheckForUpdate",L"0",txt,sizeof(txt)/sizeof(wchar_t),path);
	swscanf(txt,L"%d",&settings.CheckForUpdate);
	
	//Check for update
	if (settings.CheckForUpdate) {
		CheckForUpdate();
	}
	
	//Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void ShowContextMenu(HWND hwnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu=CreatePopupMenu();
	
	//Open/Close
	int i;
	for (i=0; i < wcslen(cdrom); i++) {
		swprintf(txt,L"%s %c:",(open[i]?L10N_MENU_CLOSE:L10N_MENU_OPEN),cdrom[i]);
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_TOGGLE+i, txt);
	}
	
	//Check autostart
	int autostart=0;
	//Open key
	HKEY key;
	RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key);
	//Read value
	wchar_t autostart_value[MAX_PATH+10];
	DWORD len=sizeof(autostart_value);
	RegQueryValueEx(key,APP_NAME,NULL,NULL,(LPBYTE)autostart_value,&len);
	//Close key
	RegCloseKey(key);
	//Get path
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL,path,MAX_PATH);
	//Compare
	wchar_t pathcmp[MAX_PATH+10];
	swprintf(pathcmp,L"\"%s\"",path);
	if (!wcscmp(pathcmp,autostart_value)) {
		autostart=1;
	}
	//Autostart
	InsertMenu(hMenu, -1, MF_BYPOSITION|(autostart?MF_CHECKED:0), (autostart?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), L10N_MENU_AUTOSTART);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Spin icon (just for fun)
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_SPIN, L10N_MENU_SPIN);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Update
	if (update) {
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_UPDATE, L10N_MENU_UPDATE);
		InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	}
	
	//About
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, L10N_MENU_ABOUT);
	
	//Exit
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, L10N_MENU_EXIT);

	//Track menu
	SetForegroundWindow(hwnd);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(hMenu);
}

int UpdateTray() {
	traydata.hIcon=icon[iconpos];
	
	int tries=0; //If trying to add, try at least five times (required on some slow systems when the program is on autostart since explorer hasn't initialized the tray area)
	while (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&traydata) == FALSE) {
		tries++;
		if (tray_added || tries >= 5) {
			Error(L"Shell_NotifyIcon(NIM_ADD/NIM_MODIFY)",L"Failed to update tray icon.",GetLastError(),__LINE__);
			return 1;
		}
	}
	
	//Success
	tray_added=1;
	return 0;
}

int RemoveTray() {
	if (!tray_added) {
		//Tray not added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_DELETE,&traydata) == FALSE) {
		Error(L"Shell_NotifyIcon(NIM_DELETE)",L"Failed to remove tray icon.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Success
	tray_added=0;
	return 0;
}

void SetAutostart(int on) {
	//Open key
	HKEY key;
	int error;
	if ((error=RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&key)) != ERROR_SUCCESS) {
		Error(L"RegOpenKeyEx(HKEY_CURRENT_USER,'Software\\Microsoft\\Windows\\CurrentVersion\\Run')",L"Error opening the registry.",error,__LINE__);
		return;
	}
	if (on) {
		//Get path
		wchar_t path[MAX_PATH];
		if (GetModuleFileName(NULL,path,MAX_PATH) == 0) {
			Error(L"GetModuleFileName(NULL)",L"",GetLastError(),__LINE__);
			return;
		}
		//Add
		wchar_t value[MAX_PATH+10];
		swprintf(value,L"\"%s\"",path);
		if ((error=RegSetValueEx(key,APP_NAME,0,REG_SZ,(LPBYTE)value,(wcslen(value)+1)*sizeof(wchar_t))) != ERROR_SUCCESS) {
			Error(L"RegSetValueEx('"APP_NAME"')",L"",error,__LINE__);
			return;
		}
	}
	else {
		//Remove
		if ((error=RegDeleteValue(key,APP_NAME)) != ERROR_SUCCESS) {
			Error(L"RegDeleteValue('"APP_NAME"')",L"",error,__LINE__);
			return;
		}
	}
	//Close key
	RegCloseKey(key);
}

DWORD WINAPI _ToggleCD(LPVOID arg) {
	int n=*(int*)arg;
	if (n <= wcslen(cdrom)) {
		//Try to figure out if the CD tray is ejected
		wchar_t drive[]=L"X:";
		drive[0]=cdrom[n];
		wchar_t name[MAX_PATH+1];
		if (GetVolumeInformation(drive,name,MAX_PATH+1,NULL,NULL,NULL,NULL,0) == 1) {
			//The tray is closed and has a CD in it
			open[n]=0;
		}
		
		//Toggle drive
		MCI_OPEN_PARMS mci;
		ZeroMemory(&mci,sizeof(MCI_OPEN_PARMS));
		mci.lpstrDeviceType=(LPCWSTR)MCI_DEVTYPE_CD_AUDIO;
		swprintf(drive,L"%c",cdrom[n]);
		mci.lpstrElementName=drive;
		if (!mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_NOTIFY | MCI_OPEN_ELEMENT, (DWORD) &mci)) {
			open[n]=!open[n];
			mciSendCommand(mci.wDeviceID, MCI_SET, (open[n]?MCI_SET_DOOR_OPEN:MCI_SET_DOOR_CLOSED), 0);
			mciSendCommand(mci.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
		}
	}
	free(arg);
}

void ToggleCD(int p_n) {
	int *n=malloc(sizeof(p_n));
	*n=p_n;
	CreateThread(NULL,0,_ToggleCD,n,0,NULL);
}

DWORD WINAPI _SpinIcon(LPVOID arg) {
	double howlong=*(double*)arg;
	time_t timestart=time(NULL), timenow;
	for (timenow=time(NULL); difftime(timenow,timestart) < howlong; timenow=time(NULL)) {
		iconpos = (iconpos+1)%14;
		UpdateTray();
		//Sleep
		//TODO: Implement some kind of sine function to make the spinning look cooler
		Sleep(20-10*difftime(timenow,timestart)/howlong);
	}
	free(arg);
}

void SpinIcon(double p_howlong) {
	double *howlong=malloc(sizeof(p_howlong));
	*howlong=p_howlong;
	CreateThread(NULL,0,_SpinIcon,howlong,0,NULL);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_ICONTRAY) {
		if (lParam == WM_LBUTTONDOWN) {
			ToggleCD(0);
			SpinIcon(1.5);
		}
		else if (lParam == WM_RBUTTONDOWN) {
			ShowContextMenu(hwnd);
		}
		else if (lParam == WM_MBUTTONDOWN) {
			ToggleCD(1);
			SpinIcon(1.5);
		}
		else if (lParam == NIN_BALLOONUSERCLICK) {
			SendMessage(hwnd,WM_COMMAND,SWM_UPDATE,0);
		}
	}
	else if (msg == WM_TASKBARCREATED) {
		tray_added=0;
		UpdateTray();
	}
	else if (msg == WM_COMMAND) {
		int wmId=LOWORD(wParam), wmEvent=HIWORD(wParam);
		if (wmId >= SWM_TOGGLE && wmId <= SWM_TOGGLE+26) {
			int n=wmId-SWM_TOGGLE-20; //This is really weird, but for some reason wmId-SWM_TOGGLE needs to be subtracted with 20
			ToggleCD(n);
			SpinIcon(1.5);
		}
		else if (wmId == SWM_AUTOSTART_ON) {
			SetAutostart(1);
		}
		else if (wmId == SWM_AUTOSTART_OFF) {
			SetAutostart(0);
		}
		else if (wmId == SWM_SPIN) {
			SpinIcon(5);
		}
		else if (wmId == SWM_UPDATE) {
			if (MessageBox(NULL, L10N_UPDATE_DIALOG, APP_NAME, MB_ICONINFORMATION|MB_YESNO) == IDYES) {
				ShellExecute(NULL, L"open", APP_URL, NULL, NULL, SW_SHOWNORMAL);
			}
		}
		else if (wmId == SWM_ABOUT) {
			MessageBox(NULL, L10N_ABOUT, L10N_ABOUT_TITLE, MB_ICONINFORMATION|MB_OK);
		}
		else if (wmId == SWM_EXIT) {
			DestroyWindow(hwnd);
		}
	}
	else if (msg == WM_DESTROY) {
		showerror=0;
		RemoveTray();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
