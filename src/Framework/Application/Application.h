#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <stdexcept>
#include "Framework/Core/Window.h"
#include "Framework/Renderer/GraphicsDevice.h"

namespace Fyuu {

	class Application final {

		friend class ApplicationBuilder;
		friend class D3D12ApplicationBuilder;
		friend class VulkanApplicationBuilder;
		friend class OpenGLApplicationBuilder;

	public:

		Application(Application const&) = delete;
		Application& operator=(Application const&) = delete;

		~Application() noexcept;

		void Tick() {
			if (!m_tick) {
				throw std::runtime_error("No tick function set");
			}
			m_tick();
		}

		bool IsQuit() const noexcept {
			return m_quit;
		}

		using ApplicationPtr = std::shared_ptr<Application>;

	private:

		Application();

		Window::WindowPtr m_window;
		GraphicsDevice::GraphicsDevicePtr m_device;
		std::function<void()> m_tick;
		std::atomic_bool m_quit;

	};

	class ApplicationBuilder {

	public:

		virtual ~ApplicationBuilder() noexcept = default;

		Application::ApplicationPtr const& GetApp() const noexcept { return m_app; }

		virtual void BuildBasicSystem() = 0;
		virtual void BuildMainWindow() = 0;
		virtual void BuildTick() = 0;
		virtual void BuildRenderer() = 0;

		using ApplicationBuilderPtr = std::shared_ptr<ApplicationBuilder>;

	protected:

		Application::ApplicationPtr m_app;

	};

	class ApplicationDirector final {

	public:

		ApplicationDirector(std::shared_ptr<ApplicationBuilder> const& builder) 
			:m_builder(builder) {}

		~ApplicationDirector() noexcept = default;

		void Construct() {
			m_builder->BuildBasicSystem();
			m_builder->BuildMainWindow();
			m_builder->BuildRenderer();
			m_builder->BuildTick();
		}

		Application::ApplicationPtr const& GetApp() const noexcept {
			return m_builder->GetApp();
		}

	private:

		ApplicationBuilder::ApplicationBuilderPtr m_builder;

	};


}

#endif // !APPLICATION_H
