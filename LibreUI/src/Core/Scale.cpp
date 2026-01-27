// LibreUI/src/Core/Scale.cpp
// Scale singleton is header-only, but this file ensures the library has a compilation unit

#include <LibreUI/Scale.h>

// Scale is implemented entirely in the header as inline methods.
// This file exists to:
// 1. Verify the header compiles correctly
// 2. Provide a place for future non-inline implementations if needed