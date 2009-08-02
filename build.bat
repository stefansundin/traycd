@echo off

taskkill /IM TrayCD.exe

if not exist build (
	mkdir build
)

windres -o build\resources.o include\resources.rc

if "%1" == "all" (
	gcc -o build\ini.exe include\ini.c -lshlwapi
	
	@echo.
	echo Building binaries
	if not exist "build\en-US\TrayCD" (
		mkdir "build\en-US\TrayCD"
	)
	gcc -o "build\en-US\TrayCD\TrayCD.exe" traycd.c build\resources.o include\WINMM.LIB -mwindows -lshlwapi -lwininet
	if not exist "build\en-US\TrayCD\TrayCD.exe" (
		exit /b
	)
	strip "build\en-US\TrayCD\TrayCD.exe"
	
	for /D %%f in (localization/*) do (
		@echo.
		echo Putting together %%f
		if not %%f == en-US (
			if not exist "build\%%f\TrayCD" (
				mkdir "build\%%f\TrayCD"
			)
			copy "build\en-US\TrayCD\TrayCD.exe" "build\%%f\TrayCD"
		)
		copy "localization\%%f\info.txt" "build\%%f\TrayCD"
		copy TrayCD.ini "build\%%f\TrayCD"
		"build\ini.exe" "build\%%f\TrayCD\TrayCD.ini" TrayCD Language %%f
	)
	
	@echo.
	echo Building installer
	makensis /V2 installer.nsi
) else (
	gcc -o TrayCD.exe traycd.c build\resources.o include\WINMM.LIB -mwindows -lshlwapi -lwininet -DDEBUG
	
	if "%1" == "run" (
		start TrayCD.exe
	)
)
