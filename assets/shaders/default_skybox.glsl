#version 450
#renderpass skybox

// Vertex
struct vertex_input {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct fragment_input {
    vec3 normal;
    vec2 uv;
};

struct fragment_output {
    vec4 color;
};

layout(set = 0, binding = 0) readonly uniform world_data { mat4 view_matrix; } world;
layout(set = 1, binding = 0) uniform sampler2D main_texture;

void vert() {
    out.normal = normalize(in.position);
    out.uv = in.uv;

    gl_Position = world.view_matrix * vec4(in.position, 1.0f);
}

void frag() {
    out.color = texture(main_texture, in.uv);
}
