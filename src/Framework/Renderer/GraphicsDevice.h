#ifndef GRAPHICS_DEVICE_H
#define GRAPHICS_DEVICE_H

#include <memory>
#include <string>

namespace Fyuu {

	class GraphicsDevice {

	public:

		virtual ~GraphicsDevice() noexcept = default;

		using GraphicsDevicePtr = std::shared_ptr<GraphicsDevice>;

	protected:

		std::string m_device_name;
		std::string m_device_vendor;

	};

}

#endif // !GRAPHICS_DEVICE_H
