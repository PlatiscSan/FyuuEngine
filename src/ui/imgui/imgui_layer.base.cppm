export module imgui_layer:base;
import std;
export import graphics;
export import layer;
export import <imgui.h>;

namespace ui::imgui {
	
	export class BaseIMGUILayer : public core::ILayer {
	protected:
		platform::IWindow* m_window;
		graphics::BaseRenderDevice* m_device;
		ImVec4 m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		bool m_show_demo_window = true;
		bool m_show_another_window = false;
	
	public:
		BaseIMGUILayer(
			platform::IWindow& window, 
			graphics::BaseRenderDevice& device, 
			ImVec4 const& clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f)
		);

		virtual ~BaseIMGUILayer() noexcept override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void Update() override;
		virtual void Render() override;

	};


}