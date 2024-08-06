
#include "pch.h"
#include "Application.h"
#include "Framework/Event/MessageBus.h"
#include "Framework/Core/AppTimer.h"
#include "Framework/Core/Logger.h"

using namespace Fyuu;

Fyuu::Application::Application() {

	MessageBus::GetInstance().Subscribe<WindowDestroyEvent>(
		[&](WindowDestroyEvent& e) {
			if (e.GetWindow() == m_window.get()) {
				m_quit = true;
				Logger::GetInstance().Info("App Quit");
			}
		}
	);

}


Fyuu::Application::~Application() noexcept {

	AppTimer::DestroyInstance();
	Logger::DestroyInstance();
	MessageBus::DestroyInstance();


}
