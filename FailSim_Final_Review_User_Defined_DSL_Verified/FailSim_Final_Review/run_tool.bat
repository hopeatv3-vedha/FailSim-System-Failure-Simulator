@echo off
setlocal
cd /d "%~dp0"

REM Make the MSYS2 UCRT64 compiler discoverable when installed in its default location.
if exist "C:\msys64\ucrt64\bin\gcc.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"

where gcc >nul 2>nul
if errorlevel 1 (
  echo ERROR: GCC was not found.
  echo Install MSYS2 UCRT64 GCC or add your GCC folder to PATH.
  echo Common path: C:\msys64\ucrt64\bin\gcc.exe
  pause
  exit /b 1
)

where python >nul 2>nul
if errorlevel 1 (
  where py >nul 2>nul
  if errorlevel 1 (
    echo ERROR: Python 3 was not found.
    echo Install Python 3 from python.org and enable the PATH option.
    pause
    exit /b 1
  )
  set "PYTHON_CMD=py -3"
) else (
  set "PYTHON_CMD=python"
)

if not defined GEMINI_MODEL set "GEMINI_MODEL=gemini-3.8-flash"
if not defined GEMINI_API_KEY (
  echo NOTE: GEMINI_API_KEY is not set in this terminal session.
  echo The compiler, simulator and rule-based recovery still work; the AI advisor needs a key.
  echo Set it in PowerShell before starting: $env:GEMINI_API_KEY = "YOUR_KEY"
  echo.
)

echo Building FailSim C compiler and simulator...
gcc -std=c11 -Wall -Wextra -pedantic src\ast.c src\lexer.c src\main.c src\model.c src\parser.c src\recovery.c src\semantic.c src\simulator.c src\token.c -o failsim.exe
if errorlevel 1 (
  echo ERROR: C build failed. The server was not started.
  pause
  exit /b 1
)

echo Starting FailSim dashboard...
%PYTHON_CMD% web\app.py
endlocal
