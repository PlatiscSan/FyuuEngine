export module fyuu_rhi:memory;
import std;
import plastic.disable_copy;

namespace fyuu_rhi {

	/// @brief 
	export enum class VideoMemoryType : std::uint8_t {
		Unknown,
		DeviceLocal,
		HostVisible,
		DeviceReadback
	};

	export enum class VideoMemoryUsage : std::uint8_t {
		Unknown,
		VertexBuffer,
		IndexBuffer,
		Texture1D,
		Texture2D,
		Texture3D,
		Count
	};

	export class AllocationError
		: public std::runtime_error {
	public:
		AllocationError(std::string const& msg)
			: std::runtime_error(msg) {

		}
	};

	export class IVideoMemory
		: public plastic::utility::DisableCopy<IVideoMemory> {
	public:
		auto CreateBufferHandle(this auto&& self, std::size_t size_in_bytes) {
			return self.CreateBufferHandleImpl(size_in_bytes);
		}

		auto CreateTextureHandle(this auto&& self, std::size_t width, std::size_t height, std::size_t depth) {
			return self.CreateTextureHandleImpl(width, height, depth);
		}

		VideoMemoryUsage GetUsage(this auto&& self) noexcept {
			return self.GetUsageImpl();
		}

		VideoMemoryType GetType(this auto&& self) noexcept {
			return self.GetTypeImpl();
		}

	};

}