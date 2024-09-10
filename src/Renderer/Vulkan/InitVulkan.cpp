
#include "pch.h"
#include "../WindowsGameCore.h"
#include "Framework/Graphics/GraphicsContext.h"
#include "VulkanGraphicsDevice.h"

using namespace Fyuu::core;
using namespace Fyuu::graphics;

void Fyuu::core::InitVulkan(int argc, char** argv) {

	auto device = std::make_shared<VulkanGraphicsDevice>();

	SetGraphicsDevice(device);

}