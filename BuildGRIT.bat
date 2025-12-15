:: BuildGRIT.bat
@echo off
setlocal

set UE_PATH=C:\Program Files\UE_5.7
set PROJECT_PATH=C:\Users\brahm\Documents\Unreal Projects\GRIT\GRIT.uproject

:START
echo Building GRIT project using UE 5.7...

"%UE_PATH%\Engine\Build\BatchFiles\Build.bat" GRITEditor Win64 Development "%PROJECT_PATH%" -waitmutex

if %ERRORLEVEL% EQU 0 (
    echo Build succeeded!
) else (
    echo Build failed with error code %ERRORLEVEL%!
)

echo.
echo Press 'R' to run the build again or any other key to exit.
set /p userInput=Enter your choice: 

if /I "%userInput%"=="R" (
    goto START
) else (
    echo Exiting...
    pause
    exit /b
)