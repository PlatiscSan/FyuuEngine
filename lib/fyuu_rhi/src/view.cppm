export module fyuu_rhi:view;
import std;

namespace fyuu_rhi {

	enum class ViewType : std::uint8_t {
		Unknown,
		VertexBufferView,
		IndexBufferView
	};

	export class IView {

	};

}