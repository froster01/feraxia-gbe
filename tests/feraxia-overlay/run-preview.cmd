@echo off
setlocal
pushd "%~dp0\..\.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FERAXIA_VS=%%i"
if not defined FERAXIA_VS exit /b 1
call "%FERAXIA_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "IMGUI=build\deps\win\vs2022\ingame_overlay\deps\ImGui"
set "OUT=%TEMP%\feraxia-overlay-ui-test"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /EHsc /std:c++17 /W3 /I"%IMGUI%" /I"%IMGUI%\backends" tests\feraxia-overlay\preview_main.cpp "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" "%IMGUI%\backends\imgui_impl_win32.cpp" "%IMGUI%\backends\imgui_impl_dx11.cpp" "%IMGUI%\backends\imgui_win_shader_blobs.cpp" /Fe:"%OUT%\preview.exe" /Fo:"%OUT%\\" /link d3d11.lib dxgi.lib d3dcompiler.lib user32.lib gdi32.lib dwmapi.lib
if errorlevel 1 exit /b 1
"%OUT%\preview.exe" %*

