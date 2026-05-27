PATH=D:\bin\Portable\mingw64-8.1.0\bin;%PATH%
set GIT_TAG=
for /f "tokens=*" %%i in ('git describe --tags --abbrev^=0 2^>nul') do set GIT_TAG=%%i

if "%GIT_TAG%"=="" (
    set VERSION_DEF=
) else (
    set VERSION_DEF=-DVERSION_TAG="\"%GIT_TAG%\""
)

gcc Customization.c -o Customization.exe %VERSION_DEF% -Os -s -ffunction-sections -fdata-sections -Wl,--gc-sections -I"../sdk/include" -L"../sdk" -liup -lmsvcrt_compat -lgdi32 -luser32 -lcomctl32 -lole32 -luuid -lcomdlg32 -mwindows
pause