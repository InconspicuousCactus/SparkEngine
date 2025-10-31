#include "Spark/ui/ui_systems.h"
#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/ui/text.h"

void render_text(ecs_iterator_t* iterator);

void ui_systems_init(ecs_world_t* world) {
    ecs_system_create_info_t text_rendering = {
        .query = {
            .component_count = 1,
            .components = (ecs_component_id[]) {
                ECS_COMPONENT_ID(text_t),
            },
        },
        .callback = render_text,
        .name = "Render Text",
        .phase = ECS_PHASE_RENDER,
    };

    ecs_system_create(world, &text_rendering);
}

void render_text(ecs_iterator_t* iterator) {
    text_t* texts = ECS_ITERATOR_GET_COMPONENTS(iterator, 0);

    for (u32 i = 0; i < iterator->entity_count; i++) {
        SDEBUG("Text %d: '%s'", i, texts[i].value);
    }
}
