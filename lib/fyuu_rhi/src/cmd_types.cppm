module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <array>
#include <memory>
#include <atomic>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:command_types;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export enum class CommandObjectType : std::uint8_t {
		Graphics,
		Compute,
		Copy,
	};

	export enum class QueuePriority : std::uint8_t {
		High,
		Medium,
		Low
	};

	export enum class LoadOp : uint8_t {
		Load,
		Clear,
		DontCare,
	};

	export enum class StoreOp : uint8_t {
		Store,
		DontCare,
	};

	export using Color = std::array<float, 4u>;

	export struct Offset2D {
		std::int32_t x;
		std::int32_t y;
	};

	export struct Extent2D {
		std::uint32_t width;
		std::uint32_t height;
	};

	export struct Rect2D {
		Offset2D offset;
		Extent2D extent;
	};

	export struct Subresource {
		std::uint32_t mip_level;
		std::uint32_t array_layer;
		std::uint32_t layer_count;
	};

	export struct Viewport {
		float x;
		float y;
		float width;
		float height;
		float min_depth;
		float max_depth;
	};

	export struct DrawInstanced {
		std::size_t vertices;
		std::size_t instances;
		std::size_t start_vertex;
		std::size_t first_instance;
	};

	export struct DrawIndexedInstanced {
		std::size_t indices;
		std::size_t instances;
		std::size_t first_index;
		std::size_t start_vertex;
		std::size_t first_instance;
	};

	export class TimelineCounter {
	private:
		std::shared_ptr<std::atomic_uint64_t> m_counter;

	public:
		explicit TimelineCounter(std::uint64_t initial_value = 0)
			: m_counter(std::make_shared<std::atomic_uint64_t>(initial_value)) {
		}

		[[nodiscard]]std::uint64_t GetNextValue() noexcept {
			return m_counter->fetch_add(1u, std::memory_order::relaxed);
		}

		std::uint64_t GetCurrentValue() const noexcept {
			return m_counter->load(std::memory_order::relaxed);
		}

	};

}