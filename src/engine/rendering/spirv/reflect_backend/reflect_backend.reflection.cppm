export module reflect_backend:reflection;
export import spirv;
export import <spirv_reflect.h>;
import std;

namespace fyuu_engine::spirv {

	export class ReflectReflection final : public IReflection {
	private:
		spv_reflect::ShaderModule m_module;

	public:
		explicit ReflectReflection(SPIRVCodeView spirv_code);
		explicit ReflectReflection(SPIRVBinaryView spirv_binary);

		std::vector<EntryPoint> QueryEntryPoints() const override;
		std::vector<ResourceDescription> QueryResources(EntryPoint const& entry) override;
	};

}