export module config;
export import :node;
export import :json;
export import :yaml;

namespace fyuu_engine::config {

	export using Node = ConfigNode;
	export using Arithmetic = util::Arithmetic;
	export using NodeValue = ConfigNode::Value;
	export using ArrayElement = util::ArrayElement;
	export using Array = util::Array;

	export using YAMLConfig = YAMLConfig;
	export using JSONConfig = JSONConfig;

}