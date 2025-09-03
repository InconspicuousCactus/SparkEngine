#version 450
#renderpass world

// Vertex
struct vertex_input {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct fragment_input {
    vec3 normal;
    vec2 tex_coord;
};

struct fragment_output {
    vec4 color;
};

layout(set = 0, binding = 0) readonly uniform world_data { mat4 view_matrix; } world;
layout(set = 0, binding = 1) readonly buffer instance_data { mat4 matrix[]; } instance;
layout(set = 1, binding = 0) uniform sampler2D main_texture;

void vert() {
    vec3 pos = in.position;
    out.tex_coord = in.uv;
    out.normal = in.normal;
    gl_Position = world.view_matrix * instance.matrix[gl_InstanceIndex] * vec4(pos, 1.0f);
}

void frag() {
    out.color = texture(main_texture, in.tex_coord / 8);
    // out_color = vec4(1,1,1,1);
    out.color.xyz *= (dot(in.normal, vec3(.707, .707, .707)) + 1) / 2;
}

