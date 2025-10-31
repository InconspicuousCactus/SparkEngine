#include "Spark/ui/text.h"
#include "Spark/ecs/entity.h"

entity_t text_create(ecs_world_t* world, const char* text, vec2 anchor, f32 point_size) {
    entity_t e = entity_create(world);
    ENTITY_SET_COMPONENT(world, e, text_t, { text });

    return e;
}
