#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"

void ecs_system_create(struct ecs_world* world, const ecs_system_create_info_t* create_info) {
#if SPARK_DEBUG
    for (u32 i = 0; i < create_info->query.component_count; i++) {
        SASSERT(create_info->query.components[i] != 0, "Cannot create system, required component was not initialized (index: %d).", i);
    }
    for (u32 i = 0; i < create_info->query.without_component_count; i++) {
        SASSERT(create_info->query.without_components[i] != 0, "Cannot create system, required without_component was not initialized (index: %d).", i);
    }
#endif

    ecs_system_t system = {
        .query = ecs_query_create(world, &create_info->query),
        .callback = create_info->callback 
    };

#if SPARK_DEBUG
    system.name = create_info->name;
#endif

    darray_ecs_system_push(&world->systems[create_info->phase], system);
}

void ecs_system_destroy(ecs_system_t* system) {

}
