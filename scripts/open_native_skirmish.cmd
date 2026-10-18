@echo off
setlocal EnableExtensions
if "%~1"=="" (
  echo Drag your extracted Comet Crash game folder onto OPEN_SKIRMISH.cmd.
  echo Starts the native preview skirmish using your original model assets.
  pause
  exit /b 2
)
set "SKIRMISH_DATA=%~1"
if exist "%~1\data\models\care\playerShip.obj" set "SKIRMISH_DATA=%~1\data"
if exist "%~1\USRDIR\data\models\care\playerShip.obj" set "SKIRMISH_DATA=%~1\USRDIR\data"
if exist "%~1\PS3_GAME\USRDIR\data\models\care\playerShip.obj" set "SKIRMISH_DATA=%~1\PS3_GAME\USRDIR\data"
if not exist "%SKIRMISH_DATA%\models\care\playerShip.obj" (
  echo Cannot locate the original models beneath the selected folder.
  pause
  exit /b 1
)
"%~dp0comet_native.exe" --assets "%SKIRMISH_DATA%" --skirmish-config "%~dp0native-skirmish.cfg" --arena
if errorlevel 1 pause
