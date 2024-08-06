#include "pch.h"
#include "AppInstance.h"

static HINSTANCE s_instance = GetModuleHandle(nullptr);

HINSTANCE const& Fyuu::AppInstance() noexcept {
    return s_instance;
}
