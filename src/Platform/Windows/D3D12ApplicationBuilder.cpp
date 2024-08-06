
#include "pch.h"
#include "D3D12ApplicationBuilder.h"
#include "Framework/Event/MessageBus.h"
#include "Framework/Core/Logger.h"
#include "Framework/Core/AppTimer.h"
#include "WindowsWindow.h"

void Fyuu::D3D12ApplicationBuilder::BuildBasicSystem() {

	Logger::GetInstance().Info("Initializd Log!");
	MessageBus::GetInstance();
	AppTimer::GetInstance();

	m_app.reset(new Application());

}

void Fyuu::D3D12ApplicationBuilder::BuildMainWindow() {

	m_app->m_window = std::make_unique<WindowsWindow>("Main Window");
	m_app->m_window->SetTitle("Fyuu Engine");
	m_app->m_window->Show();
	Logger::GetInstance().Normal("Created Window");

};

void Fyuu::D3D12ApplicationBuilder::BuildRenderer() {



}

void Fyuu::D3D12ApplicationBuilder::BuildTick() {

	m_app->m_tick = []() {

		MSG msg{};
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		MessageBus::GetInstance().Publish(AppTickEvent());

		};

	MessageBus::GetInstance().Subscribe<AppTickEvent>([](AppTickEvent& e) {AppTimer::GetInstance().Tick(); });

}
