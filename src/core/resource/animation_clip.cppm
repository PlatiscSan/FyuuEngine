export module core:animation_clip;
import std;
import math;

namespace fyuu_engine::core::resource {

	struct AnimationNodeMap {
		std::vector<std::string> convert;
	};


	struct AnimationChannel {
		std::string node_name;
		std::vector<math::Vector3> positions;
		std::vector<math::Quaternion> rotations;
		std::vector<math::Vector3> scales;
	};

	struct AnimationClip {
		std::size_t total_frames;
		std::size_t node_count;
		std::vector<AnimationChannel> channels;
	};

	struct AnimationAsset {
		AnimationNodeMap node_map;
		std::vector<AnimationClip> clips;
		std::filesystem::path skeleton_file_path;
	};

}