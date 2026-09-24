@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "HERE=%~dp0"
set "BUNDLE=%HERE%CometCrashPC-source.bundle"
set "SRC=%HERE%CometCrashPC-src"

if "%~1"=="" (
  echo.
  echo Comet Crash PC Windows Builder
  echo.
  echo Drag your extracted NPEB00142 v1.00 game folder onto BUILD_AND_RUN.cmd,
  echo or run:
  echo.
  echo   BUILD_AND_RUN.cmd "D:\Games\Comet Crash"
  echo.
  pause
  exit /b 2
)

if not exist "%BUNDLE%" (
  echo ERROR: source bundle missing: "%BUNDLE%"
  pause
  exit /b 2
)

where git.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Git for Windows is required before this builder can unpack the source.
  echo Install Git, then run this file again.
  pause
  exit /b 3
)

if not exist "%SRC%\.git" (
  echo [builder] Creating exact source checkout...
  git clone "%BUNDLE%" "%SRC%"
  if errorlevel 1 (
    echo ERROR: could not create source checkout.
    pause
    exit /b !errorlevel!
  )
) else (
  echo [builder] Existing source checkout found at:
  echo           "%SRC%"
  echo [builder] Reusing it. Delete that folder if you want a fresh checkout.
)

echo.
echo [builder] Building and launching the REAL native Comet Crash executable
echo           from your local game data...
echo.
call "%SRC%\scripts\build_and_run.cmd" "%~f1" %2
set "RC=%ERRORLEVEL%"

echo.
if exist "%SRC%\build\CometCrashPC.exe" (
  echo [builder] Real native EXE:
  echo           "%SRC%\build\CometCrashPC.exe"
)
if exist "%SRC%\logs" (
  echo [builder] Boot logs:
  echo           "%SRC%\logs"
)
echo.
pause
exit /b %RC%
