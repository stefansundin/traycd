@echo off

taskkill /IM TrayCD.exe

if not exist build (
	mkdir build
)

windres -o build/resources.o resources.rc

if "%1" == "all" (
	@echo.
	echo Building binaries
	if not exist "build/en-US/TrayCD" (
		mkdir "build\en-US\TrayCD"
	)
	gcc -o "build/en-US/TrayCD/TrayCD.exe" traycd.c build/resources.o WINMM.LIB -mwindows -lshlwapi -lwininet
	if exist "build/en-US/TrayCD/TrayCD.exe" (
		strip "build/en-US/TrayCD/TrayCD.exe"
	)
	
	for /D %%f in (localization/*) do (
		@echo.
		echo Putting together %%f
		if not %%f == en-US (
			if not exist "build/%%f/TrayCD" (
				mkdir "build\%%f\TrayCD"
			)
			copy "build\en-US\TrayCD\TrayCD.exe" "build/%%f/TrayCD"
		)
		copy "localization\%%f\info.txt" "build/%%f/TrayCD"
		copy "TrayCD.ini" "build/%%f/TrayCD"
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
