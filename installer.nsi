;Copyright (C) 2010  Stefan Sundin (recover89@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.


!define APP_NAME      "TrayCD"
!define APP_VERSION   "1.2"
!define APP_URL       "http://code.google.com/p/traycd/"
!define APP_UPDATEURL "http://traycd.googlecode.com/svn/wiki/latest-stable.txt"

;Libraries

!include "MUI2.nsh"
!include "Sections.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

; General

Name "${APP_NAME} ${APP_VERSION}"
OutFile "build/${APP_NAME}-${APP_VERSION}.exe"
InstallDir "$PROGRAMFILES\${APP_NAME}"
InstallDirRegKey HKCU "Software\${APP_NAME}" "Install_Dir"
RequestExecutionLevel admin
ShowInstDetails hide
ShowUninstDetails show
SetCompressor /SOLID lzma

; Interface

!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
!define MUI_LANGDLL_REGISTRY_KEY "Software\${APP_NAME}" 
!define MUI_LANGDLL_REGISTRY_VALUENAME "Language"

!define MUI_COMPONENTSPAGE_NODESC

!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\info.txt"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION "Launch"

; Pages

Page custom PageUpgrade PageUpgradeLeave
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPage
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPage
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Variables

Var UpgradeState

; Languages

!include "localization\installer.nsh"
!insertmacro MUI_RESERVEFILE_LANGDLL

!macro Lang id lang
${If} $LANGUAGE == ${id}
	File "build\${lang}\${APP_NAME}\info.txt"
	WriteINIStr "$INSTDIR\${APP_NAME}.ini" "${APP_NAME}" "Language" "${lang}"
${EndIf}
!macroend

; Functions

!macro CloseApp un
Function ${un}CloseApp
	;Close app if running
	FindWindow $0 "${APP_NAME}" ""
	IntCmp $0 0 done
		${If} $UpgradeState != ${BST_CHECKED}
			StrCpy $1 "$(L10N_RUNNING)"
			${If} "${un}" == "un."
				StrCpy $1 "$1$\n$(L10N_RUNNING_UNINSTALL)"
			${EndIf}
			MessageBox MB_ICONINFORMATION|MB_YESNO "$1" /SD IDYES IDNO done
		${EndIf}
		DetailPrint "Closing running ${APP_NAME}."
		SendMessage $0 ${WM_CLOSE} 0 0
		waitloop:
			Sleep 10
			FindWindow $0 "${APP_NAME}" ""
			IntCmp $0 0 closed waitloop waitloop
	closed:
	Sleep 100 ;Sleep 100ms extra to let Windows do its thing
	done:
FunctionEnd
!macroend
!insertmacro CloseApp ""
!insertmacro CloseApp "un."

; Detect previous installation

Var Upgradebox
Var Uninstallbox

Function PageUpgrade
	ReadRegStr $0 HKCU "Software\${APP_NAME}" "Install_Dir"
	IfFileExists $0 +2
		Abort
	
	nsDialogs::Create 1018
	!insertmacro MUI_HEADER_TEXT "$(L10N_UPGRADE_TITLE)" "$(L10N_UPGRADE_SUBTITLE)"
	${NSD_CreateLabel} 0 0 100% 20u "$(L10N_UPGRADE_HEADER)"
	
	${NSD_CreateRadioButton} 0 45 100% 10u "$(L10N_UPGRADE_UPGRADE)"
	Pop $Upgradebox
	${NSD_CreateLabel} 16 60 100% 20u "$(L10N_UPGRADE_INI)"
	
	${NSD_CreateRadioButton} 0 95 100% 10u "$(L10N_UPGRADE_INSTALL)"
	Pop $0
	
	${NSD_CreateRadioButton} 0 130 100% 10u "$(L10N_UPGRADE_UNINSTALL)"
	Pop $Uninstallbox
	
	;Check the correct button when going back to this page
	${If} $UpgradeState == ${BST_UNCHECKED}
		${NSD_Check} $0
	${Else}
		${NSD_Check} $Upgradebox
	${EndIf}
	
	nsDialogs::Show
FunctionEnd

Function PageUpgradeLeave
	${NSD_GetState} $Upgradebox $UpgradeState
	${NSD_GetState} $Uninstallbox $0
	${If} $0 == ${BST_CHECKED}
		Exec "$INSTDIR\Uninstall.exe"
		Quit
	${EndIf}
FunctionEnd

; Installer

Section "$(L10N_UPDATE_SECTION)" sec_update
	NSISdl::download "${APP_UPDATEURL}" "$TEMP\${APP_NAME}-updatecheck"
	Pop $0
	StrCmp $0 "success" +3
		DetailPrint "Update check failed. Error: $0."
		Goto done
	FileOpen $0 "$TEMP\${APP_NAME}-updatecheck" r
	IfErrors done
	FileRead $0 $1
	FileClose $0
	Delete /REBOOTOK "$TEMP\${APP_NAME}-updatecheck"
	${If} $1 > ${APP_VERSION}
		MessageBox MB_ICONINFORMATION|MB_YESNO "$(L10N_UPDATE_DIALOG)" /SD IDNO IDNO done
			ExecShell "open" "${APP_URL}"
			Quit
	${EndIf}
	done:
SectionEnd

Section "${APP_NAME} (${APP_VERSION})" sec_app
	SectionIn RO
	
	;Close app if running
	Call CloseApp
	
	SetOutPath "$INSTDIR"
	
	;Store directory and version
	WriteRegStr HKCU "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
	WriteRegStr HKCU "Software\${APP_NAME}" "Version" "${APP_VERSION}"
	
	;Rename old ini file if it exists
	IfFileExists "${APP_NAME}.ini" 0 +2
		Rename "${APP_NAME}.ini" "${APP_NAME}-old.ini"
	
	;Install files
	!ifdef x64
	${If} ${RunningX64}
		File "build\x64\${APP_NAME}.exe"
	${Else}
		File "build\${APP_NAME}.exe"
	${EndIf}
	!else
	File "build\${APP_NAME}.exe"
	!endif
	File "${APP_NAME}.ini"
	
	!insertmacro Lang ${LANG_ENGLISH}  en-US
	!insertmacro Lang ${LANG_SPANISH}  es-ES
	!insertmacro Lang ${LANG_GALICIAN} gl-ES
	!insertmacro Lang ${LANG_FARSI}    fa-IR
	
	;Create uninstaller
	WriteUninstaller "Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" '"$INSTDIR\${APP_NAME}.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "HelpLink" "${APP_URL}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
SectionEnd

Section "$(L10N_SHORTCUT)" sec_shortcut
	CreateShortCut "$SMPROGRAMS\${APP_NAME}.lnk" "$INSTDIR\${APP_NAME}.exe" "" "$INSTDIR\${APP_NAME}.exe" 0
SectionEnd

Section /o "$(L10N_AUTOSTART)" sec_autostart
SectionEnd

Function Launch
	Exec "$INSTDIR\${APP_NAME}.exe"
FunctionEnd

;Used when upgrading to skip the components and directory pages
Function SkipPage
	${If} $UpgradeState == ${BST_CHECKED}
		!insertmacro UnselectSection ${sec_shortcut}
		Abort
	${EndIf}
FunctionEnd

Function .onInit
	;Detect x64
	!ifdef x64
	${If} ${RunningX64}
		SectionSetText ${sec_app} "${APP_NAME} (x64)"
		;Only set x64 installation dir if not already installed
		ReadRegStr $0 HKCU "Software\${APP_NAME}" "Install_Dir"
		IfFileExists $0 +2
			StrCpy $INSTDIR "$PROGRAMFILES64\${APP_NAME}"
	${EndIf}
	!endif
	;Display language selection
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

; Uninstaller

Function un.onInit
	!insertmacro MUI_UNGETLANGUAGE
FunctionEnd

Section "Uninstall"
	Call un.CloseApp

	Delete /REBOOTOK "$INSTDIR\${APP_NAME}.exe"
	Delete /REBOOTOK "$INSTDIR\${APP_NAME}.ini"
	Delete /REBOOTOK "$INSTDIR\${APP_NAME}-old.ini"
	Delete /REBOOTOK "$INSTDIR\info.txt"
	Delete /REBOOTOK "$INSTDIR\Uninstall.exe"
	RMDir  /REBOOTOK "$INSTDIR"

	Delete /REBOOTOK "$SMPROGRAMS\${APP_NAME}.lnk"

	DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}"
	DeleteRegKey /ifempty HKCU "Software\${APP_NAME}"
	DeleteRegKey /ifempty HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd
