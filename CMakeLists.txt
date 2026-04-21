cmake_minimum_required(VERSION 3.16)
project(Helab_Microscope VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Build Options
# ============================================================================
option(USE_NI_DAQMX "Use NI-DAQmx library" ON)
option(USE_CUDA "Enable CUDA image reconstruction" ON)
option(BUILD_SHARED_LIBS "Build as shared library" ON)
option(BUILD_TESTS "Build unit tests" ON)

# ============================================================================
# Third-party Directory (relative to project root)
# ============================================================================
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/../../../3rdparty"
    CACHE PATH "Path to third-party libraries")

# ============================================================================
# Platform Detection
# ============================================================================
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS)
endif()

# ============================================================================
# Find Dependencies
# ============================================================================

# Boost (required for NIDAQ code)
find_package(Boost 1.65 REQUIRED COMPONENTS)

# Threads
find_package(Threads REQUIRED)

# CUDA (optional)
if(USE_CUDA)
    # First check our bundled CUDA
    set(CUDA_INCLUDE_DIR "${THIRDPARTY_DIR}/CUDA/include"
        CACHE PATH "CUDA include directory")
    set(CUDA_LIB_DIR "${THIRDPARTY_DIR}/CUDA/lib"
        CACHE PATH "CUDA library directory")

    if(EXISTS "${CUDA_INCLUDE_DIR}/cuda_runtime.h")
        enable_language(CUDA)
        add_definitions(-DUSE_CUDA)
        message(STATUS "Using bundled CUDA: ${CUDA_INCLUDE_DIR}")
    else()
        # Try system CUDA
        find_package(CUDA)
        if(CUDA_FOUND)
            enable_language(CUDA)
            add_definitions(-DUSE_CUDA)
        else()
            message(WARNING "CUDA not found, image reconstruction will be disabled")
            set(USE_CUDA OFF)
        endif()
    endif()
endif()

# NI-DAQmx
if(USE_NI_DAQMX)
    # Check bundled NI-DAQmx first
    set(NIDAQMX_INCLUDE_DIR "${THIRDPARTY_DIR}/NationalInstruments/DAQmx/include"
        CACHE PATH "NI-DAQmx include directory")
    set(NIDAQMX_LIBRARY_DIR "${THIRDPARTY_DIR}/NationalInstruments/DAQmx/lib"
        CACHE PATH "NI-DAQmx library directory")

    if(EXISTS "${NIDAQMX_INCLUDE_DIR}/NIDAQmx.h")
        message(STATUS "Using bundled NI-DAQmx: ${NIDAQMX_INCLUDE_DIR}")
        add_definitions(-DUSE_NI_DAQMX)

        if(PLATFORM_WINDOWS)
            find_library(NIDAQMX_LIBRARY
                NAMES NIDAQmx nicaiu
                PATHS "${NIDAQMX_LIBRARY_DIR}"
                NO_DEFAULT_PATH
            )
        endif()
    else()
        # Try system NI-DAQmx
        if(PLATFORM_WINDOWS)
            find_path(NIDAQMX_INCLUDE_DIR NIDAQmx.h
                PATHS "C:/Program Files/National Instruments/NI-DAQ/DAQmx ANSI C Dev/include"
            )
            find_library(NIDAQMX_LIBRARY
                NAMES nicaiu NIDAQmx
                PATHS "C:/Program Files/National Instruments/NI-DAQ/DAQmx ANSI C Dev/lib/msvc"
            )
        elseif(PLATFORM_LINUX)
            find_path(NIDAQMX_INCLUDE_DIR NIDAQmx.h
                PATHS /usr/local/include /usr/include /opt/nidaqmx/include
            )
            find_library(NIDAQMX_LIBRARY
                NAMES nidaqmx NIDAQMX
                PATHS /usr/local/lib /usr/lib /opt/nidaqmx/lib
            )
        endif()

        if(NIDAQMX_INCLUDE_DIR)
            message(STATUS "Found NI-DAQmx: ${NIDAQMX_INCLUDE_DIR}")
            add_definitions(-DUSE_NI_DAQMX)
        else()
            message(WARNING "NI-DAQmx not found, building without DAQ support")
            set(USE_NI_DAQMX OFF)
        endif()
    endif()
endif()

# QT_Board SDK
set(QT_BOARD_INCLUDE_DIR "${THIRDPARTY_DIR}/QT12136DC/include"
    CACHE PATH "QT_Board SDK include directory")
set(QT_BOARD_LIBRARY_DIR "${THIRDPARTY_DIR}/QT12136DC/lib"
    CACHE PATH "QT_Board SDK library directory")

if(EXISTS "${QT_BOARD_INCLUDE_DIR}/QT_Board.h")
    message(STATUS "Found QT_Board SDK: ${QT_BOARD_INCLUDE_DIR}")
    if(PLATFORM_WINDOWS)
        find_library(QT_BOARD_LIBRARY
            NAMES QT_Board
            PATHS "${QT_BOARD_LIBRARY_DIR}"
            NO_DEFAULT_PATH
        )
    endif()
else()
    message(WARNING "QT_Board SDK not found at ${QT_BOARD_INCLUDE_DIR}")
endif()

# Alpao SDK (optional)
set(ALPAO_INCLUDE_DIR "${THIRDPARTY_DIR}/Alpao/include"
    CACHE PATH "Alpao SDK include directory")
set(ALPAO_LIBRARY_DIR "${THIRDPARTY_DIR}/Alpao/lib"
    CACHE PATH "Alpao SDK library directory")

if(EXISTS "${ALPAO_INCLUDE_DIR}/asdkDM.h")
    message(STATUS "Found Alpao SDK: ${ALPAO_INCLUDE_DIR}")
    if(PLATFORM_WINDOWS)
        find_library(ALPAO_LIBRARY
            NAMES ASDK
            PATHS "${ALPAO_LIBRARY_DIR}"
            NO_DEFAULT_PATH
        )
    endif()
endif()

# ============================================================================
# Micro-Manager MMDevice API
# ============================================================================
set(MMDEVICE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../MMDevice"
    CACHE PATH "Path to Micro-Manager MMDevice headers")

if(NOT EXISTS "${MMDEVICE_INCLUDE_DIR}/MMDevice.h")
    message(WARNING "MMDevice headers not found at ${MMDEVICE_INCLUDE_DIR}")
endif()

# ============================================================================
# Source Files
# ============================================================================

# Core sources (MM entry point)
set(CORE_SOURCES
    Hub.cpp
)

# New architecture sources
set(NEW_ARCH_SOURCES
    # HAL implementations
    Device/source/hal/impl/NidaqClock.cpp
    Device/source/hal/impl/GalvoScanner.cpp
    Device/source/hal/impl/QtDetector.cpp
    Device/source/hal/impl/GenericLaser.cpp

    # Application layer
    Device/source/app/MicroscopeController.cpp

    # Image pipeline
    Device/source/pipeline/ImagePipeline.cpp

    # Micro-Manager adapter
    Device/source/mmadapter/HelabDevices.cpp
)

# Legacy device sources (kept for reference/transition)
set(DEVICE_SOURCES
    Device/source/ImageBufferManager.cpp
    Device/source/DataStream.cpp
)

# NIDAQ sources (conditional)
if(USE_NI_DAQMX)
    list(APPEND DEVICE_SOURCES
        Device/source/NIDAQ.cpp
    )
endif()

# CUDA sources (conditional)
if(USE_CUDA)
    set(CUDA_SOURCES
        Device/source/ImgReconstruction.cu
    )
    set(CMAKE_CUDA_STANDARD 17)
endif()

# ============================================================================
# Include Directories
# ============================================================================
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Device/include
    ${MMDEVICE_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

if(USE_NI_DAQMX)
    include_directories(${NIDAQMX_INCLUDE_DIR})
endif()

if(EXISTS "${QT_BOARD_INCLUDE_DIR}/QT_Board.h")
    include_directories(${QT_BOARD_INCLUDE_DIR})
endif()

if(EXISTS "${ALPAO_INCLUDE_DIR}/asdkDM.h")
    include_directories(${ALPAO_INCLUDE_DIR})
endif()

if(USE_CUDA)
    include_directories(${CUDA_INCLUDE_DIR})
endif()

# ============================================================================
# Library Definition
# ============================================================================

# Create the device adapter library
if(USE_CUDA)
    add_library(Helab_Microscope SHARED
        ${CORE_SOURCES}
        ${NEW_ARCH_SOURCES}
        ${DEVICE_SOURCES}
        ${CUDA_SOURCES}
    )
else()
    add_library(Helab_Microscope SHARED
        ${CORE_SOURCES}
        ${NEW_ARCH_SOURCES}
        ${DEVICE_SOURCES}
    )
endif()

# Set output name according to Micro-Manager convention
set_target_properties(Helab_Microscope PROPERTIES
    OUTPUT_NAME "mmgr_dal_Helab_Microscope"
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    PREFIX "lib"
)

# ============================================================================
# Link Libraries
# ============================================================================
target_link_libraries(Helab_Microscope
    Threads::Threads
)

if(USE_NI_DAQMX AND NIDAQMX_LIBRARY)
    target_link_libraries(Helab_Microscope ${NIDAQMX_LIBRARY})
endif()

if(QT_BOARD_LIBRARY)
    target_link_libraries(Helab_Microscope ${QT_BOARD_LIBRARY})
endif()

if(ALPAO_LIBRARY)
    target_link_libraries(Helab_Microscope ${ALPAO_LIBRARY})
endif()

if(USE_CUDA)
    if(PLATFORM_WINDOWS AND EXISTS "${CUDA_LIB_DIR}/cudart.lib")
        target_link_libraries(Helab_Microscope "${CUDA_LIB_DIR}/cudart.lib")
    elseif(CUDA_FOUND)
        target_link_libraries(Helab_Microscope ${CUDA_LIBRARIES})
    endif()
endif()

# Platform-specific libraries
if(PLATFORM_WINDOWS)
    # Windows-specific libraries
elseif(PLATFORM_LINUX)
    target_link_libraries(Helab_Microscope dl rt)
elseif(PLATFORM_MACOS)
    # macOS-specific libraries
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS Helab_Microscope
    LIBRARY DESTINATION lib/micro-manager
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

# Install configuration files
install(DIRECTORY Device/include/config/
    DESTINATION share/micro-manager/Helab_Microscope/config
)

# ============================================================================
# Testing (optional)
# ============================================================================
if(BUILD_TESTS)
    enable_testing()

    # Find GoogleTest
    find_package(GTest QUIET)

    if(NOT GTest_FOUND)
        # Try to fetch GoogleTest if not found
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    # Test sources
    set(TEST_SOURCES
        Device/tests/app/MicroscopeControllerTest.cpp
    )

    # Create test executable
    add_executable(Helab_Microscope_Tests ${TEST_SOURCES})

    target_link_libraries(Helab_Microscope_Tests
        Helab_Microscope
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
    )

    # Register tests with CTest
    include(GoogleTest)
    gtest_discover_tests(Helab_Microscope_Tests)
endif()

# ============================================================================
# Build Information
# ============================================================================
message(STATUS "")
message(STATUS "Helab_Microscope Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  NI-DAQmx: ${USE_NI_DAQMX}")
message(STATUS "  CUDA: ${USE_CUDA}")
message(STATUS "  MMDevice: ${MMDEVICE_INCLUDE_DIR}")
message(STATUS "  Third-party: ${THIRDPARTY_DIR}")
message(STATUS "")
