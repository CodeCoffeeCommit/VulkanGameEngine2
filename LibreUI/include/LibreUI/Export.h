// LibreUI/include/LibreUI/Export.h
// DLL export/import macros for cross-platform shared library support
//
// For static library builds: LIBREUI_API expands to nothing
// For DLL builds: LIBREUI_API handles __declspec(dllexport/dllimport)
//
// Usage:
//   class LIBREUI_API Button { ... };
//   LIBREUI_API void someFunction();

#pragma once

// Detect platform
#if defined(_WIN32) || defined(_WIN64)
#define LIBREUI_PLATFORM_WINDOWS
#elif defined(__linux__)
#define LIBREUI_PLATFORM_LINUX
#elif defined(__APPLE__)
#define LIBREUI_PLATFORM_MACOS
#elif defined(__ANDROID__)
#define LIBREUI_PLATFORM_ANDROID
#endif

// Define export/import macros
#ifdef LIBREUI_SHARED
    // Building or using as shared library (DLL)
#ifdef LIBREUI_PLATFORM_WINDOWS
#ifdef LIBREUI_EXPORTS
    // Building the DLL
#define LIBREUI_API __declspec(dllexport)
#else
    // Using the DLL
#define LIBREUI_API __declspec(dllimport)
#endif
#else
    // Linux/macOS use visibility
#ifdef LIBREUI_EXPORTS
#define LIBREUI_API __attribute__((visibility("default")))
#else
#define LIBREUI_API
#endif
#endif
#else
    // Static library - no special decoration needed
#define LIBREUI_API
#endif