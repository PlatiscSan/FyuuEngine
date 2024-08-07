
#include "pch.h"
#include "D3D12ApplicationBuilder.h"
#include "Framework/Event/MessageBus.h"
#include "Framework/Core/Logger.h"
#include "Framework/Core/AppTimer.h"
#include "Framework/Core/AssetLoader.h"
#include "WindowsWindow.h"
#include "D3D12Device.h"

void Fyuu::D3D12ApplicationBuilder::BuildBasicSystem() {

	Logger::GetInstance().Info("Initializd Log!");
	MessageBus::GetInstance();
	AssetLoader::GetInstance();
	AppTimer::GetInstance();

	m_app.reset(new Application());

}

void Fyuu::D3D12ApplicationBuilder::BuildMainWindow() {

	m_app->m_window = std::make_shared<WindowsWindow>("Main Window");
	m_app->m_window->SetTitle("Fyuu Engine");
	m_app->m_window->Show();
	Logger::GetInstance().Normal("Created Window");

};

void Fyuu::D3D12ApplicationBuilder::BuildRenderer() {

	m_app->m_device = std::make_shared<D3D12Device>();

}

void Fyuu::D3D12ApplicationBuilder::BuildTick() {

	m_app->m_tick = []() {

		MSG msg{};
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		MessageBus::GetInstance().Publish(AppTickEvent());
		AppTimer::GetInstance().Tick();
		};

}
