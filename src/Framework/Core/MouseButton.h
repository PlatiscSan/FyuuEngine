#ifndef MOUSE_BUTTON_H
#define MOUSE_BUTTON_H

#include <cstddef>
#include <cstdint>

namespace Fyuu::core {

	using MouseButton = std::uint32_t;

}

#define FYUU_BUTTON_LEFT     1
#define FYUU_BUTTON_MIDDLE   2
#define FYUU_BUTTON_RIGHT    3
#define FYUU_BUTTON_X1       4
#define FYUU_BUTTON_X2       5

#define FYUU_BUTTON(X)       (1u << ((X)-1))
#define FYUU_BUTTON_LMASK    FYUU_BUTTON(FYUU_BUTTON_LEFT)
#define FYUU_BUTTON_MMASK    FYUU_BUTTON(FYUU_BUTTON_MIDDLE)
#define FYUU_BUTTON_RMASK    FYUU_BUTTON(FYUU_BUTTON_RIGHT)
#define FYUU_BUTTON_X1MASK   FYUU_BUTTON(FYUU_BUTTON_X1)
#define FYUU_BUTTON_X2MASK   FYUU_BUTTON(FYUU_BUTTON_X2)

#endif // !MOUSE_BUTTON_H
