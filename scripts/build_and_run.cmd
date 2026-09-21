@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0.."
cd /d "%ROOT%"

if "%~1"=="" (
  echo Usage: scripts\build_and_run.cmd ^<extracted Comet Crash game folder^>
  echo Example: scripts\build_and_run.cmd "D:\Games\Comet Crash"
  exit /b 2
)
set "GAME=%~f1"
if not exist "%GAME%" (
  echo ERROR: game folder does not exist: "%GAME%"
  exit /b 2
)

rem Accept either the installed-title root ^(PARAM.SFO + USRDIR^) or a PS3_GAME parent.
set "TITLE_ROOT=%GAME%"
if exist "%GAME%\PS3_GAME\PARAM.SFO" set "TITLE_ROOT=%GAME%\PS3_GAME"
set "EBOOT=%TITLE_ROOT%\USRDIR\EBOOT.BIN"
if not exist "%TITLE_ROOT%\PARAM.SFO" (
  echo ERROR: PARAM.SFO not found in "%TITLE_ROOT%"
  exit /b 2
)
if not exist "%EBOOT%" (
  echo ERROR: EBOOT.BIN not found at "%EBOOT%"
  exit /b 2
)

rem Enter a VS x64 developer environment automatically when this is a normal shell.
where cl.exe >nul 2>nul
if errorlevel 1 (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if not exist "!VSWHERE!" (
    echo ERROR: Visual Studio 2022 Build Tools are required ^(Desktop C++ + Clang tools + Windows SDK^).
    exit /b 3
  )
  for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
  if not defined VSROOT (
    echo ERROR: Visual Studio C++ tools were not found.
    exit /b 3
  )
  call "!VSROOT!\VC\Auxiliary\Build\vcvars64.bat" >nul
  if errorlevel 1 exit /b !errorlevel!
)

where py.exe >nul 2>nul || (echo ERROR: Python launcher ^(py.exe^) not found. & exit /b 3)
where git.exe >nul 2>nul || (echo ERROR: Git not found. & exit /b 3)
where cmake.exe >nul 2>nul || (echo ERROR: CMake not found. & exit /b 3)
where clang-cl.exe >nul 2>nul || (echo ERROR: clang-cl not found. Add the Visual Studio C++ Clang tools component. & exit /b 3)
where ninja.exe >nul 2>nul
if errorlevel 1 (
  echo [setup] Ninja not found; installing it for this Python...
  py -3 -m pip install ninja || exit /b !errorlevel!
)

if not exist "%ROOT%\external\ps3recomp\.git" (
  echo [1/7] Fetching pinned ps3recomp toolchain...
  call "%ROOT%\scripts\bootstrap.cmd" || exit /b !errorlevel!
) else (
  echo [1/7] Verifying pinned ps3recomp toolchain...
  py -3 "%ROOT%\tools\bootstrap_ps3recomp.py" --skip-deps || exit /b !errorlevel!
)

if not exist "%ROOT%\work" mkdir "%ROOT%\work"
set "ELF=%ROOT%\work\EBOOT.ELF"

echo [2/7] Decrypting FREE-NPDRM EBOOT...
py -3 "%ROOT%\tools\comet_port.py" decrypt "%EBOOT%" -o "%ELF%" || exit /b !errorlevel!

echo [3/7] Validating NPEB00142 input...
py -3 "%ROOT%\tools\comet_port.py" validate "%TITLE_ROOT%" --elf "%ELF%" || exit /b !errorlevel!

echo [4/7] Analysing PPU imports/functions and embedded SPUs...
py -3 "%ROOT%\tools\comet_port.py" analyze "%TITLE_ROOT%" "%ELF%" || exit /b !errorlevel!

echo [5/7] Lifting PPU + SPU code and running compatibility gates...
py -3 "%ROOT%\tools\comet_port.py" lift "%ELF%" --clean || exit /b !errorlevel!

echo [6/7] Building native Windows runner...
py -3 "%ROOT%\tools\comet_port.py" build || exit /b !errorlevel!

echo [7/7] Starting Comet Crash PC...
echo        F1 = Graphics/Input menu
py -3 "%ROOT%\tools\comet_port.py" run "%TITLE_ROOT%" "%ELF%"
set "RC=%ERRORLEVEL%"
echo.
echo Comet Crash PC exited with code %RC%.
exit /b %RC%
