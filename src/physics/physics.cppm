export module physics;
export import math;
export import config;
import std;

namespace fyuu_engine::physics {

	export template <
		class DerivedPhysicsScene, 
		class PhysicsSceneTraits
	>  class BasePhysicsScene {
	public:

		enum class RigidBodyShapeType : std::uint8_t {
			Unknown,
			Box,
			Sphere,
			Capsule,
		};
		using Vector = typename PhysicsSceneTraits::Vector;
		using Matrix = typename PhysicsSceneTraits::Matrix;
		using Quaternion = typename PhysicsSceneTraits::Quaternion;
		using Transform = math::Transform<Vector, Matrix, Quaternion>;
		using AxisAlignedBox = math::AxisAlignedBox<Vector>;

		struct RigidBodyShape {
			Transform global_transform;
			AxisAlignedBox bounding_box;
			RigidBodyShapeType type = RigidBodyShapeType::Unknown;
		};

		struct RigidBodyComponentResource {
			std::vector<RigidBodyShape> shapes;
			float inverse_mass;
			int actor_type;
		};

		struct PhysicsHitInfo {
			Vector hit_position;
			Vector hit_normal;
			std::optional<std::uint32_t> body_id;
			float hit_distance = 0.0f;
		};

	protected:

		Vector m_gravity;

	public:
		BasePhysicsScene(Vector const& gravity)
			: m_gravity(gravity) {

		}

		Vector GetGravity() const noexcept {
			return m_gravity;
		}

		std::uint32_t CreateRigidBody(Transform const& global_transform, RigidBodyComponentResource const& rigidbody_actor_resource) {
			return static_cast<DerivedPhysicsScene*>(this)->CreateRigidBodyImpl(global_transform, rigidbody_actor_resource);
		}

		void RemoveRigidBody(std::uint32_t body_id) {
			static_cast<DerivedPhysicsScene*>(this)->RemoveRigidBodyImpl(body_id);
		}

		void UpdateRigidBodyGlobalTransform(std::uint32_t body_id, Transform const& global_transform) {
			static_cast<DerivedPhysicsScene*>(this)->UpdateRigidBodyGlobalTransformImpl(body_id, global_transform);
		}

		void Tick() const {
			static_cast<DerivedPhysicsScene*>(this)->TickImpl();
		}

		std::vector<PhysicsHitInfo> RayCast(Vector const& ray_origin, Vector const& ray_direction, float ray_length) const {
			return static_cast<DerivedPhysicsScene*>(this)->RayCastImpl(ray_origin, ray_direction, ray_length);
		}

		std::vector<PhysicsHitInfo> Sweep(
			RigidBodyShape const& shape,
			Matrix const& shape_transform,
			Vector const& sweep_direction,
			float sweep_length
		) const {
			return static_cast<DerivedPhysicsScene*>(this)->SweepImpl(shape, shape_transform, sweep_direction, sweep_length);
		}

		bool IsOverlap(RigidBodyShape const& shape, Matrix const& global_transform) const {
			return static_cast<DerivedPhysicsScene*>(this)->IsOverlapImpl(shape, global_transform);
		}

		std::vector<AxisAlignedBox> GetShapeBoundingBoxes(std::uint32_t body_id) const {
			return static_cast<DerivedPhysicsScene*>(this)->GetShapeBoundingBoxesImpl(body_id);
		}

	};

}