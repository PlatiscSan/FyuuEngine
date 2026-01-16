export module fyuu_rhi:swap_chain;
import std;
import plastic.disable_copy;

namespace fyuu_rhi {

	export class ISurface
		: public plastic::utility::DisableCopy<ISurface> {
	public:
		std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeight(this auto&& self) {
			return self.GetWidthAndHeightImpl();
		}
	};

	export class ISwapChain 
		: public plastic::utility::DisableCopy<ISwapChain> {
	public:
		void Resize(this auto&& self, std::uint32_t width, std::uint32_t height) {
			self.ResizeImpl(width, height);
		}

		void Present(this auto&& self) {
			self.PresentImpl();
		}

		void EnableVSync(this auto&& self, bool mode = true) {
			self.EnableVSyncImpl(mode);
		}

	};

}