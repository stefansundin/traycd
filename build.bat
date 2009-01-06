@echo off

taskkill /IM TrayCD.exe

if not exist build (
	mkdir build
)

windres -o build/resources.o resources.rc

if "%1" == "all" (
	echo Building all
	
	for /D %%f in (localization/*) do (
		@echo.
		echo Building %%f
		if not exist "build/%%f/TrayCD" (
			mkdir "build\%%f\TrayCD"
		)
		copy "localization\%%f\info.txt" "build/%%f/TrayCD"
		copy "TrayCD.ini" "build/%%f/TrayCD"
		
		gcc -o "build/%%f/TrayCD/TrayCD.exe" traycd.c build/resources.o WINMM.LIB -mwindows -lshlwapi -lwininet -DL10N_FILE=\"localization/%%f/strings.h\"
		if exist "build/%%f/TrayCD/TrayCD.exe" (
			strip "build/%%f/TrayCD/TrayCD.exe"
			upx --best -qq "build/%%f/TrayCD/TrayCD.exe"
		)
	)
	
	@echo.
	echo Building installer
	makensis /V2 installer.nsi
) else (
	gcc -o TrayCD.exe traycd.c build/resources.o WINMM.LIB -mwindows -lshlwapi -lwininet -DDEBUG
	
	if "%1" == "run" (
		start TrayCD.exe
	)
)
