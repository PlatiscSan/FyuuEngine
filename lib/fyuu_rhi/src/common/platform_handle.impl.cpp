module;
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // defined(__cpp_lib_modules)
#include "platform.hpp"
module fyuu_rhi:platform_handle;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;

namespace fyuu_rhi {   
#if defined(_WIN32)
    std::pair<std::size_t, std::size_t> GetSize(PlatformHandle const& handle) noexcept {
        RECT rect;
        GetClientRect(handle.impl, &rect);
        return { static_cast<std::size_t>(rect.right - rect.left),
                static_cast<std::size_t>(rect.bottom - rect.top) };
    }

#elif defined(__linux__)
	
    std::pair<std::size_t, std::size_t> GetSize(PlatformHandle const& handle) noexcept {
        return std::visit(
            [this](auto&& arg) -> std::pair<std::size_t, std::size_t> {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Xlib>) {
                    XWindowAttributes attrs;
                    XGetWindowAttributes(arg.display, arg.window, &attrs);
                    return { static_cast<std::size_t>(attrs.width),
                            static_cast<std::size_t>(attrs.height) };
                } else if constexpr (std::is_same_v<T, Wayland>) {
                    return { arg.width, arg.height };
                } else {
                    return {0, 0};
                }
            }, 
            handle.impl
        );
    }

#elif defined(__ANDROID__)
    std::pair<std::size_t, std::size_t> GetSize(PlatformHandle const& handle) noexcept {
        int width = ANativeWindow_getWidth(handle.impl);
        int height = ANativeWindow_getHeight(handle.impl);
        return {
            static_cast<std::size_t>(width),
            static_cast<std::size_t>(height)
        };
    }
#endif // defined(_WIN32)

}
