module fyuu_rhi:vulkan_resource;
import :vulkan_physical_device;
import :vulkan_logical_device;
import :vulkan_memory;
import :vulkan_command_object;

namespace fyuu_rhi::vulkan {

	void VulkanResource::ReleaseImpl() noexcept {
		m_handle.emplace<std::monostate>();
	}

	void VulkanResource::SetMemoryHandleImpl(UniqueVulkanBufferHandle&& handle) {
		m_handle.emplace<UniqueVulkanBufferHandle>(std::move(handle));
	}

	void VulkanResource::SetMemoryHandleImpl(UniqueVulkanTextureHandle&& handle) {
		m_handle.emplace<UniqueVulkanTextureHandle>(std::move(handle));
	}

	VulkanResource::UniqueMappedHandle VulkanResource::MapImpl(std::uintptr_t begin, std::uintptr_t end) {

		plastic::utility::AnyPointer<VulkanVideoMemory> video_memory
			= std::visit(
				[](auto&& handle) -> plastic::utility::AnyPointer<VulkanVideoMemory> {
					using Handle = std::decay_t<decltype(handle)>;
					if constexpr (!std::same_as<Handle, std::monostate>) {
						return plastic::utility::QueryObject<VulkanVideoMemory>(handle.GetGC().GetVideoMemoryID());
					}
					else {
						return nullptr;
					}
				},
				m_handle
			);

		void* ptr;
		VmaAllocator allocator = video_memory->GetAllocator();
		vmaMapMemory(allocator, video_memory->GetNative(), &ptr);

		return UniqueMappedHandle(
			MappedHandle{ reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(ptr) + begin), begin, end},
			GetID()
		);

	}

	void VulkanResource::SetDeviceLocalBuffer(
		VulkanLogicalDevice& logical_device, 
		VulkanCommandQueue& copy_queue, 
		std::span<std::byte const> raw, 
		std::size_t offset
	) {

		VulkanVideoMemory upload_memory(
			&logical_device, 
			raw.size(), 
			DetermineMemoryUsageFromResourceType(m_type),
			VideoMemoryType::HostVisible
		);

		VulkanResource upload_resource(&upload_memory, raw.size(), 1u, 1u, m_type);
		upload_resource.SetBufferDataImpl(logical_device, copy_queue, raw, offset);

		VulkanCommandBuffer command_buffer(&logical_device, copy_queue.GetFamily());
		vk::Queue native_queue = copy_queue.GetNative();
		vk::CommandBuffer native_buffer = command_buffer.GetNative();

		vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr, nullptr);
		command_buffer.GetNative().begin(begin_info, *logical_device.GetRawDispatcher());

		vk::BufferMemoryBarrier upload_barrier(
			vk::AccessFlagBits::eHostWrite,
			vk::AccessFlagBits::eTransferRead,
			vk::QueueFamilyIgnored,
			vk::QueueFamilyIgnored,
			upload_resource.GetBufferNative(),
			0,
			vk::WholeSize
		);

		command_buffer.GetNative().pipelineBarrier(
			vk::PipelineStageFlagBits::eHost,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlagBits::eByRegion,
			0, nullptr,
			1, &upload_barrier,
			0, nullptr,
			*logical_device.GetRawDispatcher()
		);

		vk::BufferCopy buffer_copy_data(0, offset, upload_memory.GetSize());
		command_buffer.GetNative().copyBuffer(
			upload_resource.GetBufferNative(),
			GetBufferNative(),
			buffer_copy_data,
			*logical_device.GetRawDispatcher()
		);

		vk::BufferMemoryBarrier copy_barrier(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndexRead,
			vk::QueueFamilyIgnored,
			vk::QueueFamilyIgnored,
			GetBufferNative(),
			0,
			vk::WholeSize
		);

		command_buffer.GetNative().pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eVertexInput,
			vk::DependencyFlagBits::eByRegion,
			0, nullptr,
			1, &copy_barrier,
			0, nullptr,
			*logical_device.GetRawDispatcher()
		);

		command_buffer.GetNative().end(*logical_device.GetRawDispatcher());
		copy_queue.ExecuteCommand(command_buffer);

	}

	void VulkanResource::SetHostVisibleBuffer(std::span<std::byte const> raw, std::size_t offset) {
		UniqueMappedHandle mapped_handle = MapImpl(0u, raw.size());
		std::memcpy(mapped_handle->ptr, raw.data(), raw.size());
	}

	void VulkanResource::SetBufferDataImpl(
		VulkanLogicalDevice& logical_device, 
		VulkanCommandQueue& copy_queue, 
		std::span<std::byte const> raw, 
		std::size_t offset
	) {

		switch (m_type) {
		case ResourceType::VertexBuffer:
		case ResourceType::IndexBuffer:
			break;

		default:
			throw std::runtime_error("Not buffer");
		}

		plastic::utility::AnyPointer<VulkanVideoMemory> video_memory
			= std::visit(
				[](auto&& handle) -> plastic::utility::AnyPointer<VulkanVideoMemory> {
					using Handle = std::decay_t<decltype(handle)>;
					if constexpr (!std::same_as<Handle, std::monostate>) {
						return plastic::utility::QueryObject<VulkanVideoMemory>(handle.GetGC().GetVideoMemoryID());
					}
					else {
						return nullptr;
					}
				},
				m_handle
			);

		VideoMemoryType mem_type = video_memory->GetType();

		switch (mem_type) {
		case fyuu_rhi::VideoMemoryType::DeviceReadback:
			break;

		case fyuu_rhi::VideoMemoryType::HostVisible:
			SetHostVisibleBuffer(raw, offset);
			break;

		case fyuu_rhi::VideoMemoryType::DeviceLocal:
		default:
			SetDeviceLocalBuffer(logical_device, copy_queue, raw, offset);
			break;
		}


	}

	VulkanResource VulkanResource::CreateVertexBuffer(std::size_t size_in_bytes) {
		return VulkanResource(size_in_bytes, 1u, 1u, ResourceType::VertexBuffer);
	}

	VulkanResource VulkanResource::CreateIndexBuffer(std::size_t size_in_bytes) {
		return VulkanResource(size_in_bytes, 1u, 1u, ResourceType::IndexBuffer);
	}

	VulkanResource::VulkanResource(std::size_t width, std::size_t height, std::size_t depth, ResourceType type)
		: m_width(width),
		m_height(height),
		m_depth(depth),
		m_type(type),
		m_handle() {
	}

	static std::variant<std::monostate, UniqueVulkanBufferHandle, UniqueVulkanTextureHandle> CreateResourceHandleFromMemory(
		VulkanVideoMemory& memory,
		std::size_t width,
		std::size_t height,
		std::size_t depth,
		ResourceType type
	) {

		if (DetermineResourceTypeFromMemoryUsage(memory.GetUsage()) == ResourceType::Unknown) {
			throw std::runtime_error("Incompatible memory");
		}

		std::variant<std::monostate, UniqueVulkanBufferHandle, UniqueVulkanTextureHandle> handle;
		switch (type) {
		case ResourceType::VertexBuffer:
		case ResourceType::IndexBuffer:
			handle.emplace<UniqueVulkanBufferHandle>(memory.CreateBufferHandle(width));
			break;

		case ResourceType::Texture1D:
		case ResourceType::Texture2D:
		case ResourceType::Texture3D:
			handle.emplace<UniqueVulkanTextureHandle>(memory.CreateTextureHandle(width, height, depth));

		default:
			throw std::invalid_argument("Invalid resource type");
		}

		return handle;

	}

	VulkanResource::VulkanResource(
		plastic::utility::AnyPointer<VulkanVideoMemory> const& memory,
		std::size_t width,
		std::size_t height,
		std::size_t depth,
		ResourceType type
	) : m_width(width),
		m_height(height),
		m_depth(depth),
		m_type(type),
		m_handle(CreateResourceHandleFromMemory(*memory, width, height, depth, type)) {

	}

	VulkanResource::VulkanResource(
		VulkanVideoMemory& memory, 
		std::size_t width, 
		std::size_t height, 
		std::size_t depth, 
		ResourceType type
	) : Registrable(),
		m_width(width),
		m_height(height),
		m_depth(depth),
		m_type(type),
		m_handle(CreateResourceHandleFromMemory(memory, width, height, depth, type)) {
	}

	VulkanResource::VulkanResource(VulkanResource&& other) noexcept
		: Registrable(std::move(other)),
		m_width(std::exchange(other.m_width, 0u)),
		m_height(std::exchange(other.m_height, 0u)),
		m_depth(std::exchange(other.m_depth, 0u)),
		m_type(std::exchange(other.m_type, ResourceType::Unknown)),
		m_handle(std::move(other.m_handle)) {
	}

	VulkanResource::~VulkanResource() noexcept {
		ReleaseImpl();
	}

	vk::Buffer VulkanResource::GetBufferNative() const {

		if (UniqueVulkanBufferHandle const* buffer_handle = std::get_if<UniqueVulkanBufferHandle>(&m_handle)) {
			return buffer_handle->Get().get();
		}

		return nullptr;

	}
	
	VulkanResource::MappedHandleGC::MappedHandleGC(std::size_t resource_id) noexcept
		: m_resource_id(resource_id) {
	}

	VulkanResource::MappedHandleGC::MappedHandleGC(MappedHandleGC const& other) noexcept
		: m_resource_id(other.m_resource_id) {
	}

	VulkanResource::MappedHandleGC::MappedHandleGC(MappedHandleGC&& other) noexcept
		: m_resource_id(std::exchange(other.m_resource_id, 0u)) {
	}

	VulkanResource::MappedHandleGC& VulkanResource::MappedHandleGC::operator=(MappedHandleGC const& other) noexcept {
		if (this != &other) {
			m_resource_id = other.m_resource_id;
		}
		return *this;
	}

	VulkanResource::MappedHandleGC& VulkanResource::MappedHandleGC::operator=(MappedHandleGC&& other) noexcept {
		if (this != &other) {
			m_resource_id = std::exchange(other.m_resource_id, 0u);
		}
		return *this;
	}

	void VulkanResource::MappedHandleGC::operator()(MappedHandle& handle) {

		plastic::utility::AnyPointer<VulkanResource>
			resource = plastic::utility::QueryObject<VulkanResource>(m_resource_id);

		plastic::utility::AnyPointer<VulkanVideoMemory> video_memory
			= std::visit(
				[](auto&& handle) -> plastic::utility::AnyPointer<VulkanVideoMemory> {
					using Handle = std::decay_t<decltype(handle)>;
					if constexpr (!std::same_as<Handle, std::monostate>) {
						return plastic::utility::QueryObject<VulkanVideoMemory>(handle.GetGC().GetVideoMemoryID());
					}
					else {
						return nullptr;
					}
				},
				resource->m_handle
			);

		VmaAllocator allocator = video_memory->GetAllocator();
		VmaAllocation allocation = video_memory->GetNative();

		vmaUnmapMemory(allocator, allocation);

	}

	std::size_t VulkanResource::MappedHandleGC::GetResourceID() const noexcept {
		return m_resource_id;
	}

}