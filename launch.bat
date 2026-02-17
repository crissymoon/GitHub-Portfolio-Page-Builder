@echo off
REM launch.bat - Windows launcher for the portfolio server
REM This script compiles serve.c and starts the server.
REM Requires either MSVC (cl.exe) or MinGW (gcc.exe) to be available.

cd /d "%~dp0"

set PORT=%1
if "%PORT%"=="" set PORT=9090
set BINARY=serve.exe

REM Check if binary needs to be built
if exist "%BINARY%" (
    REM Check if source is newer (basic: just rebuild if source changed recently)
    for %%A in (serve.c) do set SRC_DATE=%%~tA
    for %%A in (%BINARY%) do set BIN_DATE=%%~tA
)

REM Try MSVC first
where cl >nul 2>&1
if %ERRORLEVEL%==0 (
    echo Compiling serve.c with MSVC ...
    cl /nologo /O2 serve.c /Fe:%BINARY% ws2_32.lib
    if %ERRORLEVEL% neq 0 goto :compilefail
    goto :run
)

REM Try MinGW gcc
where gcc >nul 2>&1
if %ERRORLEVEL%==0 (
    echo Compiling serve.c with GCC ...
    gcc -O2 -o %BINARY% serve.c -lws2_32
    if %ERRORLEVEL% neq 0 goto :compilefail
    goto :run
)

REM Try clang
where clang >nul 2>&1
if %ERRORLEVEL%==0 (
    echo Compiling serve.c with Clang ...
    clang -O2 -o %BINARY% serve.c -lws2_32
    if %ERRORLEVEL% neq 0 goto :compilefail
    goto :run
)

REM No compiler found - check if binary already exists
if exist "%BINARY%" goto :run

echo.
echo No C compiler found (cl, gcc, or clang).
echo.
echo Install one of the following:
echo   - Visual Studio Build Tools: https://visualstudio.microsoft.com/downloads/
echo   - MinGW-w64: https://www.mingw-w64.org/
echo   - LLVM/Clang: https://releases.llvm.org/
echo.
echo Then run this script again.
pause
exit /b 1

:compilefail
echo.
echo Compilation failed. Check the errors above.
pause
exit /b 1

:run
echo.
echo Starting portfolio server on port %PORT% ...
echo.
%BINARY% %PORT%
pause
