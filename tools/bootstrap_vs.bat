@echo off
setlocal

rem TOOLSDIR = <repo_root>\tools\
set "TOOLSDIR=%~dp0"
for %%I in ("%TOOLSDIR%..") do set "REPO=%%~fI"

set "PY=%TOOLSDIR%init_env.py"
set "CFG=%TOOLSDIR%env_configure.json"

where py >nul 2>nul
if %errorlevel%==0 (
  py -3 "%PY%" --os windows --mode vs --repo-root "%REPO%" --config "%CFG%" %*
  exit /b %errorlevel%
)

where python >nul 2>nul
if %errorlevel%==0 (
  python "%PY%" --os windows --mode vs --repo-root "%REPO%" --config "%CFG%" %*
  exit /b %errorlevel%
)

echo [error] Python not found.
exit /b 2
