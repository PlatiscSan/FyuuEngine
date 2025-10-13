export module rendering:command_object;
export import :pso;
export import disable_copy;
import std;

namespace fyuu_engine::core {

	export struct Viewport {
		float    x;
		float    y;
		float    width;
		float    height;
		float    min_depth;
		float    max_depth;
	};

	export struct Rect {
		std::int32_t x;
		std::int32_t y;
		std::uint32_t width;
		std::uint32_t height;
	};

	export enum class ResourceState : std::uint8_t {
		Common,
		VertexBuffer,
		IndexBuffer,
		OutputTarget,
		Present,
		CopySrc,
		CopyDest,
		Count,
	};

	export struct ResourceBarrierDesc {
		ResourceState before;
		ResourceState after;
	};

	export struct VertexDesc {
		std::uint32_t slot;
		std::uint32_t stride;
		std::uint32_t size;
	};

	export template <class PipelineStateObject, class OutputTarget, class Buffer, class Texture>
		struct CommandObjectTraits {

		using PipelineStateObjectType = PipelineStateObject;
		using OutputTargetType = OutputTarget;
		using BufferType = Buffer;
		using TextureType = Texture;

	};


	export template <class DerivedCommandObject, class Traits>
		class BaseCommandObject : public util::DisableCopy<BaseCommandObject<DerivedCommandObject, Traits>> {
		public:
			void BeginRecording() {
				static_cast<DerivedCommandObject*>(this)->BeginRecordingImpl();
			}

			void BeginRecording(typename Traits::PipelineStateObjectType const& pso) {
				static_cast<DerivedCommandObject*>(this)->BeginRecordingImpl(pso);
			}

			void EndRecording() {
				static_cast<DerivedCommandObject*>(this)->EndRecordingImpl();
			}

			void Reset() {
				static_cast<DerivedCommandObject*>(this)->ResetImpl();
			}

			void SetViewport(Viewport const& viewport) {
				static_cast<DerivedCommandObject*>(this)->SetViewportImpl(viewport);
			}

			void SetScissorRect(Rect const& rect) {
				static_cast<DerivedCommandObject*>(this)->SetScissorRectImpl(rect);
			}

			void Barrier(typename Traits::BufferType const& buffer, ResourceBarrierDesc const& desc) {
				static_cast<DerivedCommandObject*>(this)->BarrierImpl(buffer, desc);
			}

			void Barrier(typename Traits::OutputTargetType const& output_target, ResourceBarrierDesc const& desc) {
				static_cast<DerivedCommandObject*>(this)->BarrierImpl(output_target, desc);
			}

			void Barrier(typename Traits::TextureType const& texture, ResourceBarrierDesc const& desc) {
				static_cast<DerivedCommandObject*>(this)->BarrierImpl(texture, desc);
			}

			void BeginRenderPass(typename Traits::OutputTargetType const& output_target, std::span<float> clear_value) {

				if (clear_value.size() > 4) {
					throw std::invalid_argument("Not RGBA format");
				}

				static_cast<DerivedCommandObject*>(this)->BeginRenderPassImpl(output_target, clear_value);

			}

			void BeginRenderPass(typename Traits::OutputTargetType const& output_target, float r, float g, float b, float a) {

				std::array clear_value = { r, g, b, a };
				static_cast<DerivedCommandObject*>(this)->BeginRenderPassImpl(output_target, { clear_value.data(), clear_value.size() });

			}

			void EndRenderPass(typename Traits::OutputTargetType const& output_target) {
				static_cast<DerivedCommandObject*>(this)->EndRenderPassImpl(output_target);
			}

			void BindVertexBuffer(typename Traits::BufferType const& vertex_buffer, VertexDesc const& desc) {
				static_cast<DerivedCommandObject*>(this)->BindVertexBufferImpl(vertex_buffer, desc);
			}

			void SetPrimitiveTopology(PrimitiveTopology primitive_topology) {
				static_cast<DerivedCommandObject*>(this)->SetPrimitiveTopologyImpl(primitive_topology);
			}

			void Draw(
				std::uint32_t index_count, std::uint32_t instance_count = 1,
				std::uint32_t start_index = 0, std::int32_t base_vertex = 0,
				std::uint32_t start_instance = 0
			) {
				static_cast<DerivedCommandObject*>(this)->DrawImpl(
					index_count, instance_count, 
					start_index, base_vertex, 
					start_instance
				);
			}

			void Clear(typename Traits::OutputTargetType const& output_target, std::span<float> rgba, Rect const& rect) {
				
				if (rgba.size() > 4) {
					throw std::invalid_argument("Not RGBA format");
				}

				static_cast<DerivedCommandObject*>(this)->ClearImpl(output_target, rgba, rect);

			}

			void Clear(typename Traits::OutputTargetType const& output_target, float r, float g, float b, float a, Rect const& rect) {

				std::array clear_value = { r, g, b, a };
				static_cast<DerivedCommandObject*>(this)->ClearImpl(output_target, { clear_value.data(), clear_value.size()}, rect);

			}

			void Copy(typename Traits::BufferType const& src, typename Traits::BufferType const& dest) {
				return static_cast<DerivedCommandObject*>(this)->CopyImpl(src, dest);
			}


	};

}