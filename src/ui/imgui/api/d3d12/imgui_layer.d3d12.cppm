module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <Windows.h>
#include <wrl/client.h>
#endif

export module imgui_layer:d3d12;
export import :base;
import std;

#ifdef WIN32
export import <imgui_impl_win32.h>;
export import <imgui_impl_dx12.h>;
#endif

export import window;

namespace ui::imgui::api::d3d12 {
#ifdef WIN32
	export class D3D12IMGUILayer : public BaseIMGUILayer {
	private:
		graphics::api::d3d12::DescriptorResourcePool m_descriptor_pool;
		std::vector<graphics::api::d3d12::DescriptorResourcePool::UniqueDescriptorHandle> m_allocated_handle;
		boost::uuids::uuid m_layer_uuid;


		static boost::uuids::uuid InitializeImGUI(
			platform::Win32Window* window,
			D3D12IMGUILayer* imgui_layer
		);

	public:
		D3D12IMGUILayer(
			platform::IWindow& window,
			graphics::BaseRenderDevice& device,
			ImVec4 const& clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f)
		);

		~D3D12IMGUILayer() noexcept override;

		void BeginFrame() override;
		void Render() override;


	};
#endif // WIN32
}