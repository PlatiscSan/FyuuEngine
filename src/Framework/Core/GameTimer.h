#ifndef GAME_TIMER_H
#define GAME_TIMER_H

#include <cstdint>

namespace Fyuu::core::performance {

    void TimerTick() noexcept;

    std::uint32_t GetFPS();

};

#endif // !GAME_TIMER_H
