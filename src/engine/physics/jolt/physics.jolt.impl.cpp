module;
#include <jolt/Jolt.h>
#include <jolt/Physics/PhysicsSystem.h>
#include <jolt/RegisterTypes.h>

module physics.jolt;
import math.eigen;
import config.yaml;

namespace fyuu_engine::physics {

	class BroadPhaseLayerImpl final : public JPH::BroadPhaseLayerInterface {
	public:
		uint32_t GetNumBroadPhaseLayers() const override { 
			return 0;
		}

		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
			return {};
		}
	};

	class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
	public:
		bool ShouldCollide([[maybe_unused]] JPH::ObjectLayer in_object1, [[maybe_unused]] JPH::BroadPhaseLayer in_object2) const override {
			return true;
		}
	};

	class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
	public:
		bool ShouldCollide(JPH::ObjectLayer in_object1, JPH::ObjectLayer in_object2) const override {
			return true;
		}
	};

	JPH::BroadPhaseLayerInterface& JoltBroadPhaseLayerInterface() noexcept {
		static BroadPhaseLayerImpl impl{};
		return impl;
	}

	ObjectVsBroadPhaseLayerFilterImpl& JoltObjectVsBroadPhaseLayerFilterImpl() noexcept {
		static ObjectVsBroadPhaseLayerFilterImpl impl{};
		return impl;
	}

	ObjectLayerPairFilterImpl& JoltObjectLayerPairFilterImpl() noexcept {
		static ObjectLayerPairFilterImpl impl;
		return impl;
	}

	void A() {

		struct PhysicsSceneTraits {
			using Vector = math::EigenVector3;
			using Matrix = math::EigenMatrix4x4;
			using Quaternion = math::EigenQuaternion;
			using Config = config::YAMLConfig;
		};

		using PhysicsScence = JoltPhysicsScene<PhysicsSceneTraits>;
		PhysicsScence test_scene{ PhysicsSceneTraits::Vector() };
		test_scene.GetGravity();
		test_scene.CreateRigidBody(PhysicsScence::Transform{ math::EigenVector3{},math::EigenVector3{},math::EigenQuaternion{} }, PhysicsScence::RigidBodyComponentResource{});

	}

}