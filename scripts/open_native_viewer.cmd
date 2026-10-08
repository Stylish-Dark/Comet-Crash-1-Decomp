@echo off
setlocal EnableExtensions
if "%~1"=="" (
  echo Drag your extracted Comet Crash game folder onto OPEN_VIEWER.cmd.
  echo This is the native model viewer. Gameplay is not implemented yet.
  pause
  exit /b 2
)
set "DATA_DIR=%~1"
if exist "%~1\data\models\care\playerShip.obj" set "DATA_DIR=%~1\data"
if exist "%~1\USRDIR\data\models\care\playerShip.obj" set "DATA_DIR=%~1\USRDIR\data"
if exist "%~1\PS3_GAME\USRDIR\data\models\care\playerShip.obj" set "DATA_DIR=%~1\PS3_GAME\USRDIR\data"
if not exist "%DATA_DIR%\models\care\playerShip.obj" (
  echo Cannot locate the original models beneath the selected folder.
  pause
  exit /b 1
)
"%~dp0comet_native.exe" --assets "%DATA_DIR%" --viewer
if errorlevel 1 pause
