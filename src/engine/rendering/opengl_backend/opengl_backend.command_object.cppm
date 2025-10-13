module;
#include <glad/glad.h>

export module opengl_backend:command_object;
export import rendering;
import std;
export import circular_buffer;
export import :buffer;

namespace fyuu_engine::opengl {

	struct PSO {

	};

	struct Texture {

	};

	export struct OpenGLFrameBuffer {
		GLuint color_texture;
		GLuint depth_texture;
		GLuint stencil_texture;
		std::uint32_t width;
		std::uint32_t height;
	};

	struct OpenGLCommandObjectTraits {

		using PipelineStateObjectType = PSO;
		using OutputTargetType = OpenGLFrameBuffer;
		using BufferType = OpenGLBuffer;
		using TextureType = Texture;

	};

	using CommandBuffer = std::deque<std::function<void()>>;

	export struct OpenGLCommandBufferReady {
		CommandBuffer& buffer;
	};

	export class OpenGLCommandObject
		: public core::BaseCommandObject<OpenGLCommandObject, OpenGLCommandObjectTraits> {
		friend class core::BaseCommandObject<OpenGLCommandObject, OpenGLCommandObjectTraits>;
	private:
		util::PointerWrapper<concurrency::SyncMessageBus> m_message_bus;
		CommandBuffer m_commands;
		core::PrimitiveTopology m_primitive_topology;

		void BeginRecordingImpl();
		void EndRecordingImpl();
		void ResetImpl();
		void SetViewportImpl(core::Viewport const& viewport);
		void SetScissorRectImpl(core::Rect const& rect);
		void BindVertexBufferImpl(OpenGLBuffer const& vertex_buffer, core::VertexDesc const& desc);
		void BeginRenderPassImpl(OpenGLFrameBuffer const& frame_buffer, std::span<float> clear_value);
		void EndRenderPassImpl(OpenGLFrameBuffer const& frame_buffer);
		void DrawImpl(
			std::uint32_t index_count, std::uint32_t instance_count = 1,
			std::uint32_t start_index = 0, std::int32_t base_vertex = 0,
			std::uint32_t start_instance = 0
		);
		void SetPrimitiveTopologyImpl(core::PrimitiveTopology primitive_topology);

	public:
		OpenGLCommandObject(util::PointerWrapper<concurrency::SyncMessageBus> const& message_bus);

		template <class Command>
			requires std::constructible_from<std::function<void()>, Command>
		void Execute(Command&& command) {
			m_commands.emplace_back(std::forward<Command>(command));
		}
	};

}