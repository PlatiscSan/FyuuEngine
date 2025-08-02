export module layer_interface;
import std;

export namespace core {

	class ILayer {
	public:
		virtual void Attach() = 0;
		virtual void Detach() = 0;
		virtual void Update() = 0;
		virtual void Render() = 0;
	};

}