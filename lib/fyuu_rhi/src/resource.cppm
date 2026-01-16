export module fyuu_rhi:resource;
import std;
import plastic.disable_copy;
import :memory;

namespace fyuu_rhi {

	export enum class ResourceType : std::uint8_t {
		Unknown,
		VertexBuffer,
		IndexBuffer,
		Texture1D,
		Texture2D,
		Texture3D,
		Count
	};

	export VideoMemoryUsage DetermineMemoryUsageFromResourceType(ResourceType type) noexcept {
		switch (type) {
		case ResourceType::VertexBuffer:
			return VideoMemoryUsage::VertexBuffer;
		case ResourceType::IndexBuffer:
			return VideoMemoryUsage::IndexBuffer;
		case ResourceType::Texture1D:
			return VideoMemoryUsage::Texture1D;
		case ResourceType::Texture2D:
			return VideoMemoryUsage::Texture2D;
		case ResourceType::Texture3D:
			return VideoMemoryUsage::Texture3D;
		default:
			return VideoMemoryUsage::Unknown;
		}
	}

	export ResourceType DetermineResourceTypeFromMemoryUsage(VideoMemoryUsage usage) noexcept {
		switch (usage) {
		case VideoMemoryUsage::VertexBuffer:
			return ResourceType::VertexBuffer;
		case VideoMemoryUsage::IndexBuffer:
			return ResourceType::IndexBuffer;
		case VideoMemoryUsage::Texture1D:
			return ResourceType::Texture1D;
		case VideoMemoryUsage::Texture2D:
			return ResourceType::Texture2D;
		case VideoMemoryUsage::Texture3D:
			return ResourceType::Texture3D;
		default:
			return ResourceType::Unknown;
		}
	}

	export class IResource
		: public plastic::utility::DisableCopy<IResource> {
	public:
		void Release(this auto&& self) noexcept {
			self.ReleaseImpl();
		}

		template <class Handle>
		void SetMemoryHandle(this auto&& self, Handle&& handle) {
			self.SetMemoryHandleImpl(std::forward<Handle>(handle));
		}

		template <class LogicalDevice, class CommandQueue>
		void SetBufferData(
			this auto&& self, 
			LogicalDevice&& logical_device, 
			CommandQueue&& copy_queue, 
			std::span<std::byte const> raw,
			std::size_t offset = 0
		) {
			self.SetBufferDataImpl(
				std::forward<LogicalDevice>(logical_device),
				std::forward<CommandQueue>(copy_queue), 
				raw,
				offset
			);
		}

		auto Map(this auto&& self, std::uintptr_t begin, std::uintptr_t end) {
			return self.MapImpl(begin, end);
		}

	};

}