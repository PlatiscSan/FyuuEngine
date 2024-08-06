
#include "pch.h"
#include "AppTimer.h"

using namespace Fyuu;

static std::mutex s_singleton_mutex;
static std::unique_ptr<AppTimer> s_instance = nullptr;

void Fyuu::AppTimer::Tick() noexcept {

    ++m_frame_count;
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - m_last_time;
    if (elapsed.count() >= 1.0) {
        m_fps = m_frame_count.load();
        m_frame_count = 0;
        m_last_time = now;
    }

}

AppTimer& Fyuu::AppTimer::GetInstance() {

	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (!s_instance) {
		try {
			s_instance.reset(new AppTimer());
		}
		catch (std::exception const&) {
			throw std::runtime_error("Failed to initialize app timer");
		}
	}
	return *s_instance.get();

}

void Fyuu::AppTimer::DestroyInstance() {

	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (s_instance) {
		s_instance.reset();
	}

}
