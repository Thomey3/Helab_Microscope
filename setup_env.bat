@echo off
REM ============================================================================
REM Helab_Microscope Environment Setup Script for Windows
REM ============================================================================
REM
REM This script checks and reports the status of required SDK installations.
REM No manual configuration needed - just install the drivers!
REM
REM Required installations:
REM   1. NI-DAQmx driver from National Instruments
REM   2. CUDA Toolkit from NVIDIA
REM   3. QT_Board SDK (bundled in 3rdparty/)
REM
REM ============================================================================

echo.
echo ========================================
echo Helab_Microscope Environment Check
echo ========================================
echo.

REM Check NI-DAQmx
echo [1/3] Checking NI-DAQmx...
if exist "C:\Program Files (x86)\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include\NIDAQmx.h" (
    echo     [OK] NI-DAQmx found at: C:\Program Files (x86)\National Instruments\NI-DAQ\
    set NI_DAQMX_FOUND=1
) else if exist "C:\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include\NIDAQmx.h" (
    echo     [OK] NI-DAQmx found at: C:\Program Files\National Instruments\NI-DAQ\
    set NI_DAQMX_FOUND=1
) else (
    echo     [MISSING] NI-DAQmx not found!
    echo     Please install from: https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html
    set NI_DAQMX_FOUND=0
)

echo.

REM Check CUDA
echo [2/3] Checking CUDA Toolkit...
set CUDA_FOUND=0
for %%v in (12.6 12.4 12.2 12.0 11.8) do (
    if exist "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v%%v\include\cuda_runtime.h" (
        echo     [OK] CUDA v%%v found
        set CUDA_FOUND=1
        goto :cuda_done
    )
)
:cuda_done
if "%CUDA_FOUND%"=="0" (
    echo     [MISSING] CUDA Toolkit not found!
    echo     Please install from: https://developer.nvidia.com/cuda-downloads
)

echo.

REM Check QT_Board SDK
echo [3/3] Checking QT_Board SDK...
set SCRIPT_DIR=%~dp0
set THIRDPARTY_QT=%SCRIPT_DIR%..\..\..\..\3rdparty\QT12136DC

if exist "%THIRDPARTY_QT%\include\QT_Board.h" (
    echo     [OK] QT_Board SDK found at: %THIRDPARTY_QT%
    set QT_BOARD_FOUND=1
) else (
    echo     [MISSING] QT_Board SDK not found!
    echo     Expected location: 3rdparty\QT12136DC\
    echo     Please copy QT_Board SDK to this location.
    set QT_BOARD_FOUND=0
)

echo.
echo ========================================
echo Summary
echo ========================================

set ALL_OK=1
if "%NI_DAQMX_FOUND%"=="0" set ALL_OK=0
if "%CUDA_FOUND%"=="0" set ALL_OK=0
if "%QT_BOARD_FOUND%"=="0" set ALL_OK=0

if "%ALL_OK%"=="1" (
    echo.
    echo [SUCCESS] All dependencies found! Ready to build.
    echo.
    echo To build:
    echo   1. Open Visual Studio 2022
    echo   2. Open MMCoreAndDevices.sln
    echo   3. Build Helab_Microscope project
    echo.
) else (
    echo.
    echo [WARNING] Some dependencies are missing.
    echo Please install the missing components above.
    echo.
)

pause
