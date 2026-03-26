module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_swap_chain;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :webgpu_common;
import :webgpu_future;

namespace fyuu_rhi::webgpu {

    export class WebGPUSwapChain
        : public PolymorphicSwapChainBase,
          public WebGPUCommon<WebGPUSwapChain> {
    private:
    
    public:
        template <class... Args>
        WebGPUSwapChain(Args&&... args)
            : PolymorphicSwapChainBase(this),
            WebGPUCommon(this) {
            // placeholder constructor
        }

        void Resize(std::optional<std::size_t> const& back_buffer_size = std::nullopt);

		std::tuple<WebGPUResource*, WebGPUView*, std::shared_ptr<WebGPUFuture>> GetNextFrame();

		void Present(std::shared_ptr<WebGPUFuture> const& future = nullptr);

		void SetVSync(bool mode);


    };

} // namespace fyuu_rhi::webgpu