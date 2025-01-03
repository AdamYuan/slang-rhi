#include "platform.h"

#include "assert.h"

#if SLANG_WINDOWS_FAMILY
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif SLANG_LINUX_FAMILY || SLANG_APPLE_FAMILY
#include <dlfcn.h>
#else
#error "Unsupported platform"
#endif

namespace rhi {

Result loadSharedLibrary(const char* path, SharedLibraryHandle& handleOut)
{
#if SLANG_WINDOWS_FAMILY
    handleOut = LoadLibraryA(path);
#elif SLANG_LINUX_FAMILY || SLANG_APPLE_FAMILY
    handleOut = dlopen(path, RTLD_LAZY);
#else
    SLANG_RHI_ASSERT_FAILURE("Not implemented");
#endif
    if (!handleOut)
    {
        return SLANG_FAIL;
    }
    return SLANG_OK;
}

void unloadSharedLibrary(SharedLibraryHandle handle)
{
#if SLANG_WINDOWS_FAMILY
    FreeLibrary(static_cast<HMODULE>(handle));
#elif SLANG_LINUX_FAMILY || SLANG_APPLE_FAMILY
    dlclose(handle);
#else
    SLANG_RHI_ASSERT_FAILURE("Not implemented");
#endif
}

void* findSymbolAddressByName(SharedLibraryHandle handle, char const* name)
{
#if SLANG_WINDOWS_FAMILY
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#elif SLANG_LINUX_FAMILY || SLANG_APPLE_FAMILY
    return dlsym(handle, name);
#else
    SLANG_RHI_ASSERT_FAILURE("Not implemented");
#endif
}

} // namespace rhi
