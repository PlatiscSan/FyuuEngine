module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <array>
#include <unordered_map>
#include <deque>
#include <memory_resource>
#include <format>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include <vma/vk_mem_alloc.h>
#endif //!defined(__APPLE__)
#include <tbb/concurrent_lru_cache.h>
#include <boost/scope/unique_resource.hpp>
module fyuu_rhi:vulkan_logical_device;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :vulkan_traits;
import :core_types;
import :vulkan_queue_allocator;
import :command_types;
import :vulkan_texture_tracker;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::vulkan;

	Backend::UniqueBinarySemaphore CreateBinarySemaphore(Backend::LogicalDevice const& ld) {
		
		static thread_local std::unordered_map<vk::Device, std::deque<vk::SharedSemaphore>> bin_sems;

		std::deque<vk::SharedSemaphore>& free_sems = bin_sems[*ld.impl];

		static auto GC = [](vk::SharedSemaphore& sem) {
			vk::SharedDevice owner = sem.getDestructorType();
			std::deque<vk::SharedSemaphore>& free_sems = bin_sems[*owner];
			free_sems.emplace_back(std::move(sem));
			};


		if (free_sems.empty()) {
			vk::SemaphoreTypeCreateInfo sem_type_info(
				vk::SemaphoreType::eBinary,
				0ull	// initial value
			);
			vk::SemaphoreCreateInfo sem_info(
				vk::SemaphoreCreateFlags{},
				&sem_type_info
			);
			vk::SharedSemaphore sem(
				ld.impl->createSemaphore(sem_info, nullptr, *ld.dispatcher),
				ld.impl,
				{ nullptr, *ld.dispatcher }
			);

			return { std::move(sem), GC };

		}

		vk::SharedSemaphore sem = std::move(free_sems.front());
		free_sems.pop_front();

		return { std::move(sem), GC };

	}

	Backend::UniqueFence CreateFence(Backend::LogicalDevice const& ld) {

		static thread_local std::unordered_map<vk::Device, std::deque<vk::SharedFence>> fences;

		std::deque<vk::SharedFence>& free_fences = fences[*ld.impl];

		static auto GC = [](vk::SharedFence& fence) {
			vk::SharedDevice owner = fence.getDestructorType();
			std::deque<vk::SharedFence>& free_fences = fences[*owner];
			free_fences.emplace_back(std::move(fence));
			};


		if (free_fences.empty()) {
			vk::SharedFence fence(
				ld.impl->createFence({}, nullptr, *ld.dispatcher),
				ld.impl,
				{ nullptr, *ld.dispatcher }
			);

			return { std::move(fence), GC };

		}

		vk::SharedFence fence = std::move(free_fences.front());
		free_fences.pop_front();

		return { std::move(fence), GC };

	}

	VmaAllocationCreateFlags ExtractAllocationFlags(ResourceFlags const& flags) {
		
		VmaAllocationCreateFlags vma_flags{};
		
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::UndedicatedAllocation, ResourceFlagBits::DedicatedAllocation);
		if (is_conflicting) {
			throw std::invalid_argument("UndedicatedAllocation and DedicatedAllocation are set simultaneously");
		}
		if (flags.Test(ResourceFlagBits::UndedicatedAllocation)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT;
		}
		else if (flags.Test(ResourceFlagBits::DedicatedAllocation)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}
		else {

		}

		if (flags.Test(ResourceFlagBits::AllocationWithinBudget)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
		}
		if (flags.Test(ResourceFlagBits::AllocationAtUpperAddress)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT;
		}
		if (flags.Test(ResourceFlagBits::AllocationAliasAllowed)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
		}

		is_conflicting = flags.TestSingleInRange(ResourceFlagBits::MinOffsetAllocation, ResourceFlagBits::FirstFitAllocation);
		if (is_conflicting) {
			throw std::invalid_argument("MinOffsetAllocation BestFitAllocation or FirstFitAllocation are set simultaneously");
		}
		else if (flags.Test(ResourceFlagBits::MinOffsetAllocation)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_STRATEGY_MIN_OFFSET_BIT;
		}
		else if (flags.Test(ResourceFlagBits::BestFitAllocation)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
		}
		else if (flags.Test(ResourceFlagBits::FirstFitAllocation)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT;
		}
		else {

		}

		if (flags.Test(ResourceFlagBits::HostVisible)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		if (flags.Test(ResourceFlagBits::DeviceReadback)) {
			vma_flags |= VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		return vma_flags;

	}

	VmaMemoryUsage ExtractMemoryUsage(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::DeviceLocal, ResourceFlagBits::DeviceReadback);
		if (is_conflicting) {
			throw std::invalid_argument("DeviceLocal HostVisible or DeviceReadback are set simultaneously");
		}
		if (flags.Test(ResourceFlagBits::DeviceLocal)) {
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}
		else if (flags.Test(ResourceFlagBits::HostVisible)) {
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
		}
		else if (flags.Test(ResourceFlagBits::DeviceReadback)) {
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
		}
		else {
			return VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}
	}

	vk::BufferUsageFlags ExtractBufferUsage(ResourceFlags const& flags) noexcept {
		vk::BufferUsageFlags vk_flags;
		if (flags.Test(ResourceFlagBits::CopySRC)) {
			vk_flags |= vk::BufferUsageFlagBits::eTransferSrc;
		}
		if (flags.Test(ResourceFlagBits::CopyDST)) {
			vk_flags |= vk::BufferUsageFlagBits::eTransferDst;
		}
		if (flags.Test(ResourceFlagBits::UniformTexelBuffer)) {
			vk_flags |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
		}
		if (flags.Test(ResourceFlagBits::StorageTexelBuffer)) {
			vk_flags |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
		}
		if (flags.Test(ResourceFlagBits::UniformBuffer)) {
			vk_flags |= vk::BufferUsageFlagBits::eUniformBuffer;
		}
		if (flags.Test(ResourceFlagBits::StorageBuffer)) {
			vk_flags |= vk::BufferUsageFlagBits::eStorageBuffer;
		}
		if (flags.Test(ResourceFlagBits::IndexBuffer)) {
			vk_flags |= vk::BufferUsageFlagBits::eIndexBuffer;
		}
		if (flags.Test(ResourceFlagBits::VertexBuffer)) {
			vk_flags |= vk::BufferUsageFlagBits::eVertexBuffer;
		}
		if (flags.Test(ResourceFlagBits::IndirectBuffer)) {
			vk_flags |= vk::BufferUsageFlagBits::eIndirectBuffer;
		}
		return vk_flags;
	}

	vk::ImageType ExtractTextureDimension(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Texture1D, ResourceFlagBits::Texture3D);
		if (is_conflicting) {
			throw std::invalid_argument("Texture1D Texture2D or Texture3D are set simultaneously");
		}
		if (flags.Test(ResourceFlagBits::Texture1D)) {
			return vk::ImageType::e1D;
		}
		else if (flags.Test(ResourceFlagBits::Texture2D)) {
			return vk::ImageType::e2D;
		}
		else if (flags.Test(ResourceFlagBits::Texture3D)) {
			return vk::ImageType::e3D;
		}
		else {
			return vk::ImageType::e2D;
		}
	}

	vk::Format ExtractFormat(ResourceFlags const& flags) {

		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::R8Unorm, ResourceFlagBits::Bc7UnormSrgb);
		if (is_conflicting) {
			throw std::invalid_argument("Only one format can be set");
		}

		if (flags.Test(ResourceFlagBits::R8Unorm)) return vk::Format::eR8Unorm;
		else if (flags.Test(ResourceFlagBits::R8Snorm)) return vk::Format::eR8Snorm;
		else if (flags.Test(ResourceFlagBits::R8Uint)) return vk::Format::eR8Uint;
		else if (flags.Test(ResourceFlagBits::R8Sint)) return vk::Format::eR8Sint;

		else if (flags.Test(ResourceFlagBits::R8G8Unorm)) return vk::Format::eR8G8Unorm;
		else if (flags.Test(ResourceFlagBits::R8G8Snorm)) return vk::Format::eR8G8Snorm;
		else if (flags.Test(ResourceFlagBits::R8G8Uint)) return vk::Format::eR8G8Uint;
		else if (flags.Test(ResourceFlagBits::R8G8Sint)) return vk::Format::eR8G8Sint;

		else if (flags.Test(ResourceFlagBits::R8G8B8A8Unorm)) return vk::Format::eR8G8B8A8Unorm;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Snorm)) return vk::Format::eR8G8B8A8Snorm;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Uint)) return vk::Format::eR8G8B8A8Uint;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Sint)) return vk::Format::eR8G8B8A8Sint;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Srgb)) return vk::Format::eR8G8B8A8Srgb;
		else if (flags.Test(ResourceFlagBits::B8G8R8A8Srgb)) return vk::Format::eB8G8R8A8Srgb;

		else if (flags.Test(ResourceFlagBits::R16Unorm)) return vk::Format::eR16Unorm;
		else if (flags.Test(ResourceFlagBits::R16Snorm)) return vk::Format::eR16Snorm;
		else if (flags.Test(ResourceFlagBits::R16Uint)) return vk::Format::eR16Uint;
		else if (flags.Test(ResourceFlagBits::R16Sint)) return vk::Format::eR16Sint;
		else if (flags.Test(ResourceFlagBits::R16Float)) return vk::Format::eR16Sfloat;

		else if (flags.Test(ResourceFlagBits::R16G16Unorm)) return vk::Format::eR16G16Unorm;
		else if (flags.Test(ResourceFlagBits::R16G16Snorm)) return vk::Format::eR16G16Snorm;
		else if (flags.Test(ResourceFlagBits::R16G16Uint)) return vk::Format::eR16G16Uint;
		else if (flags.Test(ResourceFlagBits::R16G16Sint)) return vk::Format::eR16G16Sint;
		else if (flags.Test(ResourceFlagBits::R16G16Float)) return vk::Format::eR16G16Sfloat;

		else if (flags.Test(ResourceFlagBits::R16G16B16A16Unorm)) return vk::Format::eR16G16B16A16Unorm;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Snorm)) return vk::Format::eR16G16B16A16Snorm;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Uint)) return vk::Format::eR16G16B16A16Uint;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Sint)) return vk::Format::eR16G16B16A16Sint;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Float)) return vk::Format::eR16G16B16A16Sfloat;

		else if (flags.Test(ResourceFlagBits::R32Uint)) return vk::Format::eR32Uint;
		else if (flags.Test(ResourceFlagBits::R32Sint)) return vk::Format::eR32Sint;
		else if (flags.Test(ResourceFlagBits::R32Float)) return vk::Format::eR32Sfloat;

		else if (flags.Test(ResourceFlagBits::R32G32Uint)) return vk::Format::eR32G32Uint;
		else if (flags.Test(ResourceFlagBits::R32G32Sint)) return vk::Format::eR32G32Sint;
		else if (flags.Test(ResourceFlagBits::R32G32Float)) return vk::Format::eR32G32Sfloat;

		else if (flags.Test(ResourceFlagBits::R32G32B32A32Uint)) return vk::Format::eR32G32B32A32Uint;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Sint)) return vk::Format::eR32G32B32A32Sint;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Float)) return vk::Format::eR32G32B32A32Sfloat;

		// Packed formats
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Unorm)) return vk::Format::eA2R10G10B10UnormPack32;
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Uint)) return vk::Format::eA2R10G10B10UintPack32;
		else if (flags.Test(ResourceFlagBits::R11G11B10Float)) return vk::Format::eB10G11R11UfloatPack32;
		else if (flags.Test(ResourceFlagBits::R9G9B9E5SharedExp)) return vk::Format::eE5B9G9R9UfloatPack32;

		// Depth/stencil
		else if (flags.Test(ResourceFlagBits::D16Unorm)) return vk::Format::eD16Unorm;
		else if (flags.Test(ResourceFlagBits::D24UnormS8Uint)) return vk::Format::eD24UnormS8Uint;
		else if (flags.Test(ResourceFlagBits::D32Float)) return vk::Format::eD32Sfloat;
		else if (flags.Test(ResourceFlagBits::D32FloatS8X24Uint)) return vk::Format::eD32SfloatS8Uint;

		// BC compressed
		else if (flags.Test(ResourceFlagBits::Bc1Unorm)) return vk::Format::eBc1RgbaUnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc1UnormSrgb)) return vk::Format::eBc1RgbaSrgbBlock;
		else if (flags.Test(ResourceFlagBits::Bc2Unorm)) return vk::Format::eBc2UnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc2UnormSrgb)) return vk::Format::eBc2SrgbBlock;
		else if (flags.Test(ResourceFlagBits::Bc3Unorm)) return vk::Format::eBc3UnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc3UnormSrgb)) return vk::Format::eBc3SrgbBlock;
		else if (flags.Test(ResourceFlagBits::Bc4Unorm)) return vk::Format::eBc4UnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc4Snorm)) return vk::Format::eBc4SnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc5Unorm)) return vk::Format::eBc5UnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc5Snorm)) return vk::Format::eBc5SnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc6HUfloat)) return vk::Format::eBc6HUfloatBlock;
		else if (flags.Test(ResourceFlagBits::Bc6HSfloat)) return vk::Format::eBc6HSfloatBlock;
		else if (flags.Test(ResourceFlagBits::Bc7Unorm)) return vk::Format::eBc7UnormBlock;
		else if (flags.Test(ResourceFlagBits::Bc7UnormSrgb)) return vk::Format::eBc7SrgbBlock;

		else return vk::Format::eUndefined;

	}

	vk::SampleCountFlagBits ExtractSampleCount(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Sample1, ResourceFlagBits::Sample64);
		if (is_conflicting) {
			throw std::invalid_argument("Only sample count can be set");
		}
		if (flags.Test(ResourceFlagBits::Sample1)) {
			return vk::SampleCountFlagBits::e1;
		}
		else if (flags.Test(ResourceFlagBits::Sample2)) {
			return vk::SampleCountFlagBits::e2;
		}
		else if (flags.Test(ResourceFlagBits::Sample4)) {
			return vk::SampleCountFlagBits::e4;
		}
		else if (flags.Test(ResourceFlagBits::Sample8)) {
			return vk::SampleCountFlagBits::e8;
		}
		else if (flags.Test(ResourceFlagBits::Sample16)) {
			return vk::SampleCountFlagBits::e16;
		}
		else if (flags.Test(ResourceFlagBits::Sample32)) {
			return vk::SampleCountFlagBits::e32;
		}
		else if (flags.Test(ResourceFlagBits::Sample64)) {
			return vk::SampleCountFlagBits::e64;
		}
		else {
			return vk::SampleCountFlagBits::e1;
		}
		
	}

	vk::ImageTiling ExtractTiling(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::DeviceLocal, ResourceFlagBits::DeviceReadback);
		if (is_conflicting) {
			throw std::invalid_argument("DeviceLocal HostVisible or DeviceReadback are set simultaneously");
		}
		if (flags.Test(ResourceFlagBits::DeviceLocal)) {
			return vk::ImageTiling::eOptimal;
		}
		else if (flags.Test(ResourceFlagBits::HostVisible)) {
			return vk::ImageTiling::eLinear;
		}
		else if (flags.Test(ResourceFlagBits::DeviceReadback)) {
			return vk::ImageTiling::eLinear;
		}
		else {
			return vk::ImageTiling::eOptimal;
		}
		
	}

	vk::ImageUsageFlags ExtractTextureUsage(ResourceFlags const& flags) {
		vk::ImageUsageFlags usage{};

		if (flags.Test(ResourceFlagBits::CopySRC)) {
			usage |= vk::ImageUsageFlagBits::eTransferSrc;
		}
		if (flags.Test(ResourceFlagBits::CopyDST)) {
			usage |= vk::ImageUsageFlagBits::eTransferDst;
		}
		if (flags.Test(ResourceFlagBits::TextureBinding)) {
			usage |= vk::ImageUsageFlagBits::eSampled;
		}
		if (flags.Test(ResourceFlagBits::StorageBinding)) {
			usage |= vk::ImageUsageFlagBits::eStorage;
		}
		if (flags.Test(ResourceFlagBits::RenderAttachment)) {
			usage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment;
		}
		if (flags.Test(ResourceFlagBits::TransientAttachment)) {
			usage |= vk::ImageUsageFlagBits::eTransientAttachment;
		}
		if (flags.Test(ResourceFlagBits::StorageAttachment)) {
			// StorageAttachment is interpreted as input attachment
			usage |= vk::ImageUsageFlagBits::eInputAttachment;
		}

		return usage;
	}

	vk::ImageViewType ExtractTextureViewType(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::TextureView1D, ResourceFlagBits::TextureView3D);
		if (is_conflicting) {
			throw std::invalid_argument("Only one texture view type can be set");
		}
		if (flags.Test(ResourceFlagBits::TextureView1D)) {
			return vk::ImageViewType::e1D;
		}
		else if (flags.Test(ResourceFlagBits::TextureView2D)) {
			return vk::ImageViewType::e2D;
		}
		else if (flags.Test(ResourceFlagBits::TextureView3D)) {
			return vk::ImageViewType::e3D;
		}
		else if (flags.Test(ResourceFlagBits::TextureViewCube)) {
			return vk::ImageViewType::eCube;
		}
		else if (flags.Test(ResourceFlagBits::TextureView2DArray)) {
			return vk::ImageViewType::e2DArray;
		}
		else if (flags.Test(ResourceFlagBits::TextureViewCubeArray)) {
			return vk::ImageViewType::eCubeArray;
		}
		else {
			return vk::ImageViewType::e2D;
		}
	}

	vk::ImageAspectFlags ExtractTextureAspect(Backend::Resource const& res, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags) {



	}

}

namespace fyuu_rhi::vulkan {

	Backend::Promise Backend::CreatePromise(Backend::LogicalDevice const& ld) {

		auto iter = std::find(ld.enabled_extensions.begin(), ld.enabled_extensions.end(), vk::KHRTimelineSemaphoreExtensionName);

		if (iter != ld.enabled_extensions.end()) {
			vk::SemaphoreTypeCreateInfo sem_type_info(
				vk::SemaphoreType::eTimeline,
				0ull	// initial value
			);
			vk::SemaphoreCreateInfo sem_info(
				vk::SemaphoreCreateFlags{},
				&sem_type_info
			);
			vk::SharedSemaphore sem(
				ld.impl->createSemaphore(sem_info, nullptr, *ld.dispatcher),
				ld.impl,
				{ nullptr, *ld.dispatcher }
			);

			return { 
				decltype(Backend::Promise::impl)(std::in_place_type<Backend::Promise::TimelineSemaphore>, std::move(sem), TimelineCounter{}), 
				ld.dispatcher,
				{}
			};

		}

		static thread_local std::array<std::byte, 64 * 1024> fixed_buffer; // 64KB fixed buffer
		static thread_local std::pmr::monotonic_buffer_resource fixed_upstream(
			fixed_buffer.data(), fixed_buffer.size(),
			std::pmr::new_delete_resource()
		);

		static thread_local std::pmr::unsynchronized_pool_resource pool(&fixed_upstream);
		static thread_local std::pmr::polymorphic_allocator<Backend::BinarySynchronization> alloc(&pool);

		return { 
			decltype(Backend::Promise::impl)(std::allocate_shared<Backend::BinarySynchronization>(alloc, CreateBinarySemaphore(ld), CreateFence(ld))),
			ld.dispatcher,
			{}
		};
	}

	Backend::Resource Backend::CreateBuffer(LogicalDevice const& ld, std::size_t size_in_bytes, ResourceFlags const& flags) {
		
		VmaAllocationCreateInfo alloc_create_info{
			ExtractAllocationFlags(flags),
			ExtractMemoryUsage(flags),
			0, 0, 0, nullptr, nullptr, 0.0f
		};

		VkBufferCreateInfo buf_info(
			vk::BufferCreateInfo(
				{},
				size_in_bytes,
				ExtractBufferUsage(flags),
				vk::SharingMode::eExclusive,
				0u,
				nullptr,
				nullptr
			)
		);

		VkBuffer buf;
		VmaAllocation alloc;
		VmaAllocationInfo alloc_info;

		auto result = static_cast<vk::Result>(
			vmaCreateBuffer(
				ld.mem_alloc.get(),
				&buf_info,
				&alloc_create_info,
				&buf,
				&alloc,
				&alloc_info
			));

		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("Calling vmaCreateBuffer() failed, VMA reported: {}", vk::to_string(result)));
		}

		static thread_local std::array<std::byte, 1024 * sizeof(Backend::Resource::Buffer)> fixed_buffer;
		static thread_local std::pmr::monotonic_buffer_resource fixed_upstream(
			fixed_buffer.data(), fixed_buffer.size(),
			std::pmr::new_delete_resource()
		);

		static thread_local std::pmr::unsynchronized_pool_resource pool(&fixed_upstream);
		static thread_local std::pmr::polymorphic_allocator<Backend::Resource::Buffer> pmr_alloc(&pool);

		std::shared_ptr<Backend::Resource::Buffer> vma_buf(
			pmr_alloc.new_object<Backend::Resource::Buffer>(buf_info, buf, alloc, alloc_info),
			[mem_alloc = ld.mem_alloc](Backend::Resource::Buffer* buf) {
				vmaDestroyBuffer(mem_alloc.get(), buf->vk_handle, buf->alloc);
				pmr_alloc.delete_object(buf);
			}
		);

		return { std::move(vma_buf) };

	}

	Backend::Resource Backend::CreateTexture(LogicalDevice const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags) {
		
		VmaAllocationCreateInfo alloc_create_info{
			ExtractAllocationFlags(flags),
			ExtractMemoryUsage(flags),
			0, 0, 0, nullptr, nullptr, 0.0f
		};

		vk::ImageType dim = ExtractTextureDimension(flags);

		VkImageCreateInfo tex_info(
			vk::ImageCreateInfo(
				{},
				dim,
				ExtractFormat(flags),
				{ static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), dim == vk::ImageType::e3D ? static_cast<std::uint32_t>(depth_arr_layers) : 1u },
				static_cast<std::uint32_t>(mip_lvl_cnt),
				dim == vk::ImageType::e3D ? 1u : static_cast<std::uint32_t>(depth_arr_layers),
				ExtractSampleCount(flags),
				ExtractTiling(flags),
				ExtractTextureUsage(flags),
				vk::SharingMode::eExclusive,
				0u,
				nullptr,
				vk::ImageLayout::eUndefined,
				nullptr
			)
		);

		VkImage tex;
		VmaAllocation alloc;
		VmaAllocationInfo alloc_info;

		auto result = static_cast<vk::Result>(
			vmaCreateImage(
				ld.mem_alloc.get(),
				&tex_info,
				&alloc_create_info,
				&tex,
				&alloc,
				&alloc_info
			));

		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("Calling vmaCreateImage() failed, VMA reported: {}", vk::to_string(result)));
		}

		static thread_local std::array<std::byte, 1024 * sizeof(Backend::Resource::Texture)> fixed_buffer;
		static thread_local std::pmr::monotonic_buffer_resource fixed_upstream(
			fixed_buffer.data(), fixed_buffer.size(),
			std::pmr::new_delete_resource()
		);

		static thread_local std::pmr::unsynchronized_pool_resource pool(&fixed_upstream);
		static thread_local std::pmr::polymorphic_allocator<Backend::Resource::Texture> pmr_alloc(&pool);

		RegisterTexture(tex);

		std::shared_ptr<Backend::Resource::Texture> vma_buf(
			pmr_alloc.new_object<Backend::Resource::Texture>(tex_info, tex, alloc, alloc_info),
			[mem_alloc = ld.mem_alloc](Backend::Resource::Texture* tex) {
				UnregisterTexture(tex->vk_handle);			
				vmaDestroyImage(mem_alloc.get(), tex->vk_handle, tex->alloc);
				pmr_alloc.delete_object(tex);
			}
		);

		return { std::move(vma_buf) };

	}

	Backend::View CreateTextureView(Backend::LogicalDevice const& ld, Backend::Resource const& res, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags) {

		vk::Image tex = std::visit(
			[](auto&& res) {
				using Res = std::decay_t<decltype(res)>;
				if constexpr (std::same_as<Res, Backend::Resource::Texture>) {
					return res.vk_handle;
				}
				else {
					throw std::invalid_argument("Not a valid texture");
				}
			},
			res.impl
		);

		vk::ImageViewCreateInfo info(
			{},
			tex,
			ExtractTextureViewType(flags),
			ExtractFormat(flags),
			{
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity
			},
			{
				vk::ImageAspectFlagBits::eColor,
				static_cast<std::uint32_t>(base_mip_lvl),
				static_cast<std::uint32_t>(mip_lvl_cnt),
				static_cast<std::uint32_t>(base_arr_layer),
				static_cast<std::uint32_t>(arr_layer_cnt)
			}
		);

	}

}
#endif // !defined(__APPLE__)