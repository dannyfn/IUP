PATH=D:\bin\Portable\mingw64-8.1.0\bin;%PATH%
gcc Customization.c -o Customization.exe -Os -s -ffunction-sections -fdata-sections -Wl,--gc-sections -I"../sdk/include" -L"../sdk" -liup -lmsvcrt_compat -lgdi32 -luser32 -lcomctl32 -lole32 -luuid -lcomdlg32 -mwindows
pause