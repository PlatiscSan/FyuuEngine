module;
//#include <vma/vk_mem_alloc.h>

module fyuu_rhi:vulkan_memory;
import :vulkan_physical_device;
import :vulkan_logical_device;
import :vulkan_resource;

namespace fyuu_rhi::vulkan {

	void details::VMAAllocationGC::operator()(VmaAllocation allocation) {
		vmaFreeMemory(allocator, allocation);
	}

	details::VMAAllocationGC::VMAAllocationGC(VmaAllocator allocator_)
		: allocator(allocator_) {
	}

	details::VMAAllocationGC::VMAAllocationGC(VMAAllocationGC const& other)
		: allocator(other.allocator) {
	}

	details::VMAAllocationGC::VMAAllocationGC(VMAAllocationGC&& other) noexcept
		: allocator(std::exchange(other.allocator, nullptr)) {
	}

	static VmaMemoryUsage VideoMemoryTypeToVmaMemoryUsage(VideoMemoryType type) noexcept {
		switch (type) {
		case VideoMemoryType::DeviceLocal:
			return VMA_MEMORY_USAGE_GPU_ONLY;

		case VideoMemoryType::HostVisible:
			return VMA_MEMORY_USAGE_CPU_TO_GPU;

		case VideoMemoryType::DeviceReadback:
			return VMA_MEMORY_USAGE_GPU_TO_CPU;

		default:
			return VMA_MEMORY_USAGE_AUTO;
		}
	}

	/*static vk::MemoryPropertyFlags VideoMemoryTypeToVkMemoryPropertyFlags(VideoMemoryType type) noexcept {
		switch (type) {
		case VideoMemoryType::DeviceLocal:
			return vk::MemoryPropertyFlagBits::eDeviceLocal;

		case VideoMemoryType::HostVisible:
			return vk::MemoryPropertyFlagBits::eHostVisible | 
				vk::MemoryPropertyFlagBits::eHostCoherent;

		case VideoMemoryType::DeviceReadback:
			return vk::MemoryPropertyFlagBits::eHostVisible | 
				vk::MemoryPropertyFlagBits::eHostCoherent | 
				vk::MemoryPropertyFlagBits::eHostCached;

		default:
			return vk::MemoryPropertyFlagBits::eDeviceLocal;
		}
	}*/

	static vk::UniqueBuffer CreateBuffer(
		VulkanLogicalDevice const& logical_device,
		std::size_t size_in_bytes,
		VideoMemoryUsage usage,
		VideoMemoryType type
	) {

		vk::Device vk_device = logical_device.GetNative();
		vk::DispatchLoaderDynamic* dispatcher = logical_device.GetRawDispatcher();

		vk::BufferUsageFlags usage_flags = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;

		switch (usage) {
		case fyuu_rhi::VideoMemoryUsage::IndexBuffer:
			usage_flags |= vk::BufferUsageFlagBits::eIndexBuffer;
			break;
		case fyuu_rhi::VideoMemoryUsage::VertexBuffer:
			usage_flags |= vk::BufferUsageFlagBits::eVertexBuffer;
			break;
		default:
			break;
		}

		vk::BufferCreateInfo buffer_info{
			{},
			size_in_bytes,
			usage_flags,
			vk::SharingMode::eExclusive,
			0,
			nullptr
		};

		return vk_device.createBufferUnique(
			buffer_info,
			nullptr,
			*dispatcher
		);

	}

	static details::UniqueVMAAllocation AllocateBufferMemory(
		VulkanLogicalDevice const& logical_device,
		std::size_t size_in_bytes,
		VideoMemoryUsage usage,
		VideoMemoryType type
	) {

		vk::Device vk_device = logical_device.GetNative();
		VmaAllocator vma_alloc = logical_device.GetVideoMemoryAllocator();
		vk::DispatchLoaderDynamic* dispatcher = logical_device.GetRawDispatcher();

		vk::UniqueBuffer temp_buffer = CreateBuffer(logical_device, size_in_bytes, usage, type);

		vk::MemoryRequirements buffer_mem_req = vk_device.getBufferMemoryRequirements(*temp_buffer, *dispatcher);

		VmaAllocationCreateInfo info{
			0,
			VideoMemoryTypeToVmaMemoryUsage(type),
			0,
			0,
			0,
			nullptr,
			nullptr,
			0.0f
		};

		VmaAllocation allocation;

		VkResult result = vmaAllocateMemory(
			vma_alloc,
			reinterpret_cast<VkMemoryRequirements const*>(&buffer_mem_req),
			&info,
			&allocation,
			nullptr
		);

		if (result != VK_SUCCESS) {
			throw std::runtime_error(
				std::format(
					"Failed to allocate video memory, VkResult: {}", 
					static_cast<std::size_t>(result)
				)
			);
		}

		return details::UniqueVMAAllocation(allocation, vma_alloc);

	}

	static details::UniqueVMAAllocation AllocateTextureMemory(
		VulkanLogicalDevice const& logical_device,
		std::size_t size_in_bytes,
		VideoMemoryUsage usage,
		VideoMemoryType type
	) {
		return {};
	}

	static details::UniqueVMAAllocation Allocate(
		VulkanLogicalDevice const& logical_device,
		std::size_t size_in_bytes,
		VideoMemoryUsage usage,
		VideoMemoryType type
	) {
		
		switch (usage) {
		case fyuu_rhi::VideoMemoryUsage::Texture3D:
		case fyuu_rhi::VideoMemoryUsage::Texture2D:
			return AllocateTextureMemory(logical_device, size_in_bytes, usage, type);

		case fyuu_rhi::VideoMemoryUsage::IndexBuffer:
		case fyuu_rhi::VideoMemoryUsage::VertexBuffer:
		case fyuu_rhi::VideoMemoryUsage::Unknown:
		default:
			return AllocateBufferMemory(logical_device, size_in_bytes, usage, type);
		}

	}

	VulkanVideoMemory::UniqueBufferHandle VulkanVideoMemory::CreateBufferHandleImpl(std::size_t size_in_bytes) {

		try {

			if (m_allocated_flag.test_and_set(std::memory_order::relaxed)) {
				throw AllocationError("The memory is already allocated");
			}

			plastic::utility::AnyPointer<VulkanLogicalDevice> logical_device
				= plastic::utility::QueryObject<VulkanLogicalDevice>(m_device_id);

			if (m_allocation->GetSize() < size_in_bytes) {
				throw AllocationError("Insufficient size");
			}

			vk::UniqueBuffer buffer = CreateBuffer(*logical_device, size_in_bytes, m_usage, m_type);

			vmaBindBufferMemory(GetAllocator(), m_allocation.get(), *buffer);

			return UniqueBufferHandle(std::move(buffer), GetID());

		}
		catch (std::exception const&) {
			m_allocated_flag.clear(std::memory_order::relaxed);
			throw;
		}

	}

	VulkanVideoMemory::UniqueTextureHandle VulkanVideoMemory::CreateTextureHandleImpl(
		std::size_t width,
		std::size_t height, 
		std::size_t depth
	) {
		return UniqueTextureHandle(vk::UniqueImage{}, 0);
	}

	VideoMemoryUsage VulkanVideoMemory::GetUsageImpl() const noexcept {
		return m_usage;
	}

	VideoMemoryType VulkanVideoMemory::GetTypeImpl() const noexcept {
		return m_type;
	}

	VulkanVideoMemory::VulkanVideoMemory(
		plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device, 
		std::size_t size_in_bytes, 
		VideoMemoryUsage usage,
		VideoMemoryType type
	) : m_allocation(Allocate(*logical_device, size_in_bytes, usage, type)),
		m_device_id(logical_device->GetID()),
		m_allocated_flag(),
		m_usage(usage),
		m_type(type) {
	}

	VulkanVideoMemory::VulkanVideoMemory(
		VulkanLogicalDevice const& logical_device, 
		std::size_t size_in_bytes, 
		VideoMemoryUsage usage, 
		VideoMemoryType type
	) : m_allocation(Allocate(logical_device, size_in_bytes, usage, type)),
		m_device_id(logical_device.GetID()),
		m_allocated_flag(),
		m_usage(usage),
		m_type(type) {
	}

	VulkanVideoMemory::~VulkanVideoMemory() noexcept {

		std::size_t spin = 0;

		while (m_allocated_flag.test(std::memory_order::relaxed)) {
			if (++spin > 100u) {
				m_allocated_flag.wait(true, std::memory_order::relaxed);
			}
			else {
				std::this_thread::yield();
			}
		}

	}

	VmaAllocator VulkanVideoMemory::GetAllocator() const noexcept {
		plastic::utility::AnyPointer<VulkanLogicalDevice>
			logical_device = plastic::utility::QueryObject<VulkanLogicalDevice>(m_device_id);
		return logical_device->GetVideoMemoryAllocator();
	}

	VmaAllocation VulkanVideoMemory::GetNative() const noexcept {
		return m_allocation.get();
	}

	std::size_t VulkanVideoMemory::GetSize() const noexcept {
		return m_allocation->GetSize();
	}

	void VulkanVideoMemory::HandleGC::ResetMemory() const {

		plastic::utility::AnyPointer<VulkanVideoMemory>
			video_memory = plastic::utility::QueryObject<VulkanVideoMemory>(m_video_memory_id);

		video_memory->m_allocated_flag.clear(std::memory_order::relaxed);
		video_memory->m_allocated_flag.notify_one();

	}

	VulkanVideoMemory::HandleGC::HandleGC(std::size_t video_memory_id) noexcept
		: m_video_memory_id(video_memory_id) {
	}

	VulkanVideoMemory::HandleGC::HandleGC(HandleGC const& other) noexcept
		: m_video_memory_id(other.m_video_memory_id) {
	}

	VulkanVideoMemory::HandleGC::HandleGC(HandleGC&& other) noexcept
		: m_video_memory_id(std::exchange(other.m_video_memory_id, 0u)) {
	}

	void VulkanVideoMemory::HandleGC::operator()(vk::UniqueBuffer& buffer) const {
		buffer.reset();
		ResetMemory();
	}

	void VulkanVideoMemory::HandleGC::operator()(vk::UniqueImage& texture) const {
		texture.reset();
		ResetMemory();
	}

	std::size_t VulkanVideoMemory::HandleGC::GetVideoMemoryID() const noexcept {
		return m_video_memory_id;
	}

}