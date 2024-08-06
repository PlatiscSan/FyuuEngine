#ifndef APP_TIMER_H
#define APP_TIMER_H

#include <atomic>
#include <chrono>

namespace Fyuu {

    class AppTimer final {

    public:

        AppTimer(AppTimer const&) = delete;
        AppTimer& operator=(AppTimer const&) = delete;
        AppTimer(AppTimer&&) = delete;
        AppTimer& operator=(AppTimer&&) = delete;

        ~AppTimer() noexcept = default;

        void Tick() noexcept;

        static AppTimer& GetInstance();
        static void DestroyInstance();

        std::uint32_t GetFPS() const noexcept { return m_fps; }

    private:

        AppTimer() :m_frame_count(0), m_fps(0), m_last_time(std::chrono::high_resolution_clock::now()) {}

        std::atomic_uint32_t m_frame_count;
        std::atomic_uint32_t m_fps;
        std::chrono::high_resolution_clock::time_point m_last_time;


    };

}

#endif // !APP_TIMER_H
