@echo off
cd /d %~dp0..
REM Template Rename Script for Hornet
REM Usage: rename_project.bat NewProjectName

if "%1"=="" (
    echo Please provide a new project name.
    echo Usage: rename_project.bat NewProjectName
    exit /b 1
)

set OLD_NAME=Hornet
set NEW_NAME=%1

echo Renaming project from %OLD_NAME% to %NEW_NAME%...

REM List of files to process
setlocal enabledelayedexpansion
for %%F in (
    src\Hornet\CMakeLists.txt
    src\Hornet\main.cpp
    src\Hornet\MainWindow.cpp
    src\Hornet\MainWindow.h
    .vscode\launch.json
    .vscode\settings.json
    .vscode\tasks.json
    CMakeLists.txt
) do (
    if exist %%F (
        echo Processing %%F
        powershell -Command "(Get-Content %%F) -replace '!OLD_NAME!', '!NEW_NAME!' | Set-Content %%F"
    )
)

REM Rename folder
rename src\Hornet %NEW_NAME%

echo Done!