#ifndef D3D12_APPLICATION_BUILDER_H
#define D3D12_APPLICATION_BUILDER_H

#include "Framework/Application/Application.h"

namespace Fyuu {

	class D3D12ApplicationBuilder final : public ApplicationBuilder {

	public:

		D3D12ApplicationBuilder() = default;
		~D3D12ApplicationBuilder() noexcept = default;

		void BuildBasicSystem() override;
		void BuildMainWindow() override;
		void BuildRenderer() override;
		void BuildTick() override;


	};

}

#endif // !D3D12_APPLICATION_BUILDER_H
