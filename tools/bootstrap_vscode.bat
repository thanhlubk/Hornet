@echo off
setlocal

rem TOOLSDIR = <repo_root>\tools\
set "TOOLSDIR=%~dp0"

rem REPO = parent of tools\
for %%I in ("%TOOLSDIR%..") do set "REPO=%%~fI"

rem Prefer Python launcher (py), fallback to python
where py >nul 2>nul
if %errorlevel%==0 (
  py -3 "%TOOLSDIR%init_env.py" --os windows --config "%TOOLSDIR%env_configure.json" --repo-root "%REPO%" %*
  exit /b %errorlevel%
)

where python >nul 2>nul
if %errorlevel%==0 (
  python "%TOOLSDIR%init_env.py" --os windows --config "%TOOLSDIR%env_configure.json" --repo-root "%REPO%" %*
  exit /b %errorlevel%
)

echo [error] Python not found. Install Python 3.x or enable the Windows "py" launcher.
exit /b 2
