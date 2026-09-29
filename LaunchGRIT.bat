:: LaunchGRIT.bat - Updated paths for new system
@echo off
setlocal

set UE_PATH=C:\Program Files\Epic Games\UE_5.7
set PROJECT_PATH=C:\Users\OS\Documents\Unreal Projects\GRIT\GRIT.uproject

echo Launching Unreal Editor 5.7 with DX12 support...

REM Check if UE5 exists
if not exist "%UE_PATH%\Engine\Binaries\Win64\UnrealEditor.exe" (
    echo ERROR: UE5.7 Editor not found at %UE_PATH%
    echo Please verify your UE5 installation path.
    pause
    exit /b 1
)

REM Check if project exists
if not exist "%PROJECT_PATH%" (
    echo ERROR: Project file not found at %PROJECT_PATH%
    echo Please verify your project path.
    pause
    exit /b 1
)

start "" "%UE_PATH%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_PATH%" -dx12

endlocal