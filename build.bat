@echo off
rem Build ImageOverlay.exe with Microsoft Visual C++ (x64).
rem Run this from "x64 Native Tools Command Prompt for VS"
rem (installed with Visual Studio / Build Tools, workload "Desktop development with C++").
setlocal
cd /d "%~dp0"

where cl >nul 2>nul
if errorlevel 1 (
  echo [!] cl.exe not found. Open "x64 Native Tools Command Prompt for VS" and run build.bat again.
  exit /b 1
)

if not exist build mkdir build

rc /nologo /fo build\app.res res\app.rc || exit /b 1

cl /nologo /O2 /W3 /utf-8 /std:c11 /GS- /Gs999999 /c src\overlay.c /Fobuild\overlay.obj || exit /b 1

link /nologo /out:build\ImageOverlay.exe /subsystem:windows /entry:WinMainCRTStartup /nodefaultlib ^
  /machine:x64 /dynamicbase /nxcompat /highentropyva ^
  build\overlay.obj build\app.res ^
  kernel32.lib user32.lib gdi32.lib shell32.lib comdlg32.lib comctl32.lib ole32.lib gdiplus.lib || exit /b 1

echo.
echo Done: build\ImageOverlay.exe
