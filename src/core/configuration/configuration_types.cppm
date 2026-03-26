/* configuration_types.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <ctime>
#endif // !defined(__cpp_lib_modules)
export module fyuu_engine:configuration_types;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.sealed_polymorphism;

namespace fyuu_engine::configuration {

	namespace json {
		export class JSON;
		export class JSONProxy;
	}

	namespace yaml {
		export class YAML;
		export class YAMLProxy;
	}

	using PolymorphicProxyBase = plastic::utility::PolymorphicBase<
		std::size_t,
		json::JSONProxy,
		yaml::YAMLProxy
	>;

	using PolymorphicConfigurationBase = plastic::utility::PolymorphicBase<
		std::size_t,
		json::JSON,
		yaml::YAML
	>;

}