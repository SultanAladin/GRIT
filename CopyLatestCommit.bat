@echo off
echo ========================================
echo Copying Latest Commit Files to GRIT Project
echo ========================================
echo.

set SOURCE_DIR=D:\Commit\GRIT13-01-2026
set DEST_DIR=C:\Users\OS\Documents\Unreal Projects\GRIT

REM Check if source directory exists
if not exist "%SOURCE_DIR%" (
    echo ERROR: Source directory not found at %SOURCE_DIR%
    pause
    exit /b 1
)

REM Check if destination directory exists
if not exist "%DEST_DIR%" (
    echo ERROR: Destination directory not found at %DEST_DIR%
    pause
    exit /b 1
)

echo Source: %SOURCE_DIR%
echo Destination: %DEST_DIR%
echo.
echo WARNING: This will overwrite existing files in the destination!
echo Press Ctrl+C to cancel or any key to continue...
pause > nul

echo.
echo Copying Source folder...
if exist "%SOURCE_DIR%\Source" (
    xcopy "%SOURCE_DIR%\Source\*" "%DEST_DIR%\Source\" /E /Y /I
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Source files copied successfully
    ) else (
        echo [ERROR] Failed to copy Source files
    )
) else (
    echo [SKIP] Source folder not found in commit
)

echo.
echo Copying Config folder...
if exist "%SOURCE_DIR%\Config" (
    xcopy "%SOURCE_DIR%\Config\*" "%DEST_DIR%\Config\" /E /Y /I
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Config files copied successfully
    ) else (
        echo [ERROR] Failed to copy Config files
    )
) else (
    echo [SKIP] Config folder not found in commit
)

echo.
echo Copying Content folder...
if exist "%SOURCE_DIR%\Content" (
    xcopy "%SOURCE_DIR%\Content\*" "%DEST_DIR%\Content\" /E /Y /I
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Content files copied successfully
    ) else (
        echo [ERROR] Failed to copy Content files
    )
) else (
    echo [SKIP] Content folder not found in commit
)

echo.
echo Copying Plugins folder...
if exist "%SOURCE_DIR%\Plugins" (
    xcopy "%SOURCE_DIR%\Plugins\*" "%DEST_DIR%\Plugins\" /E /Y /I
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Plugins files copied successfully
    ) else (
        echo [ERROR] Failed to copy Plugins files
    )
) else (
    echo [SKIP] Plugins folder not found in commit
)

echo.
echo Copying project file...
if exist "%SOURCE_DIR%\GRIT.uproject" (
    copy /Y "%SOURCE_DIR%\GRIT.uproject" "%DEST_DIR%\GRIT.uproject"
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Project file copied successfully
    ) else (
        echo [ERROR] Failed to copy project file
    )
) else (
    echo [SKIP] GRIT.uproject not found in commit
)

echo.
echo ========================================
echo Copy operation completed!
echo ========================================
echo.
echo IMPORTANT: You should now:
echo 1. Run RegenerateProjectFiles.bat to update Visual Studio files
echo 2. Run BuildGRIT.bat to compile the project
echo.
pause
