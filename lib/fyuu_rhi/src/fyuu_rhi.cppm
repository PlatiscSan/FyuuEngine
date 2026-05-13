module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <vector>
#include <span>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
export import :log;
export import :core_types;
#if defined(_WIN32)
import :d3d12_traits;
#endif // defined(_WIN32)
#if defined(__APPLE__)
import :metal_traits;
#else
import :vulkan_traits;
import :opengl_traits;
#endif // defined(__APPLE__)
import :webgpu_traits;

import :physical_device;
import :instance;
import :physical_device;
import :surface;
import :logical_device;
import :future;
import :resource;

namespace fyuu_rhi {
	
#if defined(_WIN32)
	export using D3D12Instance = Instance<d3d12::Backend>;
	export using D3D12PhysicalDevice = PhysicalDevice<d3d12::Backend>;
	export using D3D12Surface = Surface<d3d12::Backend>;
	export using D3D12LogicalDevice = LogicalDevice<d3d12::Backend>;
	export using D3D12Promise = Promise<d3d12::Backend>;
	export using D3D12Future = Future<d3d12::Backend>;
	export using D3D12Resource = Resource<d3d12::Backend>;
#endif // defined(_WIN32)
#if defined(__APPLE__)
	export using MetalInstance = Instance<metal::Backend>;
	export using MetalPhysicalDevice = PhysicalDevice<metal::Backend>;
	export using MetalSurface = Surface<metal::Backend>;
	export using MetalLogicalDevice = LogicalDevice<metal::Backend>;
	export using MetalPromise = Promise<metal::Backend>;
	export using MetalFuture = Future<metal::Backend>;
	export using D3D12Resource = Resource<d3d12::Backend>;
#else
	export using Version = vulkan::Version;
	export using VulkanInstance = Instance<vulkan::Backend>;
	export using VulkanPhysicalDevice = PhysicalDevice<vulkan::Backend>;
	export using VulkanSurface = Surface<vulkan::Backend>;
	export using VulkanLogicalDevice = LogicalDevice<vulkan::Backend>;
	export using VulkanPromise = Promise<vulkan::Backend>;
	export using VulkanFuture = Future<vulkan::Backend>;
	export using VulkanResource = Resource<vulkan::Backend>;

	export using OpenGLInstance = Instance<opengl::Backend>;
	export using OpenGLPhysicalDevice = PhysicalDevice<opengl::Backend>;
	export using OpenGLSurface = Surface<opengl::Backend>;
	export using OpenGLLogicalDevice = LogicalDevice<opengl::Backend>;
	export using OpenGLPromise = Promise<opengl::Backend>;
	export using OpenGLFuture = Future<opengl::Backend>;
	export using OpenGLResource = Resource<opengl::Backend>;
#endif // defined(__APPLE__)
	export using WebGPUInstance = Instance<webgpu::Backend>;
	export using WebGPUPhysicalDevice = PhysicalDevice<webgpu::Backend>;
	export using WebGPUSurface = Surface<webgpu::Backend>;
	export using WebGPULogicalDevice = LogicalDevice<webgpu::Backend>;
	export using WebGPUPromise = Promise<webgpu::Backend>;
	export using WebGPUFuture = Future<webgpu::Backend>;
	export using WebGPUResource = Resource<webgpu::Backend>;

	export template <class T> T BestPerformance(std::span<T const> phys_devs) {

		if (phys_devs.empty()) {
			throw std::invalid_argument("BestPerformace(): no available physical devices");
		}

		// Helper to convert device type to a comparable score.
		static auto TypeScore = [](PhysicalDeviceInfo::Type type) -> std::size_t {
			using Type = PhysicalDeviceInfo::Type;
			switch (type) {
			case Type::DiscreteGPU:
				return 4u;
			case Type::IntegratedGPU:
				return 3u;
			case Type::CPU:
				return 2u;
			case Type::Virtual:
				return 1u;
			default:
				return 0u; // Unknown
			}
			};

		std::size_t best_idx = 0;
		static_assert(std::same_as<decltype(phys_devs[0].GetInfo()), PhysicalDeviceInfo>,
			"GetInfo() does not return PhysicalDeviceInfo");
			
		auto best_info = phys_devs[0].GetInfo();
		std::size_t best_score = TypeScore(best_info.type);
		std::size_t best_mem = best_info.dedicated_memory.value_or(0);

		for (std::size_t i = 1; i < phys_devs.size(); ++i) {
			auto info = phys_devs[i].GetInfo();
			std::size_t score = TypeScore(info.type);
			std::size_t mem = info.dedicated_memory.value_or(0);

			if (score > best_score || (score == best_score && mem > best_mem)) {
				best_idx = i;
				best_score = score;
				best_mem = mem;
			}
		}

		return phys_devs[best_idx];
	}

	export template <class T> T BestPerformance(std::vector<T> const& phys_devs) {
		return BestPerformance(std::span<T const>(phys_devs));
	}

	export template <class T> T BestPerformance(std::vector<T>& phys_devs) {
		return BestPerformance(const_cast<std::vector<T> const&>(phys_devs));
	}

	export template <class T> T BestPerformance(T& phys_devs) {
		return phys_devs;
	}

} // namespace fyuu_rhi
