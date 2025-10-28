@echo off
REM Simple RD Script Runner
REM Usage: run_rd.bat <script_file.rd>

setlocal

REM Check if script file is provided
if "%1"=="" (
    echo Usage: rd.bat ^<script_file.rd^>
    echo Example: rd.bat test.rd
    exit /b 1
)

REM Check if script file exists
if not exist "%1" (
    echo Error: Script file "%1" not found!
    exit /b 1
)

REM Check if build directory exists, if not create it
if not exist "build" (
    mkdir build >nul 2>&1
    cd build
    cmake .. >nul 2>&1
    cmake --build . >nul 2>&1
    cd ..
) else (
    REM Check if executable exists, if not build it
    if not exist "build\Debug\rubberduck.exe" (
        cd build
        cmake .. >nul 2>&1
        cmake --build . >nul 2>&1
        cd ..
    )
)

REM Run the RD Script interpreter with the provided script
build\Debug\rubberduck.exe %1

endlocal