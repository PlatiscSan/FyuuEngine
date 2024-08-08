#ifndef OPENGL_APPLICATION_BUILDER_H
#define OPENGL_APPLICATION_BUILDER_H

#include "Framework/Application/Application.h"

namespace Fyuu {

	class OpenGLApplicationBuilder final : public ApplicationBuilder {

	public:

		OpenGLApplicationBuilder() = default;
		~OpenGLApplicationBuilder() noexcept = default;

		void BuildBasicSystem() override;
		void BuildMainWindow() override;
		void BuildRenderer() override;
		void BuildTick() override;
	};

}

#endif // !OPENGL_APPLICATION_BUILDER_H
