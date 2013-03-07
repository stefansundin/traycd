;Copyright (C) 2013  Stefan Sundin (recover89@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.

;For silent install you can use these switches: /S /L=es-ES /D=C:\installdir

;Requires AccessControl plug-in
;http://nsis.sourceforge.net/AccessControl_plug-in

!define APP_NAME      "TrayCD"
!define APP_VERSION   "1.3"
!define APP_URL       "http://code.google.com/p/traycd/"
!define APP_UPDATEURL "http://traycd.googlecode.com/svn/wiki/latest-stable.txt"

;Libraries

!include "MUI2.nsh"
!include "Sections.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

; General

Name "${APP_NAME} ${APP_VERSION}"
OutFile "build\${APP_NAME}-${APP_VERSION}.exe"
InstallDir "$PROGRAMFILES\${APP_NAME}"
InstallDirRegKey HKCU "Software\${APP_NAME}" "Install_Dir"
RequestExecutionLevel admin
ShowInstDetails hide
ShowUninstDetails show
SetCompressor /SOLID lzma

; Interface

!define MUI_LANGDLL_REGISTRY_ROOT HKCU
!define MUI_LANGDLL_REGISTRY_KEY "Software\${APP_NAME}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Language"

!define MUI_COMPONENTSPAGE_NODESC

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION "Launch"

; Pages

Page custom PageUpgrade PageUpgradeLeave
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipPage
!define MUI_PAGE_CUSTOMFUNCTION_SHOW HideBackButton
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

!macro Lang lang id
${If} $LANGUAGE == ${id}
	WriteINIStr "$INSTDIR\${APP_NAME}.ini" "${APP_NAME}" "Language" "${lang}"
${EndIf}
!macroend

!macro SetLang lang1 lang2 id
${If} ${lang1} == ${lang2}
	;Beautiful NSIS-way of doing $LANGUAGE = ${id}
	IntOp $LANGUAGE 0 + ${id}
${EndIf}
!macroend

; Functions

!macro CloseApp un
Function ${un}CloseApp
	;Close app if running
	FindWindow $0 "${APP_NAME}" ""
	IntCmp $0 0 done
		DetailPrint "Closing running ${APP_NAME}."
		SendMessage $0 ${WM_CLOSE} 0 0 /TIMEOUT=500
		waitloop:
			Sleep 10
			FindWindow $0 "${APP_NAME}" ""
			IntCmp $0 0 closed waitloop waitloop
	closed:
	Sleep 100 ;Sleep a little extra to let Windows do its thing
	done:
FunctionEnd
!macroend
!insertmacro CloseApp ""
!insertmacro CloseApp "un."

; Installer

Section "" sec_update
	NSISdl::download /TIMEOUT=5000 "${APP_UPDATEURL}" "$TEMP\${APP_NAME}-updatecheck"
	Pop $0
	StrCmp $0 "success" +3
		DetailPrint "Update check failed. Error: $0."
		Goto done
	FileOpen $0 "$TEMP\${APP_NAME}-updatecheck" r
	IfErrors done
	FileRead $0 $1
	FileClose $0
	Delete /REBOOTOK "$TEMP\${APP_NAME}-updatecheck"
	
	;Make sure the response is valid
	StrCpy $3 "Version: "
	StrLen $4 $3
	StrCpy $5 $1 $4
	StrCmpS $3 $5 0 done
	
	;New version available?
	StrCpy $6 $1 "" $4
	StrCmp $6 ${APP_VERSION} done
		MessageBox MB_ICONINFORMATION|MB_YESNO "$(L10N_UPDATE_DIALOG)" /SD IDNO IDNO done
			ExecShell "open" "${APP_URL}"
			Quit
	done:
SectionEnd

Section "" sec_app
	;Close app if running
	Call CloseApp
	
	SetOutPath "$INSTDIR"
	
	;Rename old ini file if it exists
	IfFileExists "${APP_NAME}.ini" 0 +3
		Delete "${APP_NAME}-old.ini"
		Rename "${APP_NAME}.ini" "${APP_NAME}-old.ini"
	
	;Delete files that existed in earlier versions
	Delete /REBOOTOK "$INSTDIR\info.txt" ;existed in <= 1.2
	
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
	
	!insertmacro Lang "en-US" ${LANG_ENGLISH}
	!insertmacro Lang "es-ES" ${LANG_SPANISH}
	!insertmacro Lang "gl-ES" ${LANG_GALICIAN}
	!insertmacro Lang "fa-IR" ${LANG_FARSI}
	
	;Grant write rights to ini file to all users
	AccessControl::GrantOnFile "$INSTDIR\${APP_NAME}.ini" "(BU)" "FullAccess"
	
	;Update registry
	WriteRegStr HKCU "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
	WriteRegStr HKCU "Software\${APP_NAME}" "Version" "${APP_VERSION}"
	
	;Create uninstaller
	WriteUninstaller "Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" '"$INSTDIR\${APP_NAME}.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "HelpLink" "${APP_URL}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "Stefan Sundin"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
	
	;Compute size for uninstall information
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "EstimatedSize" "$0"
SectionEnd

Section "" sec_shortcut
	CreateShortCut "$SMPROGRAMS\${APP_NAME}.lnk" "$INSTDIR\${APP_NAME}.exe" "" "$INSTDIR\${APP_NAME}.exe" 0
SectionEnd

Function Launch
	Exec "$INSTDIR\${APP_NAME}.exe"
FunctionEnd

; Detect previous installation

Var Upgradebox
Var Uninstallbox

Function PageUpgrade
	IfFileExists $INSTDIR +2
		Abort
	
	nsDialogs::Create 1018
	!insertmacro MUI_HEADER_TEXT "$(L10N_UPGRADE_TITLE)" "$(L10N_UPGRADE_SUBTITLE)"
	${NSD_CreateLabel} 0 0 100% 20u "$(L10N_UPGRADE_HEADER)"
	
	${NSD_CreateRadioButton} 0 45 100% 10u "$(L10N_UPGRADE_UPGRADE)"
	Pop $Upgradebox
	${NSD_Check} $Upgradebox
	${NSD_CreateLabel} 16 62 100% 20u "$(L10N_UPGRADE_INI)"
	
	${NSD_CreateRadioButton} 0 95 100% 10u "$(L10N_UPGRADE_INSTALL)"
	
	${NSD_CreateRadioButton} 0 130 100% 10u "$(L10N_UPGRADE_UNINSTALL)"
	Pop $Uninstallbox
	
	nsDialogs::Show
FunctionEnd

Function PageUpgradeLeave
	${NSD_GetState} $Uninstallbox $0
	${If} $0 == ${BST_CHECKED}
		Exec "$INSTDIR\Uninstall.exe"
		Quit
	${EndIf}
	
	${NSD_GetState} $Upgradebox $UpgradeState
	${If} $UpgradeState == ${BST_CHECKED}
		!insertmacro UnselectSection ${sec_update}
	${EndIf}
FunctionEnd

;Used when upgrading to skip directory page
Function SkipPage
	${If} $UpgradeState == ${BST_CHECKED}
		!insertmacro UnselectSection ${sec_shortcut} ;Move to PageUpgradeLeave?
		Abort
	${EndIf}
FunctionEnd

Function HideBackButton
	GetDlgItem $0 $HWNDPARENT 3
	ShowWindow $0 ${SW_HIDE}
FunctionEnd

Function .onInit
	;Detect x64
	!ifdef x64
	${If} ${RunningX64}
		;Only set x64 installation dir if not already installed
		ReadRegStr $0 HKCU "Software\${APP_NAME}" "Install_Dir"
		IfFileExists $0 +2
			StrCpy $INSTDIR "$PROGRAMFILES64\${APP_NAME}"
	${EndIf}
	!endif
	
	;Handle silent install
	IfSilent 0 done1
		!insertmacro UnselectSection ${sec_update}
	done1:
	
	;Set language from command line
	ClearErrors
	${GetParameters} $0
	IfErrors done2
	${GetOptionsS} $0 "/L=" $0
	IfErrors done2
	!insertmacro SetLang $0 "en-US" ${LANG_ENGLISH}
	!insertmacro SetLang $0 "es-ES" ${LANG_SPANISH}
	!insertmacro SetLang $0 "gl-ES" ${LANG_GALICIAN}
	!insertmacro SetLang $0 "fa-IR" ${LANG_FARSI}
	WriteRegStr ${MUI_LANGDLL_REGISTRY_ROOT} "${MUI_LANGDLL_REGISTRY_KEY}" "${MUI_LANGDLL_REGISTRY_VALUENAME}" "$LANGUAGE"
	done2:
	
	;Display language selection
	!insertmacro MUI_LANGDLL_DISPLAY
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
	Delete /REBOOTOK "$INSTDIR\Uninstall.exe"
	RMDir  /REBOOTOK "$INSTDIR"

	Delete /REBOOTOK "$SMPROGRAMS\${APP_NAME}.lnk"

	DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}"
	DeleteRegKey /ifempty HKCU "Software\${APP_NAME}"
	DeleteRegKey /ifempty HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd
