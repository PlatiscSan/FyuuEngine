export module layer;
import std;

export namespace core {

	class ILayer {
	public:
		virtual ~ILayer() noexcept = default;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Update() = 0;
		virtual void Render() = 0;
	};

}