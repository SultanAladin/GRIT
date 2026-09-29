@echo off
echo Regenerating GRIT project files for UE 5.7...

REM Set the UE5 path
set UE5_PATH="C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

REM Check if UE5 exists
if not exist %UE5_PATH% (
    echo ERROR: UE5.7 not found at %UE5_PATH%
    echo Please verify your UE5 installation path.
    pause
    exit /b 1
)

REM Generate project files
echo Generating Visual Studio project files...
%UE5_PATH% -projectfiles -project="%~dp0GRIT.uproject" -game -rocket -progress

if %ERRORLEVEL% EQU 0 (
    echo Project files generated successfully!
    echo You can now open GRIT.sln in Visual Studio.
) else (
    echo ERROR: Failed to generate project files.
    echo Error code: %ERRORLEVEL%
)

pause