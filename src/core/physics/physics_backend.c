#include "Spark/physics/physics_backend.h"
#include "Spark/core/logging.h"
#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/ecs/entity.h"
#include "Spark/math/quat.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/physics/physics.h"
#include "Spark/types/transforms.h"
#include "joltc.h"

// Private data types
typedef struct physics_state {
    JPH_JobSystem*                     job_system;
    JPH_ObjectLayerPairFilter*         object_layer_pair_filter_table;
    JPH_BroadPhaseLayerInterface*      broadphase_layer_interface_table;
    JPH_ObjectVsBroadPhaseLayerFilter* object_to_broadphase_filert;
    JPH_PhysicsSystem*                 system;
    JPH_BodyInterface*                 body_interface;
} physics_state_t;

static physics_state_t* state;

// Private functions
void jph_trace(const char* message);
void physics_step_system(ecs_iterator_t* iterator);

JPH_MotionType convert_motion_type(physics_motion_t);

// Function impl
void physics_backend_initialize(linear_allocator_t* allocator) {
    STRACE("Initializing jolt physics backend");
    // Allocate data
    state = linear_allocator_allocate(allocator, sizeof(physics_state_t));

    if (!JPH_Init()) {
        SCRITICAL("Unable to initialize jolt physics backend");
        return;
    }

    JPH_SetTraceHandler(jph_trace);
    //JPH_SetAssertFailureHandler(JPH_AssertFailureFunc handler);

    state->job_system = JPH_JobSystemThreadPool_Create(NULL);

    // We use only 2 layers: one for non-moving objects and one for moving objects
    state->object_layer_pair_filter_table = JPH_ObjectLayerPairFilterTable_Create(2);
    JPH_ObjectLayerPairFilterTable_EnableCollision(state->object_layer_pair_filter_table, PHYSICS_LAYER_MOVING, PHYSICS_LAYER_MOVING);
    JPH_ObjectLayerPairFilterTable_EnableCollision(state->object_layer_pair_filter_table, PHYSICS_LAYER_MOVING, PHYSICS_LAYER_NON_MOVING);

    // We use a 1-to-1 mapping between object layers and broadphase layers
    state->broadphase_layer_interface_table = JPH_BroadPhaseLayerInterfaceTable_Create(2, 2);
    JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(state->broadphase_layer_interface_table, PHYSICS_LAYER_NON_MOVING, PHYSICS_BROADPHASE_LAYER_NON_MOVING);
    JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(state->broadphase_layer_interface_table, PHYSICS_LAYER_MOVING, PHYSICS_BROADPHASE_LAYER_MOVING);

    JPH_ObjectVsBroadPhaseLayerFilter* object_to_broadphase_filert = JPH_ObjectVsBroadPhaseLayerFilterTable_Create(state->broadphase_layer_interface_table, 2, state->object_layer_pair_filter_table, 2);

    JPH_PhysicsSystemSettings settings = {
        .maxBodies = 65536,
        .numBodyMutexes = 0,
        .maxBodyPairs = 65536,
        .maxContactConstraints = 65536,
        .broadPhaseLayerInterface = state->broadphase_layer_interface_table,
        .objectLayerPairFilter = state->object_layer_pair_filter_table,
        .objectVsBroadPhaseLayerFilter = object_to_broadphase_filert,
    };
    state->system = JPH_PhysicsSystem_Create(&settings);
    state->body_interface = JPH_PhysicsSystem_GetBodyInterface(state->system);
    STRACE("Done initializing jolt physics backend");

    // Setup physics system
    ecs_world_t* world = ecs_world_get();

#define physics_step_component_count 5
    ecs_component_id physics_step_components[physics_step_component_count] = {
        ECS_COMPONENT_ID(physics_body_t), 
        ECS_COMPONENT_ID(velocity_t), 
        ECS_COMPONENT_ID(translation_t), 
        ECS_COMPONENT_ID(rotation_t),
        ECS_COMPONENT_ID(dirty_transform_t)  
    };
    ecs_query_create_info_t physics_step_create_info = {
        .component_count = physics_step_component_count,
        .components = physics_step_components,
    };
    ecs_system_create(world, 
            ECS_PHASE_PHYSICS, 
            &physics_step_create_info,
            physics_step_system, 
            "Physics");
}

void physics_backend_shutdown() {
    // Remove the destroy sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
    // JPH_BodyInterface_RemoveAndDestroyBody(state->bodyInterface, sphereId);
    JPH_JobSystem_Destroy(state->job_system);

    JPH_PhysicsSystem_Destroy(state->system);
    JPH_Shutdown();
}

void entity_add_cube_collider(ecs_world_t* world, entity_t entity, vec3 extents, physics_motion_t motion, physics_layer_t layer) {
    JPH_Vec3 _extents = { extents.x, extents.y, extents.z };
    JPH_BoxShape* box_shape = JPH_BoxShape_Create(&_extents, JPH_DEFAULT_CONVEX_RADIUS);

    JPH_Vec3 position = { 0, -1, 0};
    JPH_BodyCreationSettings* settings = JPH_BodyCreationSettings_Create3(
            (const JPH_Shape*)box_shape,
            &position,
            NULL,
            convert_motion_type(motion),
            layer
            );

    physics_body_t body = {
        .id = JPH_BodyInterface_CreateAndAddBody(state->body_interface, settings, JPH_Activation_Activate),
    };
    SDEBUG("Creating cube body %d", body.id);

    JPH_BodyCreationSettings_Destroy(settings);

    ENTITY_SET_COMPONENT(world, entity, physics_body_t, body);
    ENTITY_ADD_COMPONENT(world, entity, velocity_t);
    ENTITY_ADD_COMPONENT(world, entity, translation_t);
    if (!ENTITY_HAS_COMPONENT(world, entity, rotation_t)) {
        ENTITY_SET_COMPONENT(world, entity, rotation_t, { quat_identity() });
    }
}

// Private functionm implementations 
void jph_trace(const char* message) {
    STRACE("Jolt Physics: %s", message);
}

void physics_step_system(ecs_iterator_t* iterator) {
    physics_body_t* bodies = ECS_ITERATOR_GET_COMPONENTS(iterator, 0);
    velocity_t* velocities = ECS_ITERATOR_GET_COMPONENTS(iterator, 1);
    translation_t* translations = ECS_ITERATOR_GET_COMPONENTS(iterator, 2);
    rotation_t* rotations = ECS_ITERATOR_GET_COMPONENTS(iterator, 3);
    dirty_transform_t* dirty = ECS_ITERATOR_GET_COMPONENTS(iterator, 4);

    for (u32 i = 0; i < iterator->entity_count; i++) {
        // Output current position and velocity of the sphere
        JPH_BodyID id = bodies[i].id;

        // Skip inactive bodies
        if (!JPH_BodyInterface_IsActive(state->body_interface, id)) {
            continue;
        }

        // Sync position and rotation
        JPH_RVec3 position = { .x = translations[i].value.x, .y = translations[i].value.y, .z = translations[i].value.z };
        JPH_RVec3 velocity = { velocities[i].value.x, velocities[i].value.y, velocities[i].value.z };
        JPH_Quat rotation = { rotations[i].value.x, rotations[i].value.y, rotations[i].value.z, rotations[i].value.w };
        JPH_BodyInterface_SetPosition(state->body_interface, id, &position, JPH_Activation_Activate);
        JPH_BodyInterface_SetRotation(state->body_interface, id, &rotation, JPH_Activation_Activate);
        JPH_BodyInterface_SetLinearVelocity(state->body_interface, id, &velocity);
    }

    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int cCollisionSteps = 1;
    // Step the world
    JPH_PhysicsSystem_Update(state->system, 1.0f / 60.0f, cCollisionSteps, state->job_system);

    for (u32 i = 0; i < iterator->entity_count; i++) {
        // Output current position and velocity of the sphere
        JPH_BodyID id = bodies[i].id;

        // Skip inactive bodies
        if (!JPH_BodyInterface_IsActive(state->body_interface, id)) {
            continue;
        }

        JPH_RVec3 position = { };
        JPH_Quat rotation  = { };
        JPH_RVec3 velocity = { };
        JPH_BodyInterface_GetPosition      (state->body_interface, id, &position);
        JPH_BodyInterface_GetRotation      (state->body_interface, id, &rotation);
        JPH_BodyInterface_GetLinearVelocity(state->body_interface, id, &velocity);

        translations[i] = (translation_t) { .value = { {position.x, position.y, position.z} } };
        rotations[i]    = (rotation_t)    { .value = { {rotation.x, rotation.y, rotation.z, rotation.w} } };
        velocities[i]   = (velocity_t)    { .value = { {velocity.x, velocity.y, velocity.z, } } };

        dirty[i].dirty = true;
    }
}

JPH_MotionType convert_motion_type(physics_motion_t motion) {
    switch (motion) {
        default:
            SWARN("Failed to convert spark motion type (%d) to jolt motion type.", motion);
            return JPH_MotionType_Static;
        case PHYSICS_MOTION_STATIC:
            return JPH_MotionType_Static;
        case PHYSICS_MOTION_KINEMATIC:
            return JPH_MotionType_Kinematic;
        case PHYSICS_MOTION_DYNAMIC:
            return JPH_MotionType_Dynamic;
    }
}

void physics_body_destroy(void* component) {
    physics_body_t* body = component;
    SDEBUG("Attempting to remove body %p / %d", body, body->id);
    if (JPH_BodyInterface_IsActive(state->body_interface, body->id)) {
        SDEBUG("Removing body %p / %d", body, body->id);
        JPH_Shape* shape =(JPH_Shape*)JPH_BodyInterface_GetShape(state->body_interface, body->id); 
        if (shape) {
            JPH_Shape_Destroy(shape);
        }
        JPH_BodyInterface_RemoveAndDestroyBody(state->body_interface, body->id);
    }
}
