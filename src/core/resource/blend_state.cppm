export module core:blend_state;
import std;
import :animation_clip;
import :animation_skeleton_node_map;

namespace fyuu_engine::core::resource {

	struct BoneBlendWeight {
		std::vector<float> weights;
	};

	struct BlendStateWithClipData {
		std::size_t clip_count;
		std::vector<AnimationClip> blend_clips;
		std::vector<AnimationSkeletonNodeMap> blend_anim_skel_map;
		std::vector<BoneBlendWeight> blend_weights;
		std::vector<float> blend_ratio;
	};

	struct BlendState {
		std::size_t clip_count;
		std::vector<std::filesystem::path> blend_clip_file_paths;
		std::vector<std::filesystem::path> blend_anim_skel_map_file_paths;
		std::vector<std::filesystem::path> blend_mask_file_paths;
		std::vector<float> blend_weights;
		std::vector<float> blend_ratio;
	};

}