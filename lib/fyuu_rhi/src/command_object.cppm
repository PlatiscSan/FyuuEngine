export module fyuu_rhi:command_object;
import std;
import plastic.disable_copy;

namespace fyuu_rhi {

	namespace details {
		template <class T> concept Fence = requires (T t) {
			{ t.Wait() } -> std::same_as<void>;
			{ t.Reset() } -> std::same_as<void>;
			{ t.Signal() } -> std::same_as<void>;
		};
	}

	export struct DrawInstancedDescriptor {
		std::uint32_t instance_count;
		std::uint32_t start_vertex;
		std::uint32_t first_instance;
	};

	export struct DrawIndexedInstancedDescriptor {
		std::uint32_t instance_count;
		std::uint32_t start_vertex;
		std::uint32_t first_index;
		std::uint32_t first_instance;
	};

	export struct VertexBufferBindingDescriptor {
		std::uint32_t binding_position;
		std::uint32_t offset_into_buffer;
	};

	export struct Viewport {
		std::int32_t x;
		std::int32_t y;
		std::uint32_t width;
		std::uint32_t height;
		std::uint32_t min_depth;
		std::uint32_t max_depth;
	};

	export struct Rect {
		std::array<std::int32_t, 2> offset;
		std::array<std::uint32_t, 2> extent;
	};

	export class ICommandBuffer 
		: public plastic::utility::DisableCopy<ICommandBuffer> {
	public:
	
	};

	export class ICommandAllocator
		: public plastic::utility::DisableCopy<ICommandAllocator> {
	public:
		auto AllocateBuffer(this auto&& self) {
			return self.AllocateBufferImpl();
		}
	};

	export enum class CommandObjectType : std::uint8_t {
		Unknown,
		AllCommands, 
		Compute,
		Copy, 
	};

	export enum class QueuePriority : std::uint8_t {
		Unknown,
		High,
		Medium,
		Low
	};

	export class ICommandQueue 
		: public plastic::utility::DisableCopy<ICommandQueue> {
	public:
		void WaitUntilCompleted(this auto&& self) {
			self.WaitUntilCompletedImpl();
		}

		template <class CommandBuffer>
		auto ExecuteCommand(this auto&& self, CommandBuffer&& command_buffer) {
			return self.ExecuteCommandImpl(std::forward<CommandBuffer>(command_buffer));
		}

	};

}