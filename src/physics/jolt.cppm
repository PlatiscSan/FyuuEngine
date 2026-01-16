module;
#include <jolt/Jolt.h>
#include <jolt/Physics/PhysicsSystem.h>
#include <jolt/RegisterTypes.h>
#include <jolt/Core/JobSystemThreadPool.h>

export module physics.jolt;
export import physics;

namespace fyuu_engine::physics {

	class BroadPhaseLayerImpl;
	class ObjectVsBroadPhaseLayerFilterImpl;
	class ObjectLayerPairFilterImpl;

	JPH::BroadPhaseLayerInterface& JoltBroadPhaseLayerInterface() noexcept;
	ObjectVsBroadPhaseLayerFilterImpl& JoltObjectVsBroadPhaseLayerFilterImpl() noexcept;
	ObjectLayerPairFilterImpl& JoltObjectLayerPairFilterImpl() noexcept;

	export template <
		class PhysicsSceneTraits
	> class JoltPhysicsScene : public BasePhysicsScene<JoltPhysicsScene<PhysicsSceneTraits>, PhysicsSceneTraits> {
		friend class BasePhysicsScene<JoltPhysicsScene<PhysicsSceneTraits>, PhysicsSceneTraits>;
	public:
		using Base = BasePhysicsScene<JoltPhysicsScene<PhysicsSceneTraits>, PhysicsSceneTraits>;

	private:
		/*
		*	single Jolt physics system is used for each scene
		*/

		static inline std::shared_ptr<JPH::PhysicsSystem> s_jolt_physics_system;

		static inline std::shared_ptr<JPH::JobSystem> s_jolt_job_system;
		static inline std::shared_ptr<JPH::TempAllocator> s_temp_allocator;

		static inline std::atomic_size_t s_ref_count;
		static inline std::atomic_flag s_flag;

		static bool BroadPhaseCanCollide(JPH::ObjectLayer in_layer1, JPH::BroadPhaseLayer in_layer2) {

		}

		static bool ObjectCanCollide(JPH::ObjectLayer in_object1, JPH::ObjectLayer in_object2) {

		}

		static void InitJoltPhysicsSystem(typename PhysicsSceneTraits::Vector const& gravity, typename PhysicsSceneTraits::Config const& config) {
			
			JPH::Factory::sInstance = new JPH::Factory();
			JPH::RegisterTypes();

			s_jolt_physics_system = std::make_shared<JPH::PhysicsSystem>();
			s_jolt_job_system = std::make_shared<JPH::JobSystemThreadPool>(
				config["jolt"]["max_job"].GetOr(1024u),
				config["jolt"]["max_barrier"].GetOr(8u),
				config["jolt"]["max_concurrent_job"].GetOr(4)
			);


			// 16M temp memory
			s_temp_allocator = std::make_shared<JPH::TempAllocatorImpl>(16u * 1024u * 1024u);

			s_jolt_physics_system->Init(
				config["jolt"]["max_body"].GetOr(10240u),
				config["jolt"]["body_mutex"].GetOr(0u),
				config["jolt"]["max_body_pairs"].GetOr(65536u),
				config["jolt"]["max_contact_constraints"].GetOr(10240u),
				JoltBroadPhaseLayerInterface(),
				JoltObjectVsBroadPhaseLayerFilterImpl(),
				JoltObjectLayerPairFilterImpl()
			);

			s_jolt_physics_system->SetPhysicsSettings(JPH::PhysicsSettings());
			s_jolt_physics_system->SetGravity(JPH::Vec3(gravity[0], gravity[1], gravity[2]));

			s_flag.test_and_set(std::memory_order::release);

		}

		static void DestroyPhysicsSystem() {


		}

		std::uint32_t CreateRigidBodyImpl(
			typename Base::Transform const& global_transform, 
			typename Base::RigidBodyComponentResource const& rigid_body_actor_resource
		) {
			
			JPH::BodyInterface& body_interface = s_jolt_physics_system->GetBodyInterface();

			struct JPHShapeData {
				JPH::Shape* shape{ nullptr };
				typename Base::Transform	local_transform;
				typename Base::Vector		global_position;
				typename Base::Vector		global_scale;
				typename Base::Quaternion	global_rotation;
			};

			std::vector<JPHShapeData> jph_shapes;
			std::vector<typename Base::RigidBodyShape> const& shapes = rigid_body_actor_resource.shapes;
			for (auto const& shape : shapes) {

				typename Base::Matrix shape_global_transform = global_transform * shape.local_transform;

				typename Base::Vector global_position, global_scale;
				typename Base::Quaternion global_rotation;


			}



		}

		void RemoveRigidBodyImpl(std::uint32_t body_id) {

		}

		void UpdateRigidBodyGlobalTransformImpl(std::uint32_t body_id, Base::Transform const& global_transform) {

		}

		void TickImpl() const {

		}

		std::vector<typename Base::PhysicsHitInfo> RayCastImpl(
			Base::Vector const& ray_origin, 
			Base::Vector const& ray_direction,
			float ray_length
		) const {

		}

		std::vector<typename Base::PhysicsHitInfo> SweepImpl(
			Base::RigidBodyShape const& shape,
			Base::Matrix const& shape_transform,
			Base::Vector const& sweep_direction,
			float sweep_length
		) const {

		}

		bool IsOverlapImpl(Base::RigidBodyShape const& shape, Base::Matrix const& global_transform) const {

		}

		std::vector<typename Base::AxisAlignedBox> GetShapeBoundingBoxesImpl(std::uint32_t body_id) const {
			static_cast<Base*>(this)->m_gravity;
		}


	public:
		JoltPhysicsScene(typename PhysicsSceneTraits::Vector const& gravity, std::optional<typename PhysicsSceneTraits::Config> const& config = std::nullopt)
			: Base(gravity) {
			if (s_ref_count.fetch_add(1u, std::memory_order::acq_rel) == 0) {
				InitJoltPhysicsSystem(gravity, *config);
			}
			while (!s_flag.test(std::memory_order::relaxed)) {
				s_flag.wait(false, std::memory_order::relaxed);
			}
		}

		~JoltPhysicsScene() noexcept {
			if (s_ref_count.fetch_sub(1u, std::memory_order::acq_rel) == 1) {
				DestroyPhysicsSystem();
				s_flag.clear(std::memory_order::relaxed);
			}
		}

	};



}