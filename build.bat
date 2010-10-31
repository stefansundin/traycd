@echo off

:: For traditional MinGW, set prefix32 to empty string
:: For mingw-w32, set prefix32 to i686-w64-mingw32-
:: For mingw-w64, set prefix64 to x86_64-w64-mingw32-

set prefix32=i686-w64-mingw32-
set prefix64=x86_64-w64-mingw32-
set l10n=en-US es-ES gl-ES fa-IR

taskkill /IM TrayCD.exe

if not exist build. mkdir build

if "%1" == "all" (
	%prefix32%windres -o build\traycd.o include\traycd.rc
	%prefix32%gcc -o build\ini.exe include\ini.c -lshlwapi
	
	@echo.
	echo Building binaries
	%prefix32%gcc -o "build\TrayCD.exe" traycd.c build\traycd.o include\winmm.lib -mwindows -lshlwapi -lwininet -O2 -s
	if not exist "build\TrayCD.exe". exit /b
	
	if "%2" == "x64" (
		if not exist "build\x64". mkdir "build\x64"
		%prefix64%windres -o build\x64\traycd.o include\traycd.rc
		%prefix64%gcc -o "build\x64\TrayCD.exe" traycd.c build\x64\traycd.o include\winmm_x64.lib -mwindows -lshlwapi -lwininet -O2 -s
		if not exist "build\x64\TrayCD.exe". exit /b
	)
	
	for %%f in (%l10n%) do (
		@echo.
		echo Putting together %%f
		if not exist "build\%%f\TrayCD". mkdir "build\%%f\TrayCD"
		copy "build\TrayCD.exe" "build\%%f\TrayCD"
		copy "localization\%%f\info.txt" "build\%%f\TrayCD"
		copy "TrayCD.ini" "build\%%f\TrayCD"
		"build\ini.exe" "build\%%f\TrayCD\TrayCD.ini" TrayCD Language %%f
		if "%2" == "x64" (
			if not exist "build\x64\%%f\TrayCD". mkdir "build\x64\%%f\TrayCD"
			copy "build\x64\TrayCD.exe" "build\x64\%%f\TrayCD"
			copy "build\%%f\TrayCD\info.txt" "build\x64\%%f\TrayCD"
			copy "build\%%f\TrayCD\TrayCD.ini" "build\x64\%%f\TrayCD"
		)
	)
	
	@echo.
	echo Building installer
	if "%2" == "x64" (
		makensis /V2 /Dx64 installer.nsi
	) else (
		makensis /V2 installer.nsi
	)
) else if "%1" == "lock" (
	%prefix32%gcc -o lock.exe lock.c -mconsole -O2 -s
) else if "%1" == "x64" (
	if not exist "build\x64". mkdir "build\x64"
	%prefix64%windres -o build\x64\traycd.o include\traycd.rc
	%prefix64%gcc -o TrayCD.exe traycd.c build\x64\traycd.o include\winmm_x64.lib -mwindows -lshlwapi -lwininet -g -DDEBUG
) else (
	%prefix32%windres -o build\traycd.o include\traycd.rc
	%prefix32%gcc -o TrayCD.exe traycd.c build\traycd.o include\winmm.lib -mwindows -lshlwapi -lwininet -g -DDEBUG
	
	if "%1" == "run" (
		start TrayCD.exe
	)
)
