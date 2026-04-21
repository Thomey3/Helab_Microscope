/**
 * @file Platform.h
 * @brief Platform abstraction layer for cross-platform support
 *
 * This header provides platform-specific definitions and utilities
 * to enable the device adapter to work on Windows, Linux, and macOS.
 */

#pragma once

// ============================================================================
// Platform Detection
// ============================================================================
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_LINUX 0
    #define PLATFORM_MACOS 0
#elif defined(__linux__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX 1
    #define PLATFORM_MACOS 0
#elif defined(__APPLE__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX 0
    #define PLATFORM_MACOS 1
#else
    #error "Unsupported platform"
#endif

// ============================================================================
// Export/Import Macros for Shared Library
// ============================================================================
#if PLATFORM_WINDOWS
    #ifdef HELAB_EXPORTS
        #define HELAB_API __declspec(dllexport)
    #else
        #define HELAB_API __declspec(dllimport)
    #endif
#else
    #define HELAB_API __attribute__((visibility("default")))
#endif

// ============================================================================
// Platform-Specific Headers
// ============================================================================
#if PLATFORM_WINDOWS
    #include <windows.h>
    #include <io.h>
    #define PATH_SEPARATOR '\\'
    #define PATH_SEPARATOR_STR "\\"
#else
    #include <unistd.h>
    #include <dlfcn.h>
    #define PATH_SEPARATOR '/'
    #define PATH_SEPARATOR_STR "/"
#endif

// ============================================================================
// Thread-Safe String Functions
// ============================================================================
#if PLATFORM_WINDOWS
    #define STRCPY_SAFE(dst, size, src) strcpy_s(dst, size, src)
    #define STRNCPY_SAFE(dst, size, src, count) strncpy_s(dst, size, src, count)
    #define SPRINTF_SAFE(dst, size, fmt, ...) sprintf_s(dst, size, fmt, __VA_ARGS__)
#else
    #define STRCPY_SAFE(dst, size, src) strncpy(dst, src, size)
    #define STRNCPY_SAFE(dst, size, src, count) strncpy(dst, src, count)
    #define SPRINTF_SAFE(dst, size, fmt, ...) snprintf(dst, size, fmt, __VA_ARGS__)
#endif

// ============================================================================
// Sleep Functions
// ============================================================================
namespace Platform {

/**
 * @brief Sleep for specified milliseconds
 */
inline void SleepMs(unsigned int milliseconds) {
#if PLATFORM_WINDOWS
    ::Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

/**
 * @brief Sleep for specified microseconds
 */
inline void SleepUs(unsigned int microseconds) {
#if PLATFORM_WINDOWS
    ::Sleep(microseconds / 1000);
#else
    usleep(microseconds);
#endif
}

/**
 * @brief Get current timestamp in milliseconds
 */
inline unsigned long long GetTimestampMs() {
#if PLATFORM_WINDOWS
    return GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

/**
 * @brief Get current timestamp in microseconds
 */
inline unsigned long long GetTimestampUs() {
#if PLATFORM_WINDOWS
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (unsigned long long)(counter.QuadPart * 1000000.0 / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#endif
}

} // namespace Platform

// ============================================================================
// NI-DAQmx Platform Abstraction
// ============================================================================
#ifdef USE_NI_DAQMX
    // NI-DAQmx is available on Windows and Linux
    #if PLATFORM_WINDOWS
        // Windows NI-DAQmx uses NIDAQmx.h directly
        #include <NIDAQmx.h>
    #elif PLATFORM_LINUX
        // Linux NI-DAQmx Base
        #include <NIDAQmx.h>
    #endif
#else
    // Stub definitions when NI-DAQmx is not available
    typedef int int32;
    typedef unsigned int uInt32;
    typedef unsigned long long uInt64;
    typedef double float64;
    typedef void* TaskHandle;

    #define DAQmxFailed(err) ((err) < 0)
    #define DAQmxSuccess(err) ((err) == 0)
#endif

// ============================================================================
// CUDA Platform Abstraction
// ============================================================================
#ifdef USE_CUDA
    #include <cuda_runtime.h>
    #include <device_launch_parameters.h>
#else
    // Stub CUDA types when CUDA is not available
    #define cudaError_t int
    #define cudaSuccess 0
#endif
