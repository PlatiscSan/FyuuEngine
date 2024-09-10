
#include "pch.h"
#include "GameTimer.h"

using namespace Fyuu;

static std::atomic_uint32_t s_frame_count = 0;
static std::atomic_uint32_t s_fps = 0;
static std::chrono::high_resolution_clock::time_point s_last_time = std::chrono::high_resolution_clock::now();

void Fyuu::core::performance::TimerTick() noexcept {

    ++s_frame_count;
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long double> elapsed = now - s_last_time;
    if (elapsed.count() >= 1.0) {
        s_fps = s_frame_count.load();
        s_frame_count = 0;
        s_last_time = now;
    }

}

std::uint32_t Fyuu::core::performance::GetFPS() {
    return s_fps;
}

