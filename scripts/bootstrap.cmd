@echo off
setlocal
set "ROOT=%~dp0.."
py -3 "%ROOT%\tools\bootstrap_ps3recomp.py"
if errorlevel 1 exit /b %ERRORLEVEL%
py -3 "%ROOT%\tools\check_env.py" --ps3recomp "%ROOT%\external\ps3recomp"
exit /b %ERRORLEVEL%
