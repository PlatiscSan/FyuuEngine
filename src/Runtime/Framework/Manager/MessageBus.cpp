
#include "pch.h"
#include "MessageBus.h"

#include "FyuuEvent.h"

using namespace Fyuu;

FYUU_API void Fyuu_SubscribeWindowMessage(Fyuu_WindowCallback callback) {
	MessageBus::GetInstance()->Subscribe<Fyuu_WindowEvent>(callback);
}