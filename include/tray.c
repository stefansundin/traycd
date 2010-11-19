/*
	Tray functions.
	Copyright (C) 2010  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

#include <time.h>

NOTIFYICONDATA tray;
HICON icon[15];
int tray_added = 0;
int iconpos = 0;
extern update;

int InitTray() {
	//Load icons
	wchar_t iconname[] = L"trayXX";
	for (iconpos=0; iconpos <= 14; iconpos++) {
		swprintf(iconname+4, L"%02d", iconpos);
		icon[iconpos] = LoadImage(g_hinst,iconname,IMAGE_ICON,0,0,LR_DEFAULTCOLOR);
		if (icon[iconpos] == NULL) {
			Error(L"LoadImage()", iconname, GetLastError(), TEXT(__FILE__), __LINE__);
			PostQuitMessage(1);
		}
	}
	//Seed iconpos
	srand(time(NULL));
	iconpos = rand()%14;
	
	//Create icondata
	tray.cbSize = sizeof(NOTIFYICONDATA);
	tray.uID = 0;
	tray.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
	tray.hWnd = g_hwnd;
	tray.uCallbackMessage = WM_TRAY;
	wcsncpy(tray.szTip, APP_NAME, sizeof(tray.szTip)/sizeof(wchar_t));
	//Balloon tooltip
	tray.uTimeout = 10000;
	wcsncpy(tray.szInfoTitle, APP_NAME, sizeof(tray.szInfoTitle)/sizeof(wchar_t));
	tray.dwInfoFlags = NIIF_USER;
	
	//Register TaskbarCreated so we can re-add the tray icon if (when) explorer.exe crashes
	WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
	
	return 0;
}

int UpdateTray() {
	tray.hIcon = icon[iconpos];
	
	//Try until it succeeds, sleep 100 ms between each attempt
	while (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&tray) == FALSE) {
		Sleep(100);
	}
	
	//Success
	tray_added = 1;
	return 0;
}

int RemoveTray() {
	if (!tray_added) {
		//Tray not added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_DELETE,&tray) == FALSE) {
		Error(L"Shell_NotifyIcon(NIM_DELETE)", L"Failed to remove tray icon.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Success
	tray_added = 0;
	return 0;
}

void ShowContextMenu(HWND hwnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU menu = CreatePopupMenu();
	
	//Open/Close
	int i;
	wchar_t txt[10];
	for (i=0; i < wcslen(cdrom); i++) {
		swprintf(txt, (open[i]?l10n->menu_close:l10n->menu_open), cdrom[i]);
		InsertMenu(menu, -1, MF_BYPOSITION, SWM_TOGGLE+i, txt);
	}
	
	//Check autostart
	int autostart=0;
	CheckAutostart(&autostart);
	//Options
	HMENU menu_options = CreatePopupMenu();
	InsertMenu(menu_options, -1, MF_BYPOSITION|(autostart?MF_CHECKED:0), (autostart?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), l10n->menu_autostart);
	InsertMenu(menu_options, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	InsertMenu(menu_options, -1, MF_BYPOSITION, SWM_SETTINGS, l10n->menu_settings);
	InsertMenu(menu_options, -1, MF_BYPOSITION, SWM_CHECKFORUPDATE, l10n->menu_chkupdate);
	InsertMenu(menu, -1, MF_BYPOSITION|MF_POPUP, (UINT_PTR)menu_options, l10n->menu_options);
	InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Spin icon (just for fun)
	InsertMenu(menu, -1, MF_BYPOSITION, SWM_SPIN, l10n->menu_spin);
	InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//Update
	if (update) {
		InsertMenu(menu, -1, MF_BYPOSITION, SWM_UPDATE, l10n->menu_update);
	}
	
	//About
	InsertMenu(menu, -1, MF_BYPOSITION, SWM_ABOUT, l10n->menu_about);
	
	//Exit
	InsertMenu(menu, -1, MF_BYPOSITION, SWM_EXIT, l10n->menu_exit);

	//Track menu
	SetForegroundWindow(hwnd);
	TrackPopupMenu(menu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(menu);
}

DWORD WINAPI _SpinIcon(LPVOID arg) {
	double howlong = *(double*)arg;
	free(arg);
	
	time_t timestart = time(NULL), timenow;
	for (timenow=time(NULL); difftime(timenow,timestart) < howlong; timenow=time(NULL)) {
		iconpos = (iconpos+1)%15;
		UpdateTray();
		//Sleep
		//TODO: Implement some kind of sine function to make the spinning look cooler
		Sleep(20-10*difftime(timenow,timestart)/howlong);
	}
	return 0;
}

void SpinIcon(double p_howlong) {
	double *howlong = malloc(sizeof(p_howlong));
	*howlong = p_howlong;
	HANDLE thread = CreateThread(NULL, 0, _SpinIcon, howlong, 0, NULL);
	CloseHandle(thread);
}
