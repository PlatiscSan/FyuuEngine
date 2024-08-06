#ifndef APPLICATION_EVENT_H
#define APPLICATION_EVENT_H

#include "CommonEvent.h"

namespace Fyuu {

	class AppTickEvent final : public CommonEvent {};
	class AppUpdateEvent final : public CommonEvent {};
	class AppRenderEvent final : public CommonEvent {};

}

#endif // !APPLICATION_EVENT_H
