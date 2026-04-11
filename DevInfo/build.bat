@echo off
echo Compiling IUP DevInfo (static)...
gcc DevInfo.c -o DevInfo.exe ^
    -Os -s ^
    -ffunction-sections -fdata-sections -Wl,--gc-sections ^
    -I"%~dp0../sdk/include" ^
    -L"%~dp0../sdk" ^
    -liup -lmsvcrt_compat ^
    -lgdi32 -luser32 -lcomctl32 -lole32 -luuid -lcomdlg32^
    -mwindows

if %ERRORLEVEL% EQU 0 (
    echo Build OK!  -^> DevInfo.exe ^(static, no DLL needed^)
) else (
    echo Build FAILED!
)
