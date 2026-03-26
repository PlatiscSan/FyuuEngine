/* vulkan_swap_chain.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <stdexcept>
#include <memory>
#include <tuple>
#include <vector>
#include <span>
#include <limits>
#include <algorithm>
#endif // !defined(__cpp_lib_modules)
#include "declare_pool.hpp"
module fyuu_rhi:vulkan_swap_chain_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_swap_chain;
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_physical_device;
import :vulkan_logical_device;
import :vulkan_command_queue;
import :vulkan_surface;
import :vulkan_resource;
import :vulkan_view;
import :vulkan_types;

namespace {

	vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
		std::span<vk::SurfaceFormatKHR> available_formats,
		std::optional<vk::Format> const& desired_format = std::nullopt
	) {

		if (desired_format) {
			for (vk::SurfaceFormatKHR const& fmt : available_formats) {
				if (fmt.format == *desired_format && fmt.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
					return fmt;
				}
			}
			for (vk::SurfaceFormatKHR const& fmt : available_formats) {
				if (fmt.format == *desired_format) {
					return fmt;
				}
			}
		}

		for (vk::SurfaceFormatKHR const& available_format : available_formats) {
			if (available_format.format == vk::Format::eB8G8R8A8Unorm &&
				available_format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
				return available_format;
			}
		}

		return available_formats[0];
	}

	vk::PresentModeKHR ChooseSwapPresentMode(std::span<vk::PresentModeKHR> available_present_modes, bool v_sync) {

		auto query_mode = [available_present_modes](vk::PresentModeKHR desired_mode) -> std::optional<vk::PresentModeKHR> {
			for (vk::PresentModeKHR const& available_present_mode : available_present_modes) {
				if (available_present_mode == desired_mode) {
					return desired_mode;    // use Mailbox on high-perf devices
				}
			}
			return std::nullopt;
			};

		std::optional<vk::PresentModeKHR> desired_mode = v_sync ? 
			query_mode(vk::PresentModeKHR::eMailbox) : 
			query_mode(vk::PresentModeKHR::eImmediate);

		return desired_mode ? *desired_mode : vk::PresentModeKHR::eFifo;

	}

	vk::Extent2D ChooseSwapExtent(vk::SurfaceCapabilitiesKHR& capabilities, std::uint32_t width, std::uint32_t height) {

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			vk::Extent2D actual_extent(width, height);

			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	};

} // namespace


namespace fyuu_rhi::vulkan {

	VulkanSwapChain::Frame::Frame(VulkanLogicalDevice const& logical_device, vk::Image back_buffer, vk::Format back_buffer_format, std::size_t width, std::size_t height, std::size_t index)
		: texture(back_buffer, back_buffer_format, width, height),
		view(logical_device, texture, index),
		future(nullptr) {
	}

	VulkanSwapChain::Promise::Promise(VulkanLogicalDevice const& logical_device)
		: impl(logical_device, VulkanPromise::UseFence{}),
		acquired_frame_index() {
	}

	void VulkanSwapChain::CreateSwapChain(
		VulkanPhysicalDevice const& physical_device,
		VulkanLogicalDevice const& logical_device,
		VulkanCommandQueue const& present_queue,
		VulkanSurface const& surface,
		std::size_t back_buffer_size
	) {

		vk::Bool32 present_support = physical_device.IsQueueSupportsPresenting(
			present_queue.GetFamily(),
			surface.GetNative()
		);

		if (!present_support) {
			throw std::runtime_error("The queue does not support presentation to the given surface.");
		}

		VulkanSwapChainSupportDetails swap_chain_support = physical_device.QuerySwapChainSupport(surface.GetNative());
		
		std::optional<vk::Format> desired_format;
		if (auto vk_fmt = DetermineFormat(m_option.back_buffer_flags); vk_fmt != vk::Format::eUndefined) {
			desired_format = vk_fmt;
		}
	
		vk::SurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(
			swap_chain_support.formats,
			desired_format
		);

		vk::PresentModeKHR main_present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes, m_option.enable_v_sync);

		auto [width, height] = surface.GetSize();

		vk::Extent2D extent = ChooseSwapExtent(swap_chain_support.capabilities, width, height);

		std::uint32_t desired_image_count = back_buffer_size > 0 ?
			back_buffer_size :
			swap_chain_support.capabilities.minImageCount + 1;

		std::uint32_t image_count = std::clamp(
			desired_image_count,
			swap_chain_support.capabilities.minImageCount,
			swap_chain_support.capabilities.maxImageCount > 0 ? swap_chain_support.capabilities.maxImageCount : std::numeric_limits<std::uint32_t>::max()
		);

		std::vector<vk::PresentModeKHR> allowed_modes = physical_device.GetCompatiblePresentModes(
			surface.GetNative(),
			main_present_mode
		);

		vk::SwapchainPresentModesCreateInfoEXT present_modes_info(allowed_modes);

		auto& [current_swap_chain, current_format, current_extent] = m_swap_chain_bundle;

		vk::SwapchainCreateInfoKHR swap_chain_create_info(
			vk::SwapchainCreateFlagBitsKHR::eDeferredMemoryAllocationEXT,
			surface.GetNative(),
			image_count,
			surface_format.format,
			surface_format.colorSpace,
			extent,
			1,													// always 1 unless we are doing stereoscopic 3D
			vk::ImageUsageFlagBits::eColorAttachment,			// use vk::ImageUsageFlagBits::eColorAttachment for offscreen rendering
			vk::SharingMode::eExclusive,						// set the flag the later
			{},
			swap_chain_support.capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			main_present_mode,
			vk::True,											// we don't care about pixels that are obscured
			current_swap_chain ? *current_swap_chain : nullptr,
			&present_modes_info
		);

		std::uint32_t graphics_queue_index = logical_device.CommandObjectTypeToQueueFamily(CommandObjectType::AllCommands);
		std::uint32_t present_queue_index = present_queue.GetIndex();

		if (graphics_queue_index != present_queue_index) {
			std::array queue_family_indices = { graphics_queue_index, present_queue_index };
			swap_chain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
			swap_chain_create_info.queueFamilyIndexCount = queue_family_indices.size();
			swap_chain_create_info.pQueueFamilyIndices = queue_family_indices.data();
		}

		current_swap_chain = logical_device.CreateSwapChain(swap_chain_create_info);
		current_format = surface_format.format;
		current_extent = extent;

		m_allowed_modes = std::move(allowed_modes);
		m_main_present_mode = main_present_mode;

	}

	void VulkanSwapChain::CreateRenderTarget(VulkanLogicalDevice const& logical_device, std::size_t back_buffer_size) {
		
		auto& [current_swap_chain, current_format, current_extent] = m_swap_chain_bundle;

		std::vector<vk::Image> back_buffers = logical_device.GetSwapChainBackBuffers(*current_swap_chain);
		std::size_t back_buffers_size = back_buffers.size();

		for (std::size_t i = 0; i < back_buffers_size; ++i) {
			vk::Image& back_buffer = back_buffers[i];
			m_frames.emplace_back(logical_device, back_buffer, current_format, current_extent.width, current_extent.height, i);
			m_internal_promises.emplace_back(logical_device);
		}

	}

	void VulkanSwapChain::Resize(
		VulkanPhysicalDevice const& physical_device,
		VulkanLogicalDevice const& logical_device,
		VulkanCommandQueue const& present_queue,
		VulkanSurface const& surface,
		std::size_t back_buffer_size
	) {
		present_queue.WaitUntilCompleted();
		m_frames.clear();
		m_internal_promises.clear();
		CreateSwapChain(physical_device, logical_device, present_queue, surface, back_buffer_size);
		CreateRenderTarget(logical_device, back_buffer_size);
		m_next_promise_index = 0u;
		m_current_promise_index.reset();
	}

	VulkanSwapChain::VulkanSwapChain(
		VulkanPhysicalDevice const& physical_device,
		VulkanLogicalDevice const& logical_device,
		VulkanCommandQueue const& present_queue,
		VulkanSurface const& surface,
		SwapChainOption const& swap_chain_option
	) : PolymorphicSwapChainBase(this),
		VulkanCommon(this),
		m_physical_device_id(static_cast<VulkanCommon<VulkanPhysicalDevice> const&>(physical_device).GetID()),
		m_logical_device_id(logical_device.GetID()),
		m_present_queue_id(present_queue.GetID()),
		m_surface_id(surface.GetID()),
		m_option(swap_chain_option),
		m_swap_chain_bundle(),
		m_allowed_modes(),
		m_main_present_mode(),
		m_frames(),
		m_next_promise_index(0u),
		m_current_promise_index() {
		Resize(physical_device, logical_device, present_queue, surface, swap_chain_option.back_buffer_size);
	}

	VulkanSwapChain::~VulkanSwapChain() noexcept {

		if (!m_present_queue_id) {
			return;
		}

		VulkanCommandQueue* present_queue = plastic::utility::QueryObject<VulkanCommandQueue>(*m_present_queue_id); 
		
		if (!present_queue) {
			return;
		}

		present_queue->WaitUntilCompleted();

	}

	void VulkanSwapChain::Resize(std::optional<std::size_t> const& back_buffer_size) {
#define CHECK_GET_VULKAN_OBJECT(id, Type, var, name)\
		if (!(id)) throw std::invalid_argument("Invalid " name);\
		Type* var = plastic::utility::QueryObject<Type>(*(id)); \
		if (!(var)) throw std::runtime_error(name " lost");

		CHECK_GET_VULKAN_OBJECT(m_physical_device_id, VulkanPhysicalDevice, physical_device, "physical device")
		CHECK_GET_VULKAN_OBJECT(m_logical_device_id, VulkanLogicalDevice, logical_device, "logical device")
		CHECK_GET_VULKAN_OBJECT(m_present_queue_id, VulkanCommandQueue, present_queue, "present queue")
		CHECK_GET_VULKAN_OBJECT(m_surface_id, VulkanSurface, surface, "surface")
	
		if (back_buffer_size) {
			Resize(*physical_device, *logical_device, *present_queue, *surface, *back_buffer_size);
			m_option.back_buffer_size = *back_buffer_size;
		}
		else {
			Resize(*physical_device, *logical_device, *present_queue, *surface, m_option.back_buffer_size);
		}

	}

	std::tuple<VulkanResource*, VulkanView*, std::shared_ptr<VulkanFuture>> VulkanSwapChain::GetNextFrame() {
		
		if (m_current_promise_index) {
			auto& frame_index = m_internal_promises[*m_current_promise_index].acquired_frame_index;
			if (frame_index) {
				auto& frame = m_frames[*frame_index];
				return { &frame.texture, &frame.view, frame.future };
			}
		}

		std::optional<std::size_t> frame_index;
		std::size_t current_promise_index;
		Promise* current_promise = nullptr;
		CHECK_GET_VULKAN_OBJECT(m_logical_device_id, VulkanLogicalDevice, logical_device, "logical device")

		do {

			current_promise_index = m_next_promise_index++;
			m_next_promise_index %= m_option.back_buffer_size;
			
			current_promise = &m_internal_promises[current_promise_index];
			auto future = current_promise->impl.GetFuture();
			if (future) {
				future->Wait();
			}

			frame_index = current_promise->impl.SwapChainBackBufferSignal(logical_device, this);
			if (!frame_index) {
				Resize();
			}

		} while (!frame_index);

		auto& future = m_frames[*frame_index].future;
		if (future) {
			future->Wait();
		}

		current_promise->acquired_frame_index = frame_index;
		m_current_promise_index = current_promise_index;

		Frame& frame = m_frames[*frame_index];
		frame.future = current_promise->impl.GetFuture();

		return { &frame.texture, &frame.view, frame.future };

	}

	void VulkanSwapChain::Present(std::shared_ptr<VulkanFuture> const& future) {

		DECLARE_POOL(wait_semaphores, vk::Semaphore, 4u)

		vk::PresentInfoKHR present_info;
		std::pmr::vector<vk::Semaphore> wait_semaphores(wait_semaphores_alloc);

		CHECK_GET_VULKAN_OBJECT(m_logical_device_id, VulkanLogicalDevice, logical_device, "logical device")
		CHECK_GET_VULKAN_OBJECT(m_present_queue_id, VulkanCommandQueue, present_queue, "present queue")

		auto& [swap_chain, _, __] = m_swap_chain_bundle;
		std::array swap_chains = { *swap_chain };
		present_info.setSwapchains(swap_chains);

		if (future) {
			wait_semaphores.emplace_back(future->GetBinarySemaphore());
		}

		if (!m_current_promise_index) {
			return;
		}

		Promise& current_promise = m_internal_promises[*m_current_promise_index];
		std::uint32_t frame_index = static_cast<std::uint32_t>(*current_promise.acquired_frame_index);
		Frame& current_frame = m_frames[frame_index];

		auto& frame_future = current_frame.future;
		if (frame_future) {
			frame_future->Wait();
			wait_semaphores.emplace_back(frame_future->GetBinarySemaphore());
		}

		if (!wait_semaphores.empty()) {
			present_info.setWaitSemaphores(wait_semaphores);
		}

		std::array back_buffer_indices = { frame_index };
		present_info.setImageIndices(back_buffer_indices);

		vk::PresentModeKHR desired_mode;
		if (m_option.enable_v_sync) {
			if (std::find(m_allowed_modes.begin(), m_allowed_modes.end(), vk::PresentModeKHR::eMailbox) != m_allowed_modes.end()) {
				desired_mode = vk::PresentModeKHR::eMailbox;
			} 
			else {
				desired_mode = vk::PresentModeKHR::eFifo;
			}
		} 
		else {
			desired_mode = vk::PresentModeKHR::eImmediate;
		}

		if (std::find(m_allowed_modes.begin(), m_allowed_modes.end(), desired_mode) == m_allowed_modes.end()) {
			desired_mode = m_main_present_mode;
		}

		std::array present_modes = { desired_mode };

		// present_info -> present_mode_info -> present_fence_info -> nullptr

		vk::SwapchainPresentFenceInfoEXT present_fence_info{ 1u, nullptr, nullptr };
		current_promise.impl.PresentingSignal(present_fence_info);

		vk::SwapchainPresentModeInfoEXT present_mode_info(present_modes, &present_fence_info);
		present_info.setPNext(&present_mode_info);

		bool result = present_queue->Present(present_info);
		if (!result) {
			Resize();
			return;
		}
		
		current_promise.acquired_frame_index.reset();

		logical_device->VMATick();

	}

	void VulkanSwapChain::SetVSync(bool mode) {
		m_option.enable_v_sync = mode;
	}

	vk::SwapchainKHR VulkanSwapChain::GetNative() const noexcept {
		auto& [swap_chain, _, __] = m_swap_chain_bundle;
		return *swap_chain;
	}

}

#endif // !defined(__APPLE__)