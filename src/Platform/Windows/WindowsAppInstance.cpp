
#include "pch.h"
#include "WindowsAppInstance.h"

static HINSTANCE const s_instance = GetModuleHandle(nullptr);

HINSTANCE const& Fyuu::core::WindowsAppInstance() noexcept {
    return s_instance;
}
