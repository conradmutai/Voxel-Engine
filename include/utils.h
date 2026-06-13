#pragma once

#include <string>
#include <libgen.h>
#include <mach-o/dyld.h>

inline std::string getExecutableDir() {
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::string(dirname(path)) + "/";
    }
    return "./";
}
