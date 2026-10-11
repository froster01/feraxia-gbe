@echo off
setlocal
pushd "%~dp0\..\.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FERAXIA_VS=%%i"
if not defined FERAXIA_VS exit /b 1
set "TEST_ARCH=%~1"
if not defined TEST_ARCH set "TEST_ARCH=x64"
if /i "%TEST_ARCH%"=="x64" (call "%FERAXIA_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul) else if /i "%TEST_ARCH%"=="x86" (call "%FERAXIA_VS%\VC\Auxiliary\Build\vcvars32.bat" >nul) else exit /b 2
if errorlevel 1 exit /b 1
set "IMGUI=build\deps\win\vs2022\ingame_overlay\deps\ImGui"
set "OUT=%TEMP%\feraxia-overlay-texture-test-%TEST_ARCH%"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /EHsc /std:c++17 /W3 /I. /I"%IMGUI%" tests\feraxia-overlay\texture_recovery_test.cpp "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" "%IMGUI%\backends\imgui_impl_dx11.cpp" "%IMGUI%\backends\imgui_win_shader_blobs.cpp" /Fe:"%OUT%\texture-test.exe" /Fo:"%OUT%\\" /link d3d11.lib dxgi.lib d3dcompiler.lib
if errorlevel 1 exit /b 1
"%OUT%\texture-test.exe"
exit /b %errorlevel%
