@echo off
setlocal
pushd "%~dp0\..\.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FERAXIA_VS=%%i"
if not defined FERAXIA_VS exit /b 1
call "%FERAXIA_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 tests\feraxia-overlay\ping_test.cpp /Fe:"%TEMP%\feraxia-ping-test.exe" /Fo:"%TEMP%\feraxia-ping-test.obj"
if errorlevel 1 exit /b 1
"%TEMP%\feraxia-ping-test.exe"
