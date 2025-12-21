@echo off
REM RubberDuck RD Script Runner / Builder

setlocal

REM === Build if needed ===
if not exist "build" (
    echo [INFO] Build directory not found. Building...
    mkdir build >nul 2>&1
    cd build
    cmake .. || exit /b 1
    cmake --build . || exit /b 1
    cd ..
) else (
    if not exist "build\Debug\rubberduck.exe" (
        echo [INFO] Executable not found. Building...
        cd build
        cmake .. || exit /b 1
        cmake --build . || exit /b 1
        cd ..
    )
)

REM === If no RD file provided, stop after build ===
if "%1"=="" (
    echo [INFO] Build completed. No script provided to run.
    exit /b 0
)

REM === If RD file provided, check existence ===
if not exist "%1" (
    echo [ERROR] Script file "%1" not found!
    exit /b 1
)

REM === Run the RD script ===
echo [INFO] Running script: %1
build\Debug\rubberduck.exe "%1"

endlocal