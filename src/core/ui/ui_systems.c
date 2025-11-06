#include "Spark/ui/ui_systems.h"
#include "Spark/ecs/ecs.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/math/mat4.h"
#include "Spark/renderer/renderer_frontend.h"
#include "Spark/resources/resource_loader.h"
#include "Spark/resources/resource_types.h"
#include "Spark/types/transforms.h"
#include "Spark/ui/anchor_2d.h"
#include "Spark/ui/text.h"
#include "Spark/ui/ui_defaults.h"

void render_text(ecs_iterator_t* iterator);
void anchor_ui(ecs_iterator_t* iterator);
ui_default_values_t ui_defaults;

void ui_systems_init(ecs_world_t* world) {
    // Load default values
    resource_t text_mat_res = resource_loader_get_resource("assets/resources/materials/ui_text_material", false);
    ui_defaults.text_material = resource_get_material(&text_mat_res);

    // Create systems
    // Text
    // ecs_system_create_info_t text_rendering = {
    //     .query = {
    //         .component_count = 1,
    //         .components = (ecs_component_id[]) {
    //             ECS_COMPONENT_ID(text_t),
    //         },
    //     },
    //     .callback = render_text,
    //     .name = "Render Text",
    //     .phase = ECS_PHASE_RENDER,
    // };
    //
    // ecs_system_create(world, &text_rendering);

    ecs_system_create_info_t ui_anchor = {
        .query = {
            .component_count = 2,
            .components = (ecs_component_id[]) {
                ECS_COMPONENT_ID(anchor_2d_t),
                ECS_COMPONENT_ID(local_to_world_t),
            },
        },
        .callback = anchor_ui,
        .name = "Anchor UI",
        .phase = ECS_PHASE_TRANSFORM,
    };

    ecs_system_create(world, &ui_anchor);

}

void render_text(ecs_iterator_t* iterator) {
    // text_t* texts = ECS_ITERATOR_GET_COMPONENTS(iterator, 0);
    //
    // for (u32 i = 0; i < iterator->entity_count; i++) {
    // }
}

void anchor_ui(ecs_iterator_t* iterator) {
    anchor_2d_t* anchors = ECS_ITERATOR_GET_COMPONENTS(iterator, 0);
    local_to_world_t* locals = ECS_ITERATOR_GET_COMPONENTS(iterator, 1);

    const vec2 screen_size = renderer_get_screen_size();
    for (u32 i = 0; i < iterator->entity_count; i++) {
        vec3 pos = {
            .x = screen_size.x * (anchors[i].pos.x + 0) / 1,
            .y = screen_size.y * (-anchors[i].pos.y + 0) / 1,
        };
        locals[i].value = mat4_translation(pos);
        // locals[i].value = mat4_identity();
    }
}
