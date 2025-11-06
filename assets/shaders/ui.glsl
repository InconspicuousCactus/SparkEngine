#version 450
#renderpass ui 

// Vertex
struct vertex_input {
    vec2 position;
    vec2 uv;
};

struct fragment_input {
    vec2 tex_coord;
    vec4 color;
};

struct fragment_output {
    vec4 color;
};

layout(set = 0, binding = 0) readonly uniform world_data { mat4 matrix; } world;
layout(set = 0, binding = 1) readonly buffer instance_data { mat4 matrix[]; } instance;
layout(set = 1, binding = 0) uniform sampler2D main_texture;

void vert() {
    vec2 pos = in.position;
    out.tex_coord = in.uv;
    out.color = vec4(1);
    gl_Position = world.matrix * instance.matrix[gl_InstanceIndex] * vec4(pos, 1.0f, 1.0f);
}

void frag() {
    out.color = texture(main_texture, in.tex_coord);
    if (out.color.w <= .5f) {
        discard;
    }
}
