@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "UE_VERSION=5.7"
set "UE_PATH=C:\Program Files\Epic Games\UE_5.7"
set "PROJECT_PATH=%SCRIPT_DIR%\GRIT.uproject"
set "RULES_DIR=%SCRIPT_DIR%\Intermediate\Build\BuildRules"
set "DUPLICATE_PLUGIN_SOURCE=%SCRIPT_DIR%\Plugins\EpicAdapter\Source\EpicAdapter\Source\EpicAdapter\EpicAdapter.Build.cs"
set "UBT_ARGS=-waitmutex -NoUBA -MaxParallelActions=4"

:START
echo Building GRIT project using UE %UE_VERSION%...

if not exist "%UE_PATH%\Engine\Build\BatchFiles\Build.bat" (
    echo ERROR: Unreal Engine %UE_VERSION% was not found at:
    echo   %UE_PATH%
    echo Update UE_PATH in BuildGRIT.bat if your engine is installed elsewhere.
    pause
    exit /b 1
)

if not exist "%PROJECT_PATH%" (
    echo ERROR: Project file not found at:
    echo   %PROJECT_PATH%
    pause
    exit /b 1
)

if exist "%DUPLICATE_PLUGIN_SOURCE%" (
    echo ERROR: Duplicate EpicAdapter source tree detected:
    echo   %DUPLICATE_PLUGIN_SOURCE%
    echo Move or remove the nested duplicate under Plugins\EpicAdapter\Source\EpicAdapter\Source before building.
    pause
    exit /b 1
)

if exist "%RULES_DIR%" (
    echo Clearing stale UnrealBuildTool rules cache...
    del /q "%RULES_DIR%\*" >nul 2>&1
)

echo Running UnrealBuildTool with local UBA disabled and max parallel actions=4 (use all logical cores on this i3)...
call "%UE_PATH%\Engine\Build\BatchFiles\Build.bat" GRITEditor Win64 Development "%PROJECT_PATH%" %UBT_ARGS%
set "BUILD_EXIT=%ERRORLEVEL%"

if "%BUILD_EXIT%"=="0" (
    echo Build succeeded!
) else (
    echo Build failed with error code %BUILD_EXIT%!
)

if defined CODEX_NONINTERACTIVE (
    exit /b %BUILD_EXIT%
)

echo.
echo Press 'R' to run the build again or any other key to exit.
set /p userInput=Enter your choice:

if /I "%userInput%"=="R" goto START

echo Exiting...
pause
exit /b %BUILD_EXIT%
