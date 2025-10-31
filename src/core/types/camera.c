
#include "Spark/types/camera.h"
#include "Spark/renderer/renderer_frontend.h"

vec2 orthographic_camera_screen_space_to_world_space(orthographic_camera_t* camera, vec2 pos, vec3 position) {
    vec2 screen_size = renderer_get_screen_size();
    f32 ratio = screen_size.x / screen_size.y;
    f32 x = camera->distance * ratio + position.x;
    f32 y = camera->distance + position.y;
    return (vec2) { x, y };
}
