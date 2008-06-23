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

#include <windows.h>
#include <time.h>
#include <mmsystem.h>

//Tray messages
#define WM_ICONTRAY       WM_USER+1
#define SWM_TOGGLE        WM_APP+1
#define SWM_SPIN          WM_APP+2
#define SWM_AUTOSTART_ON  WM_APP+3
#define SWM_AUTOSTART_OFF WM_APP+4
#define SWM_ABOUT         WM_APP+5
#define SWM_EXIT          WM_APP+6

//Stuff
LPSTR szClassName="TrayCD";
LRESULT CALLBACK MyWndProc(HWND, UINT, WPARAM, LPARAM);

//Global info
static HICON icon[4];
static NOTIFYICONDATA traydata;
static UINT WM_TASKBARCREATED;
static int tray_added=0;
static int iconpos=0;
static int eject=1;

static char msg[100];

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
	wnd.lpszClassName=szClassName;
	
	//Register class
	if (RegisterClass(&wnd) == 0) {
		sprintf(msg,"RegisterClass() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Error", MB_ICONERROR|MB_OK);
		return 1;
	}
	
	//Create window
	HWND hWnd;
	hWnd=CreateWindow(szClassName, "TrayCD", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);               //no parameters to pass
	//ShowWindow(hWnd, iCmdShow); //Show
	//UpdateWindow(hWnd); //Update
	
	//Register TaskbarCreated so we can readd the tray icon if explorer.exe crashes
	if ((WM_TASKBARCREATED=RegisterWindowMessage("TaskbarCreated")) == 0) {
		sprintf(msg,"RegisterWindowMessage() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	
	//Load icons
	char iconname[6];
	for (iconpos=0; iconpos < 4; iconpos++) {
		sprintf(iconname, "icon%d", iconpos);
		if ((icon[iconpos] = LoadImage(hInst, iconname, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)) == NULL) {
			sprintf(msg,"LoadImage() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
			MessageBox(NULL, msg, "TrayCD Error", MB_ICONERROR|MB_OK);
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
	traydata.hWnd=hWnd;
	traydata.uCallbackMessage=WM_ICONTRAY;
	strncpy(traydata.szTip,"TrayCD",sizeof(traydata.szTip));
	traydata.hIcon=icon[iconpos];
	
	//Add tray icon
	AddTray();

	//Message loop
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void ShowContextMenu(HWND hWnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu;
	if ((hMenu = CreatePopupMenu()) == NULL) {
		sprintf(msg,"CreatePopupMenu() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "KillKeys Warning", MB_ICONWARNING|MB_OK);
	}
	
	//Open/Close
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_TOGGLE, (eject?"Open CD Tray":"Close CD Tray"));
	
	//Spin icon (just for fun)
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_SPIN, "Spin icon");
	
	//Autostart
	//Check registry
	int autostart=0;
	//Open key
	HKEY key;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key) != ERROR_SUCCESS) {
		sprintf(msg,"RegOpenKeyEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Read value
	char autostart_value[MAX_PATH+10];
	DWORD len=sizeof(autostart_value);
	DWORD res=RegQueryValueEx(key,"TrayCD",NULL,NULL,(LPBYTE)autostart_value,&len);
	if (res != ERROR_FILE_NOT_FOUND && res != ERROR_SUCCESS) {
		sprintf(msg,"RegQueryValueEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Close key
	if (RegCloseKey(key) != ERROR_SUCCESS) {
		sprintf(msg,"RegCloseKey() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Get path
	char path[MAX_PATH];
	if (GetModuleFileName(NULL,path,sizeof(path)) == 0) {
		sprintf(msg,"GetModuleFileName() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
	}
	//Compare
	char pathcmp[MAX_PATH+10];
	sprintf(pathcmp,"\"%s\"",path);
	if (!strcmp(pathcmp,autostart_value)) {
		autostart=1;
	}
	//Add to menu
	InsertMenu(hMenu, -1, MF_BYPOSITION|(autostart?MF_CHECKED:0), (autostart?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), "Autostart");
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, SWM_ABOUT, "");
	
	//About
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, "About");
	
	//Exit
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, "Exit");

	//Must set window to the foreground, or else the menu won't disappear when clicking outside it
	SetForegroundWindow(hWnd);

	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL );
	DestroyMenu(hMenu);
}

int AddTray() {
	if (tray_added) {
		//Tray already added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_ADD,&traydata) == FALSE) {
		sprintf(msg,"Shell_NotifyIcon() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return 1;
	}
	
	//Success
	tray_added=1;
}

int RemoveTray() {
	if (!tray_added) {
		//Tray not added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_DELETE,&traydata) == FALSE) {
		sprintf(msg,"Shell_NotifyIcon() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return 1;
	}
	
	//Success
	tray_added=0;
}

void ToggleCD() {
	MCI_OPEN_PARMS open;
	ZeroMemory(&open, sizeof(MCI_OPEN_PARMS));
	open.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
	open.lpstrElementName = "D";

	if (!mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_NOTIFY, (DWORD) &open)) {
		mciSendCommand(open.wDeviceID, MCI_SET, (eject?MCI_SET_DOOR_OPEN:MCI_SET_DOOR_CLOSED), 0);
		mciSendCommand(open.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
	}
	
	eject=!eject;
}

void RotateIcon(double howlong) {
	time_t timenow=time(NULL), timethen;
	for (timethen=time(NULL); difftime(timethen,timenow) < howlong; timethen=time(NULL)) {
		//Sleep(100);
		iconpos = (iconpos+1)%4;
		traydata.hIcon = icon[iconpos];
		if (Shell_NotifyIcon(NIM_MODIFY,&traydata) == FALSE) {
			sprintf(msg,"Shell_NotifyIcon() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
			MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
			return;
		}
		//Sleep
		//TODO: Implement some kind of sine function to make the spinning look cooler
		if (difftime(timethen,timenow+50) < howlong) {
			Sleep(50);
		}
	}
}

void AddAutostart() {
	//Open key
	HKEY key;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&key) != ERROR_SUCCESS) {
		sprintf(msg,"RegOpenKeyEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
	//Get path
	char path[MAX_PATH];
	if (GetModuleFileName(NULL,path,sizeof(path)) == 0) {
		sprintf(msg,"GetModuleFileName() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
	//Add
	char value[MAX_PATH+10];
	sprintf(value,"\"%s\"",path);
	if (RegSetValueEx(key,"TrayCD",0,REG_SZ,value,strlen(value)+1) != ERROR_SUCCESS) {
		sprintf(msg,"RegSetValueEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
	//Close key
	if (RegCloseKey(key) != ERROR_SUCCESS) {
		sprintf(msg,"RegCloseKey() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
}

void RemoveAutostart() {
	//Open key
	HKEY key;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&key) != ERROR_SUCCESS) {
		sprintf(msg,"RegOpenKeyEx() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
	//Remove
	if (RegDeleteValue(key,"TrayCD") != ERROR_SUCCESS) {
		sprintf(msg,"RegDeleteValue() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
	//Close key
	if (RegCloseKey(key) != ERROR_SUCCESS) {
		sprintf(msg,"RegCloseKey() failed (error code: %d) in file %s, line %d.",GetLastError(),__FILE__,__LINE__);
		MessageBox(NULL, msg, "TrayCD Warning", MB_ICONWARNING|MB_OK);
		return;
	}
}

LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_COMMAND) {
		int wmId=LOWORD(wParam), wmEvent=HIWORD(wParam);
		if (wmId == SWM_TOGGLE) {
			RotateIcon(0.9);
			ToggleCD();
		}
		else if (wmId == SWM_SPIN) {
			RotateIcon(5);
		}
		else if (wmId == SWM_AUTOSTART_ON) {
			AddAutostart();
		}
		else if (wmId == SWM_AUTOSTART_OFF) {
			RemoveAutostart();
		}
		else if (wmId == SWM_ABOUT) {
			MessageBox(NULL, "TrayCD - 0.2\nhttp://traycd.googlecode.com/\nrecover89@gmail.com\n\nEject and insert the cd tray via the tray icon.\n\nSend feedback to recover89@gmail.com", "About TrayCD", MB_ICONINFORMATION|MB_OK);
		}
		else if (wmId == SWM_EXIT) {
			DestroyWindow(hWnd);
		}
	}
	else if (msg == WM_ICONTRAY) {
		if (lParam == WM_LBUTTONDOWN) {
			RotateIcon(0.9);
			ToggleCD();
		}
		else if (lParam == WM_RBUTTONDOWN) {
			ShowContextMenu(hWnd);
		}
	}
	else if (msg == WM_TASKBARCREATED) {
		tray_added=0;
		AddTray();
	}
	else if (msg == WM_DESTROY) {
		if (tray_added) {
			RemoveTray();
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
