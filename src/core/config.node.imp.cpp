module config:node;
import std;

namespace core {

	std::vector<std::string> ConfigNode::SplitSegment(std::string_view segment) {
		std::vector<std::string> parts;
		std::string::size_type start = 0;
		std::string::size_type end = segment.find('.');

		while (end != std::string::npos) {
			parts.emplace_back(segment.substr(start, end - start));
			start = end + 1;
			end = segment.find('.', start);
		}

		parts.emplace_back(segment.substr(start));
		return parts;
	}

	std::pair<ConfigNode::Map*, std::string> ConfigNode::TraverseForWrite(std::vector<std::string> const& segments) {

		if (segments.empty()) {
			throw std::invalid_argument("Empty key path");
		}

		Map* current = &m_root;
		for (size_t i = 0; i < segments.size() - 1; ++i) {
			auto const& seg = segments[i];
			auto it = current->find(seg);

			if (it == current->end()) {
				auto [new_it, _] = current->try_emplace(seg, ConfigNode());
				current = &std::get<ConfigNode>(new_it->second).m_root;
			}
			else {
				if (!std::holds_alternative<ConfigNode>(it->second)) {
					throw std::runtime_error("Path segment '" + seg + "' is not a ConfigNode");
				}
				current = &std::get<ConfigNode>(it->second).m_root;
			}
		}

		return { current, segments.back() };

	}

	std::pair<ConfigNode::Map const*, std::string> ConfigNode::TraverseForRead(std::vector<std::string> const& segments) const {
		if (segments.empty()) {
			return { nullptr, "" };
		}

		const Map* current = &m_root;
		for (size_t i = 0; i < segments.size() - 1; ++i) {
			auto const& seg = segments[i];
			auto it = current->find(seg);

			if (it == current->end() ||
				!std::holds_alternative<ConfigNode>(it->second)) {
				return { nullptr, "" };
			}
			current = &std::get<ConfigNode>(it->second).m_root;
		}
		return { current, segments.back() };
	}

	ConfigNode::Map const& ConfigNode::Root() const noexcept {
		return m_root;
	}

}