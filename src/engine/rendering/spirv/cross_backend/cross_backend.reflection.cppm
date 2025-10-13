export module cross_backend:reflection;
export import spirv;
import std;
import <spirv_cross.hpp>;

namespace fyuu_engine::spirv {

	export class CrossReflection final : public IReflection {
	private:
		spirv_cross::Compiler m_compiler;

	public:
		explicit CrossReflection(SPIRVCodeView spirv_code);
		explicit CrossReflection(SPIRVBinaryView spirv_binary);

		std::vector<EntryPoint> QueryEntryPoints() const override;

		std::vector<ResourceDescription> QueryResources(EntryPoint const& entry) override;
	};

}