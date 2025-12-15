:: PlayGRIT.bat
@echo off
setlocal

set UE_PATH=C:\Program Files\UE_5.7
set PROJECT_PATH=C:\Users\brahm\Documents\Unreal Projects\GRIT\GRIT.uproject

echo Running GRIT in standalone game mode (UE 5.7)...
"%UE_PATH%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_PATH%" -game

echo Game session ended.
pause