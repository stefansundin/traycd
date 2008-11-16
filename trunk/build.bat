
taskkill /IM TrayCD.exe

windres -o resources.o resources.rc
gcc -o TrayCD traycd.c resources.o WINMM.LIB -mwindows

strip TrayCD.exe

upx --best -qq TrayCD.exe
