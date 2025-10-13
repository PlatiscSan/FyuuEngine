module;
#include <glad/glad.h>

module opengl_backend:command_object;

namespace fyuu_engine::opengl {

	class IDrawStrategy {
	public:
		virtual ~IDrawStrategy() = default;
		virtual void Execute() const = 0;
	};

	class StandardDrawStrategy : public IDrawStrategy {
	private:
		GLenum m_primitive;
		GLsizei m_count;
		void const* m_indices;

	public:
		StandardDrawStrategy(GLenum primitive, GLsizei count, void const* indices)
			: m_primitive(primitive), m_count(count), m_indices(indices) {
		}

		void Execute() const override {
			glDrawElements(m_primitive, m_count, GL_UNSIGNED_INT, m_indices);
		}
	};

	class BaseVertexDrawStrategy : public IDrawStrategy {
	private:
		GLenum m_primitive;
		GLsizei m_count;
		void const* m_indices;
		GLint m_base_vertex;

	public:
		BaseVertexDrawStrategy(GLenum primitive, GLsizei count, void const* indices, GLint base_vertex)
			: m_primitive(primitive), m_count(count), m_indices(indices), m_base_vertex(base_vertex) {
		}

		void Execute() const override {
			glDrawElementsBaseVertex(m_primitive, m_count, GL_UNSIGNED_INT, m_indices, m_base_vertex);
		}
	};

	class InstancedDrawStrategy : public IDrawStrategy {
	private:
		GLenum m_primitive;
		GLsizei m_count;
		void const* m_indices;
		GLsizei m_instance_count;

	public:
		InstancedDrawStrategy(GLenum primitive, GLsizei count, void const* indices, GLsizei instance_count)
			: m_primitive(primitive), m_count(count), m_indices(indices), m_instance_count(instance_count) {
		}

		void Execute() const override {
			glDrawElementsInstanced(m_primitive, m_count, GL_UNSIGNED_INT, m_indices, m_instance_count);
		}
	};

	class FullFeaturedDrawStrategy : public IDrawStrategy {
	private:
		GLenum m_primitive;
		GLsizei m_count;
		void const* m_indices;
		GLsizei m_instance_count;
		GLint m_base_vertex;
		GLuint m_base_instance;

	public:
		FullFeaturedDrawStrategy(GLenum primitive, GLsizei count, void const* indices,
			GLsizei instance_count, GLint base_vertex, GLuint base_instance)
			: m_primitive(primitive), m_count(count), m_indices(indices)
			, m_instance_count(instance_count), m_base_vertex(base_vertex), m_base_instance(base_instance) {
		}

		void Execute() const override {
			glDrawElementsInstancedBaseVertexBaseInstance(
				m_primitive, m_count, GL_UNSIGNED_INT, m_indices,
				m_instance_count, m_base_vertex, m_base_instance
			);
		}
	};

	static constexpr std::size_t StrategyBufferSize = std::max(
		{
			sizeof(StandardDrawStrategy), 
			sizeof(BaseVertexDrawStrategy), 
			sizeof(InstancedDrawStrategy),
			sizeof(FullFeaturedDrawStrategy)
		}
	);

	using StrategyBuffer = std::array<std::byte, StrategyBufferSize>;

	class DrawStrategyFactory {
	private:
		enum class StrategyType {
			Standard,
			BaseVertex,
			Instanced,
			FullFeatured
		};

		using StrategyCreator = IDrawStrategy * (*)(StrategyBuffer&, GLenum, GLsizei, void const*, GLsizei, GLint, GLuint);

		static constexpr StrategyType SelectStrategy(
			GLsizei instance_count,
			GLint base_vertex, 
			GLuint base_instance
		) {

			constexpr bool use_instancing = false;
			constexpr bool use_base_vertex = false;
			constexpr bool use_base_instance = false;

			std::uint32_t const features =
				((instance_count > 1) ? 1u : 0u) |
				((base_vertex != 0) ? 2u : 0u) |
				((base_instance != 0) ? 4u : 0u);

			constexpr StrategyType strategy_map[] = {
				StrategyType::Standard,
				StrategyType::Instanced,
				StrategyType::BaseVertex,
				StrategyType::FullFeatured,
				StrategyType::FullFeatured,
				StrategyType::FullFeatured,
				StrategyType::FullFeatured,
				StrategyType::FullFeatured
			};

			return strategy_map[features];
		}

		static IDrawStrategy* CreateStandard(
			StrategyBuffer& buffer, 
			GLenum primitive, 
			GLsizei count,
			void const* indices, 
			GLsizei instance_count,
			GLint base_vertex, 
			GLuint base_instance
		) {
			return new (buffer.data()) StandardDrawStrategy(primitive, count, indices);
		}

		static IDrawStrategy* CreateBaseVertex(
			StrategyBuffer& buffer, 
			GLenum primitive, 
			GLsizei count,
			void const* indices, 
			GLsizei instance_count,
			GLint base_vertex, 
			GLuint base_instance
		) {
			return new (buffer.data()) BaseVertexDrawStrategy(primitive, count, indices, base_vertex);
		}

		static IDrawStrategy* CreateInstanced(
			StrategyBuffer& buffer, 
			GLenum primitive, 
			GLsizei count,
			void const* indices, 
			GLsizei instance_count,
			GLint base_vertex, 
			GLuint base_instance
		) {
			return new (buffer.data()) InstancedDrawStrategy(primitive, count, indices, instance_count);
		}

		static IDrawStrategy* CreateFullFeatured(
			StrategyBuffer& buffer, 
			GLenum primitive, 
			GLsizei count,
			void const* indices,
			GLsizei instance_count,
			GLint base_vertex,
			GLuint base_instance
		) {
			return new (buffer.data()) FullFeaturedDrawStrategy(
				primitive, 
				count, 
				indices,
				instance_count, 
				base_vertex, 
				base_instance
			);
		}

		static constexpr std::array<StrategyCreator, 4> StrategyTable = {
			CreateStandard,      // StrategyType::Standard
			CreateBaseVertex,    // StrategyType::BaseVertex  
			CreateInstanced,     // StrategyType::Instanced
			CreateFullFeatured   // StrategyType::FullFeatured
		};

	public:
		static IDrawStrategy* Create(
			StrategyBuffer& buffer,
			GLenum primitive, GLsizei count, void const* indices,
			GLsizei instance_count = 1, GLint base_vertex = 0, GLuint base_instance = 0) {

			StrategyType type = SelectStrategy(instance_count, base_vertex, base_instance);

			return StrategyTable[static_cast<size_t>(type)](
				buffer, primitive, count, indices, instance_count, base_vertex, base_instance);
		}

		static void Destroy(StrategyBuffer& buffer, IDrawStrategy* strategy) {
			if (strategy) {
				strategy->~IDrawStrategy();
			}
		}
	};

	static GLenum PrimitiveTopologyToGLPrimitive(core::PrimitiveTopology topology) noexcept {
		switch (topology) {
		case core::PrimitiveTopology::PointList: return GL_POINTS;
		case core::PrimitiveTopology::LineList: return GL_LINES;
		case core::PrimitiveTopology::LineStrip: return GL_LINE_STRIP;
		case core::PrimitiveTopology::TriangleList: return GL_TRIANGLES;
		case core::PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
		//case core::PrimitiveTopology::TriangleFan: return GL_TRIANGLE_FAN;
		//case core::PrimitiveTopology::LineListAdjacency: return GL_LINES_ADJACENCY;
		//case core::PrimitiveTopology::LineStripAdjacency: return GL_LINE_STRIP_ADJACENCY;
		//case core::PrimitiveTopology::TriangleListAdjacency: return GL_TRIANGLES_ADJACENCY;
		//case core::PrimitiveTopology::TriangleStripAdjacency: return GL_TRIANGLE_STRIP_ADJACENCY;
		default: return GL_TRIANGLES;
		}
	}

	void OpenGLCommandObject::BeginRecordingImpl() {
		m_commands.clear();
	}

	void OpenGLCommandObject::EndRecordingImpl() {
		if (m_message_bus) {
			m_message_bus->Publish<OpenGLCommandBufferReady>(m_commands);
		}
	}

	void OpenGLCommandObject::ResetImpl() {
		m_commands.clear();
	}

	void OpenGLCommandObject::SetViewportImpl(core::Viewport const& viewport) {
		m_commands.emplace_back(
			[viewport]() {
				glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
			}
		);
	}

	void OpenGLCommandObject::SetScissorRectImpl(core::Rect const& rect) {
		m_commands.emplace_back(
			[rect]() {
				glScissor(rect.x, rect.y, rect.width, rect.height);
			}
		);
	}

	void OpenGLCommandObject::BindVertexBufferImpl(OpenGLBuffer const& vertex_buffer, core::VertexDesc const& desc) {
		m_commands.emplace_back(
			[vertex_buffer, desc]() {
				
			}
		);
	}

	void OpenGLCommandObject::BeginRenderPassImpl(OpenGLFrameBuffer const& frame_buffer, std::span<float> clear_value) {
		m_commands.emplace_back(
			[frame_buffer, clear_value]() {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, frame_buffer.width, frame_buffer.height);
				glClearColor(clear_value[0], clear_value[1], clear_value[2], clear_value[3]);
				glClear(GL_COLOR_BUFFER_BIT);
			}
		);
	}

	void OpenGLCommandObject::EndRenderPassImpl(OpenGLFrameBuffer const& frame_buffer) {
		m_commands.emplace_back(
			[frame_buffer]() {
				
			}
		);
	}

	void OpenGLCommandObject::DrawImpl(
		std::uint32_t index_count, std::uint32_t instance_count,
		std::uint32_t start_index, std::int32_t base_vertex,
		std::uint32_t start_instance
	) {
		m_commands.emplace_back(
			[=]() {
				std::array<std::byte, StrategyBufferSize> strategy_buffer{};
				GLenum gl_primitive = PrimitiveTopologyToGLPrimitive(m_primitive_topology);
				auto indices_offset =
					reinterpret_cast<void const*>(
						static_cast<std::uintptr_t>(start_index * sizeof(std::uint32_t))
						);

				IDrawStrategy* strategy = DrawStrategyFactory::Create(
					strategy_buffer,
					gl_primitive,
					static_cast<GLsizei>(index_count),
					indices_offset,
					static_cast<GLsizei>(instance_count),
					static_cast<GLint>(base_vertex),
					static_cast<GLuint>(start_instance)
				);

				strategy->Execute();
				DrawStrategyFactory::Destroy(strategy_buffer, strategy);

			}
		);
	}

	void OpenGLCommandObject::SetPrimitiveTopologyImpl(core::PrimitiveTopology primitive_topology) {
		m_commands.emplace_back(
			[this, primitive_topology]() {
				m_primitive_topology = primitive_topology;
			}
		);
	}

	OpenGLCommandObject::OpenGLCommandObject(util::PointerWrapper<concurrency::SyncMessageBus> const& message_bus)
		: m_message_bus(util::MakeReferred(message_bus)) {
	}

}