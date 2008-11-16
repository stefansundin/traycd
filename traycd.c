/*
	TrayCD - Eject and insert the cd tray via a tray icon
	Copyright (C) 2008  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <mmsystem.h>

//Messages
#define WM_ICONTRAY       WM_USER+1
#define SWM_SPIN          WM_APP+1
#define SWM_AUTOSTART_ON  WM_APP+2
#define SWM_AUTOSTART_OFF WM_APP+3
#define SWM_ABOUT         WM_APP+4
#define SWM_EXIT          WM_APP+5
#define SWM_TOGGLE        WM_APP+10 //10-36 reserved for toggle

//Stuff
LRESULT CALLBACK MyWndProc(HWND, UINT, WPARAM, LPARAM);

static HICON icon[15];
static NOTIFYICONDATA traydata;
static UINT WM_TASKBARCREATED;
static int tray_added=0;

static char cdrom[27]="";
static int open[26];
static int iconpos=0;

static char txt[100];

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	//Create window class
	WNDCLASS wnd;
	wnd.style=CS_HREDRAW | CS_VREDRAW;
	wnd.lpfnWndProc=MyWndProc;
	wnd.cbClsExtra=0;
	wnd.cbWndExtra=0;
	wnd.hInstance=hInst;
	wnd.hIcon=LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	wnd.hCursor=LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR);
	wnd.hbrBackground=(HBRUSH)(COLOR_BACKGROUND+1);
	wnd.lpszMenuName=NULL;
	wnd.lpszClassName="TrayCD";
	
	//Register class
	if (RegisterClass(&wnd) == 0) {
		sprintf(txt,"RegisterClass() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Error", MB_ICONERROR|MB_OK);
		return 1;
	}
	
	//Create window
	HWND hwnd=CreateWindow(wnd.lpszClassName, "TrayCD", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	
	//Register TaskbarCreated so we can re-add the tray icon if explorer.exe crashes
	if ((WM_TASKBARCREATED=RegisterWindowMessage("TaskbarCreated")) == 0) {
		sprintf(txt,"RegisterWindowMessage() failed (error code: %d) in file %s, line %d.\nThis means the tray icon won't be added if (or should I say when) explorer.exe crashes.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	
	//Load icons
	char iconname[]="trayXX";
	for (iconpos=0; iconpos < 14; iconpos++) {
		sprintf(iconname+4, "%02d", iconpos);
		if ((icon[iconpos] = LoadImage(hInst, iconname, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)) == NULL) {
			sprintf(txt,"LoadImage('%s') failed (error code: %d) in file %s, line %d.",iconname,GetLastError(),__FILE__,__LINE__);
			MessageBox(NULL, txt, "TrayCD Error", MB_ICONERROR|MB_OK);
			PostQuitMessage(1);
		}
	}
	
	//Seed iconpos
	srand(time(NULL));
	iconpos=rand()%4;
	
	//Create icondata
	traydata.cbSize=sizeof(NOTIFYICONDATA);
	traydata.uID=0;
	traydata.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
	traydata.hWnd=hwnd;
	traydata.uCallbackMessage=WM_ICONTRAY;
	strncpy(traydata.szTip,"TrayCD",sizeof(traydata.szTip));
	traydata.hIcon=icon[iconpos];
	
	//Add tray icon
	UpdateTray();
	
	//Check for CD drives
	DWORD drives=GetLogicalDrives();
	int bitmask=1;
	char driveletters[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;
	for (i=0; i < strlen(driveletters); i++) {
		open[i]=0;
		if (drives&bitmask) {
			char drive[]="X:\\";
			drive[0]=driveletters[i];
			if (GetDriveType(drive) == DRIVE_CDROM) {
				sprintf(cdrom,"%s%c",cdrom,driveletters[i]);
			}
		}
		bitmask=bitmask << 1;
	}
	
	//Message loop
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void ShowContextMenu(HWND hwnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu;
	if ((hMenu = CreatePopupMenu()) == NULL) {
		sprintf(txt,"CreatePopupMenu() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "KillKeys Warning", MB_ICONWARNING|MB_OK);
	}
	
	//Open/Close
	int i;
	for (i=0; i < strlen(cdrom); i++) {
		sprintf(txt,"%s %c:",(open[i]?"Close":"Open"),cdrom[i]);
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_TOGGLE+i, txt);
	}
	
	//Autostart
	//Check registry
	int autostart=0;
	//Open key
	HKEY key;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key) != ERROR_SUCCESS) {
		sprintf(txt,"RegOpenKeyEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Read value
	char autostart_value[MAX_PATH+10];
	DWORD len=sizeof(autostart_value);
	DWORD res=RegQueryValueEx(key,"TrayCD",NULL,NULL,(LPBYTE)autostart_value,&len);
	if (res != ERROR_FILE_NOT_FOUND && res != ERROR_SUCCESS) {
		sprintf(txt,"RegQueryValueEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Close key
	if (RegCloseKey(key) != ERROR_SUCCESS) {
		sprintf(txt,"RegCloseKey() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Get path
	char path[MAX_PATH];
	if (GetModuleFileName(NULL,path,sizeof(path)) == 0) {
		sprintf(txt,"GetModuleFileName() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Compare
	char pathcmp[MAX_PATH+10];
	sprintf(pathcmp,"\"%s\"",path);
	if (!strcmp(pathcmp,autostart_value)) {
		autostart=1;
	}
	//Add to menu
	InsertMenu(hMenu, -1, MF_BYPOSITION|(autostart?MF_CHECKED:0), (autostart?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), "Autostart");
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Spin icon (just for fun)
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_SPIN, "Spin icon");
	
	//About
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, "About");
	
	//Exit
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, "Exit");

	//Must set window to the foreground, or else the menu won't disappear when clicking outside it
	SetForegroundWindow(hwnd);

	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL );
	DestroyMenu(hMenu);
}

int UpdateTray() {
	traydata.hIcon=icon[iconpos];
	
	if (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&traydata) == FALSE) {
		sprintf(txt,"Failed to add tray icon.\n\nShell_NotifyIcon() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return 1;
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
		sprintf(txt,"Failed to remove tray icon.\n\nShell_NotifyIcon() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return 1;
	}
	
	//Success
	tray_added=0;
	return 0;
}

DWORD WINAPI _ToggleCD(LPVOID arg) {
	int n=*(int*)arg;
	if (n <= strlen(cdrom)) {
		MCI_OPEN_PARMS mci;
		ZeroMemory(&mci, sizeof(MCI_OPEN_PARMS));
		mci.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
		
		char drive[]="X";
		drive[0]=cdrom[n];
		mci.lpstrElementName = drive;
		
		if (!mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_NOTIFY | MCI_OPEN_ELEMENT, (DWORD) &mci)) {
			open[n]=!open[n];
			mciSendCommand(mci.wDeviceID, MCI_SET, (open[n]?MCI_SET_DOOR_OPEN:MCI_SET_DOOR_CLOSED), 0);
			mciSendCommand(mci.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
		}
	}
	free(arg);
}

void ToggleCD(int p_n) {
	int *n=malloc(sizeof(int));
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
	double *howlong=malloc(sizeof(double));
	*howlong=p_howlong;
	CreateThread(NULL,0,_SpinIcon,howlong,0,NULL);
}

void SetAutostart(int on) {
	//Open key
	HKEY key;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&key) != ERROR_SUCCESS) {
		sprintf(txt,"RegOpenKeyEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
	if (on) {
		//Get path
		char path[MAX_PATH];
		if (GetModuleFileName(NULL,path,sizeof(path)) == 0) {
			sprintf(txt,"GetModuleFileName() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
			MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
			return;
		}
		//Add
		char value[MAX_PATH+10];
		sprintf(value,"\"%s\"",path);
		if (RegSetValueEx(key,"TrayCD",0,REG_SZ,value,strlen(value)+1) != ERROR_SUCCESS) {
			sprintf(txt,"RegSetValueEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
			MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
			return;
		}
	}
	else {
		//Remove
		if (RegDeleteValue(key,"TrayCD") != ERROR_SUCCESS) {
			sprintf(txt,"RegDeleteValue() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
			MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
			return;
		}
	}
	//Close key
	if (RegCloseKey(key) != ERROR_SUCCESS) {
		sprintf(txt,"RegCloseKey() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, txt, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
}

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_COMMAND) {
		int wmId=LOWORD(wParam), wmEvent=HIWORD(wParam);
		if (wmId >= SWM_TOGGLE && wmId <= SWM_TOGGLE+26) {
			int n=wmId-SWM_TOGGLE-20; //This is really weird, but somehow wmId-SWM_TOGGLE needs to be subtracted with 20
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
		else if (wmId == SWM_ABOUT) {
			MessageBox(NULL, "TrayCD - 0.3\n\
http://traycd.googlecode.com/\n\
\n\
Eject and insert the cd tray via a tray icon.\n\
Icons by Onyx Reyes, onyxreyes@gmail.com.\n\
\n\
Send feedback to recover89@gmail.com.", "About TrayCD", MB_ICONINFORMATION|MB_OK);
		}
		else if (wmId == SWM_EXIT) {
			DestroyWindow(hwnd);
		}
	}
	else if (msg == WM_ICONTRAY) {
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
	}
	else if (msg == WM_TASKBARCREATED) {
		tray_added=0;
		UpdateTray();
	}
	else if (msg == WM_DESTROY) {
		if (tray_added) {
			RemoveTray();
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
