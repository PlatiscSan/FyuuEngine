#ifndef VULKAN_APPLICATION_BUILDER_H
#define VULKAN_APPLICATION_BUILDER_H

#include "Framework/Application/Application.h"

namespace Fyuu {

	class VulkanApplicationBuilder final : public ApplicationBuilder {

	public:

		VulkanApplicationBuilder() = default;
		~VulkanApplicationBuilder() noexcept = default;

		void BuildBasicSystem() override;
		void BuildMainWindow() override;
		void BuildRenderer() override;
		void BuildTick() override;
	};

}

#endif // !VULKAN_APPLICATION_BUILDER_H
