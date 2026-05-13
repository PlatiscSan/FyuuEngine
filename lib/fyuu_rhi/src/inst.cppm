module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <type_traits>
#include <vector>
#include <mutex>
#include <ranges>
#include <algorithm>
#include <iterator>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:instance;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :physical_device;
import :surface;

namespace fyuu_rhi {

	namespace details {

		template <class T> struct IsVector : public std::false_type {};

		template <class T, class Alloc> struct IsVector<std::vector<T, Alloc>> : public std::true_type {};

	}

	export template <class Backend> class Instance {
	private:
		typename Backend::Instance m_impl;
		static inline Instance* s_instance;

		template <class... Args>
		Instance(Args&&... args)
			: m_impl(Backend::CreateInstance(std::forward<Args>(args)...)) {

		}

	public:
		template <class... Args>
		static void Initialize(Args&&... args) {
			static Instance instance(std::forward<Args>(args)...);
			static std::once_flag init_flag;
			std::call_once(
				init_flag,
				[]() {
					s_instance = &instance;
				}
			);
		}

		static Instance* Get() noexcept {
			return s_instance;
		}

		~Instance() noexcept {
			if constexpr (requires{ Backend::DestroyInstance(m_impl); }) {
				Backend::DestroyInstance(m_impl);
			}
		}

		auto EnumeratePhysicalDevices() const {

			using Ret = decltype(Backend::EnumeratePhysicalDevices(m_impl));
			using PhysDev = PhysicalDevice<Backend>;

			if constexpr (details::IsVector<Ret>::value) {
				using Vector = Ret;
				using Element = typename Vector::value_type;
				static_assert(std::constructible_from<PhysDev, Element>, 
					"PhysicalDevice<Backend> must be constructible from physical device returned by EnumeratePhysicalDevices()");

				auto devices = Backend::EnumeratePhysicalDevices(m_impl);
				std::vector<PhysDev> result;
				result.reserve(devices.size());
				std::ranges::transform(
					devices, std::back_inserter(result),
					[](auto&& dev) { 
						return PhysDev{ dev }; 
					}
				);
				return result;

			}
			else {
				static_assert(std::constructible_from<PhysDev, Ret>, 
					"PhysicalDevice<Backend> must be constructible from physical device returned by EnumeratePhysicalDevices()");
				return PhysDev{ Backend::EnumeratePhysicalDevices(m_impl) };
			}

		}

		template <class... PlatformHandles>
		Surface<Backend> CreateSurface(PlatformHandles&&... platform_handles) {
			using Ret = decltype(Backend::CreateSurface(m_impl, std::forward<PlatformHandles>(platform_handles)...));
			static_assert(std::constructible_from<Surface<Backend>, Ret>, 
				"PhysicalDevice<Backend> must be constructible from physical device returned by EnumeratePhysicalDevices()");
			return Backend::CreateSurface(m_impl, std::forward<PlatformHandles>(platform_handles)...);
		}

		void ShareContextOnThisThread() {
			if constexpr (requires{ Backend::ShareContextOnThisThread(m_impl); }) {
				Backend::ShareContextOnThisThread(m_impl);
			}
		}

	};
	
} // namespace fyuu_rhi
