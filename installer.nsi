;TrayCD installer
;NSIS Unicode: http://www.scratchpaper.com/
;
;Copyright (C) 2009  Stefan Sundin (recover89@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.


!define APP_NAME      "TrayCD"
!define APP_VERSION   "1.0"
!define APP_URL       "http://traycd.googlecode.com/"
!define APP_UPDATEURL "http://traycd.googlecode.com/svn/wiki/latest-stable.txt"
!define L10N_VERSION  1

;Libraries

!include "MUI2.nsh"
!include "Sections.nsh"
!include "LogicLib.nsh"

;General

Name "${APP_NAME} ${APP_VERSION}"
OutFile "build/${APP_NAME}-${APP_VERSION}.exe"
InstallDir "$PROGRAMFILES\${APP_NAME}"
InstallDirRegKey HKCU "Software\${APP_NAME}" "Install_Dir"
;RequestExecutionLevel user
ShowInstDetails hide
ShowUninstDetails show
SetCompressor /SOLID lzma

;Interface

!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
!define MUI_LANGDLL_REGISTRY_KEY "Software\${APP_NAME}" 
!define MUI_LANGDLL_REGISTRY_VALUENAME "Language"

!define MUI_COMPONENTSPAGE_NODESC

!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
;!define MUI_FINISHPAGE_SHOWREADME_TEXT "Read info.txt"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\info.txt"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION "Launch"

;Pages

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;Languages

!include "localization\installer.nsh"

!insertmacro MUI_RESERVEFILE_LANGDLL

;Installer

Section "$(L10N_UPDATE_SECTION)" sec_update
	NSISdl::download "${APP_UPDATEURL}" "$TEMP\${APP_NAME}-updatecheck"
	Pop $R0
	StrCmp $R0 "success" +3
		DetailPrint "Update check failed. Error: $R0"
		Goto done
	FileOpen $0 "$TEMP\${APP_NAME}-updatecheck" r
	IfErrors done
	FileRead $0 $1
	FileClose $0
	Delete /REBOOTOK "$TEMP\${APP_NAME}-updatecheck"
	StrCmp ${APP_VERSION} $1 done 0
	MessageBox MB_ICONINFORMATION|MB_YESNO "$(L10N_UPDATE_DIALOG)" IDNO done
		ExecShell "open" "${APP_URL}"
		Quit
	done:
SectionEnd

Section "${APP_NAME} (${APP_VERSION})"
	SectionIn RO
	
	FindWindow $0 "${APP_NAME}" ""
	IntCmp $0 0 continue
		MessageBox MB_ICONINFORMATION|MB_YESNO "$(L10N_RUNNING_INSTALL)" /SD IDYES IDNO continue
			DetailPrint "$(L10N_CLOSING)"
			SendMessage $0 ${WM_CLOSE} 0 0
			Sleep 1000
	continue:
	
	SetOutPath "$INSTDIR"
	
	;Store directory and version
	WriteRegStr HKCU "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
	WriteRegStr HKCU "Software\${APP_NAME}" "Version" "${APP_VERSION}"
	
	;Install files
	File "build\en-US\${APP_NAME}\${APP_NAME}.exe"
	File "build\en-US\${APP_NAME}\${APP_NAME}.ini"
	
	IntCmp $LANGUAGE ${LANG_ENGLISH} en-US
	IntCmp $LANGUAGE ${LANG_SPANISH} es-ES
	en-US:
		File "build\en-US\${APP_NAME}\info.txt"
		Goto files_installed
	es-ES:
		File "build\es-ES\${APP_NAME}\info.txt"
		WriteINIStr "$INSTDIR\${APP_NAME}.ini" "${APP_NAME}" "Language" "es-ES"
		Goto files_installed
	
	files_installed:
	
	;Create uninstaller
	WriteUninstaller "Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
SectionEnd

Section "$(L10N_SHORTCUT)"
	CreateShortCut "$SMPROGRAMS\${APP_NAME}.lnk" "$INSTDIR\${APP_NAME}.exe" "" "$INSTDIR\${APP_NAME}.exe" 0
SectionEnd

Section /o "$(L10N_AUTOSTART)" sec_autostart
SectionEnd

Function Launch
	Exec "$INSTDIR\${APP_NAME}.exe"
FunctionEnd

Function .onInit
	!insertmacro MUI_LANGDLL_DISPLAY
	;If silent, deselect check for update
	IfSilent 0 autostart_check
		!insertmacro UnselectSection ${sec_update}
	autostart_check:
	;Determine current autostart setting
	ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}"
	IfErrors done
		!insertmacro SelectSection ${sec_autostart}
	done:
FunctionEnd

Function .onInstSuccess
	;Set or remove autostart
	${If} ${SectionIsSelected} ${sec_autostart}
		WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}" '"$INSTDIR\${APP_NAME}.exe"'
	${Else}
		DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}"
	${EndIf}
	;Run program if silent
	IfSilent 0 +2
		Call Launch
FunctionEnd

;Uninstaller

Section "Uninstall"
	FindWindow $0 "${APP_NAME}" ""
	IntCmp $0 0 continue
		MessageBox MB_ICONINFORMATION|MB_YESNO "$(L10N_RUNNING_UNINSTALL)" /SD IDYES IDNO continue
			DetailPrint "$(L10N_CLOSING)"
			SendMessage $0 ${WM_CLOSE} 0 0
			Sleep 1000
	continue:

	Delete /REBOOTOK "$INSTDIR\${APP_NAME}.exe"
	Delete /REBOOTOK "$INSTDIR\${APP_NAME}.ini"
	Delete /REBOOTOK "$INSTDIR\info.txt"
	Delete /REBOOTOK "$INSTDIR\Uninstall.exe"
	RMDir /REBOOTOK "$INSTDIR"

	Delete /REBOOTOK "$SMPROGRAMS\${APP_NAME}.lnk"

	DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}"
	DeleteRegKey /ifempty HKCU "Software\${APP_NAME}"
	DeleteRegKey /ifempty HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd

Function un.onInit
	!insertmacro MUI_UNGETLANGUAGE
FunctionEnd
