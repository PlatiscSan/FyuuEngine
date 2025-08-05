module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <Windows.h>
#include <wrl/client.h>
#endif

export module imgui_layer:d3d12;
export import graphics;
export import layer;
import std;

#ifdef WIN32
export import <imgui_impl_win32.h>;
export import <imgui_impl_dx12.h>;
#endif

export import window;
export import graphics;

namespace ui::imgui::api::d3d12 {
#ifdef WIN32
	export class D3D12ImGUILayer : public core::ILayer {
	private:
		platform::Win32Window* m_window;
		graphics::api::d3d12::D3D12RenderDevice* m_device;
		boost::uuids::uuid m_layer_uuid;
		ImVec4 m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		bool m_show_demo_window = true;
		bool m_show_another_window = false;

		static boost::uuids::uuid InitializeImGUI(
			D3D12ImGUILayer* layer,
			platform::Win32Window* window,
			graphics::api::d3d12::D3D12RenderDevice* device
		);

	public:
		D3D12ImGUILayer(platform::Win32Window& window, graphics::api::d3d12::D3D12RenderDevice& device);
		~D3D12ImGUILayer() noexcept override;

		void SetClearColor(float r, float g, float b, float a);

		void BeginFrame() override;
		void EndFrame() override;

		void Update() override;
		void Render() override;

	};
#endif // WIN32
}