#ifndef APPLICATION_EVENT_H
#define APPLICATION_EVENT_H

#include "Framework/Core/MessageBus.h"

namespace Fyuu::core {

	class AppTickEvent final : public BaseEvent {};
	class AppUpdateEvent final : public BaseEvent {};
	class AppRenderEvent final : public BaseEvent {};

}

#endif // !APPLICATION_EVENT_H
