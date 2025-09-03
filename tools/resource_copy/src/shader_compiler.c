#include "shader_compiler.h"
#include "Spark/core/logging.h"
#include "Spark/renderer/renderpasses.h"
#include "Spark/renderer/shader.h"
#include "Spark/types/s3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_string_literal(FILE* file, const char* literal) {
    fwrite(literal, strlen(literal), 1, file);
}

b8 string_equal_literal(const char* base, const char* literal) {
    return strncmp(base, literal, strlen(literal)) == 0;
}

b8 string_contains(const char* base, const char c) {
    for (u32 i = 0; i < strlen(base); i++) {
        if (base[i] == c) {
            return true;
        }
    }
    return false;
}
b8 string_equal(const char* base, const char* literal) {
    return strcmp(base, literal) == 0;
}

b8 is_variable_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

b8 is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

char* read_token(const char* input, u32* out_token_length) {
    const char* text_start = input;

    u32 token_length = 0;
    if (*input == '#') { // Macros
        while (*input != '\n') {
            token_length++;
            input++;
        }
    } else if (is_variable_char(*input)) { // Variables
        while (is_variable_char(*input) && !is_whitespace(*input)) {
            input++;
            token_length++;
        }
    } else if (!is_variable_char(*input)) { // Other
        while (!is_variable_char(*input) && !is_whitespace(*input)) {
            input++;
            token_length++;
        }
    }

    char* token = malloc(token_length);
    token[token_length] = 0;

    strncpy(token, text_start, token_length);
    *out_token_length = token_length;
    return token;
}

char** tokenize(const char* input, u32* out_token_count) {
    u32 token_capacity = 1024;
    *out_token_count = 0;
    char** tokens = malloc(sizeof(char*) * token_capacity);

    u32 input_length = strlen(input);
    const char* max_input = input + input_length;

    u32 token_count = 0;
    while (input < max_input) {
        // Skip whitespace
        while (is_whitespace(*input)) {
            input++;
        }

        // Delete comments
        while (string_equal_literal(input, "//")) {
            while (*input != '\n') {
                input++;
            }
            input++;
        }
        while (string_equal_literal(input, "/*")) {
            while (!string_equal_literal(input, "*/")) {
                input++;
            }
            input += 2;
        }

        // Skip whitespace
        while (is_whitespace(*input)) {
            input++;
        }

        u32 token_length = 0;
        tokens[*out_token_count] = read_token(input, &token_length);
        *out_token_count += 1;
        input += token_length;
        token_count++;
    }

    return tokens;
}


u32 parse_struct(FILE* output, const char** tokens, u32 token_offset, u32 token_count, b8 input) {
    u32 read_token_count = 0;
    while (!string_contains(tokens[token_offset + read_token_count], '{')) {
        read_token_count++;
    }
    read_token_count++;

    u32 location_index = 0;

    while (!string_contains(tokens[token_offset + read_token_count], '}')) {
        char write_buffer[1024] = {};
        snprintf(write_buffer, sizeof(write_buffer), "layout(location = %d) %s %s %s%s;\n", 
                location_index++, 
                input ? "in" : "out", 
                tokens[token_offset + read_token_count], 
                input ? "input_" : "output_",
                tokens[token_offset + read_token_count + 1]);

        if (output) {
            write_string_literal(output, write_buffer);
        }

        while (!string_contains(tokens[token_offset + read_token_count], ';')) {
            read_token_count++;
        }
        read_token_count++;
    }

    return read_token_count;
}

void skip_function(const char** tokens, u32* token_offset, u32 token_count) {
    u32 indent = 1;
    while (!string_contains(tokens[*token_offset], '{')) {
        *token_offset += 1;
    }
    *token_offset += 1;

    for (u32 i = *token_offset; i < token_count; i++, *token_offset += 1) {
        if (indent <= 0) {
            break;
        }
        if (string_contains(tokens[i], '}')) {
            indent--;
        }
        if (string_contains(tokens[i], '{')) {
            indent++;
        }
    }
    *token_offset -= 1;
}

b8 compile_stage(const char* file, const char** tokens, const u32 token_count, shader_stage_flags_t stage, char** out_file_name) {
    const char* shader_stage_names[SHADER_STAGE_ENUM_MAX] = {
        "",
        "vert",
        "frag",
    };

    // Create new file
#define temp_file_name_size 512
    char temp_file_name[temp_file_name_size];
    strncat(temp_file_name, ".", temp_file_name_size - 1);
    strncpy(temp_file_name, file, temp_file_name_size - 1);
    strncat(temp_file_name, ".", temp_file_name_size - 1);
    strncat(temp_file_name, shader_stage_names[stage], temp_file_name_size - 1);
    strncat(temp_file_name, ".tmp", temp_file_name_size - 1);

    FILE* out_file = fopen(temp_file_name, "w");

    // Parse old file and inject new data
    const char* vertex_input = NULL;
    const char* vertex_output = NULL;

    for (u32 i = 0; i < token_count; i++) {
        const char* token = tokens[i];
        if (string_equal_literal(token, "#renderpass")) {
            // Do nothing with renderpass data (yet)
        } else if (string_equal(token, "struct")) {
            if (string_equal(tokens[i + 1], "vertex_input")) {
                i += parse_struct(
                        stage == SHADER_STAGE_VERTEX ? out_file : NULL, 
                        tokens, 
                        i, 
                        token_count, 
                        true);

            } else if (string_equal(tokens[i + 1], "fragment_input")) {
                switch (stage) {
                    case SHADER_STAGE_VERTEX:
                        i += parse_struct(out_file, tokens, i, token_count, false);
                        break;
                    case SHADER_STAGE_FRAGMENT:
                        i += parse_struct(out_file, tokens, i, token_count, true);
                        break;
                    default:
                        i += parse_struct(NULL, tokens, i, token_count, true);
                        break;
                }
            } else if (string_equal(tokens[i + 1], "fragment_output")) {
                switch (stage) {
                    case SHADER_STAGE_FRAGMENT:
                        i += parse_struct(out_file, tokens, i, token_count, false);
                        break;
                    default:
                        i += parse_struct(NULL, tokens, i, token_count, true);
                        break;
                }
            }
        } else if (string_equal(token, "void") && string_equal(tokens[i + 1], "vert") ) {
            if (stage != SHADER_STAGE_VERTEX) {
                skip_function(tokens, &i, token_count);
            } else {
                i++; // Skips function name
                write_string_literal(out_file, "void main ");
            }
        } else if (string_equal(token, "void") && string_equal(tokens[i + 1], "frag")) {
            if (stage != SHADER_STAGE_FRAGMENT) {
                skip_function(tokens, &i, token_count);
            } else {
                i++; // Skips function name
                write_string_literal(out_file, "void main ");
            }
        } else if (string_equal(token, "in") && string_equal(tokens[i + 1], ".")) {
            write_string_literal(out_file, " input_");
            i++;
        } else if (string_equal(token, "out") && string_equal(tokens[i + 1], ".")) {
            write_string_literal(out_file, " output_");
            i++;
        } else {
            fwrite(token, strlen(token), 1, out_file);
            b8 add_space = *token != '.';
            const char no_space_chars[] = {
                '.', 
                '(',
                '0',
                '1',
                '2',
                '3',
                '4',
                '5',
                '6',
                '7',
                '8',
                '9',
            };

            if (i < token_count - 1) {
                for (u32 j = 0; j < sizeof(no_space_chars) / sizeof(char); j++) {
                    add_space &= *tokens[i + 1] != no_space_chars[j];
                }
            }
            if (add_space) {
                fwrite(" ", 1, 1, out_file);
            } 
        }


        if (*token == '{' || *token == '}' || *token == ';' || *token == '#') {
            fwrite("\n", 1, 1, out_file);
        }
    }
    fclose(out_file);


    const char* stage_name = shader_stage_names[stage];

    const u32 out_file_name_size = 512;
    *out_file_name = malloc(out_file_name_size);
    snprintf(*out_file_name, out_file_name_size, "%s.%s.spv", file, stage_name);

    char compile_command[1024];
    snprintf(compile_command, sizeof(compile_command), "glslc -fshader-stage=%s '%s' -o '%s'", stage_name, temp_file_name, *out_file_name);
    b8 result = system(compile_command) == 0;
    return result;
}

b8 create_shader_config(const char* file_name, const char** tokens, u32 token_count, shader_config_t* out_config) {
    strncpy(out_config->name, file_name, sizeof(out_config->name) / sizeof(char) - 1);

    // Look through all tokens
    for (u32 i = 0; i < token_count - 1; i++) {
        // Found the vertex input struct
        if (string_equal(tokens[i], "struct") && string_equal(tokens[i + 1], "vertex_input")) {
            while (!string_contains(tokens[i], '}')) {
                while (!string_contains(tokens[i], '{')) {
                    i++;
                }
                i++;

                if (string_equal(tokens[i], "float")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_FLOAT;
                } else if (string_equal(tokens[i], "vec2")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_VEC2;
                } else if (string_equal(tokens[i], "vec3")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_VEC3;
                } else if (string_equal(tokens[i], "vec4")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_VEC4;
                } else if (string_equal(tokens[i], "int")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_INT;
                } else if (string_equal(tokens[i], "vec2i")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_IVEC2;
                } else if (string_equal(tokens[i], "vec3i")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_IVEC3;
                } else if (string_equal(tokens[i], "vec4i")) {
                    out_config->attributes[out_config->attribute_count++] = VERTEX_ATTRIBUTE_IVEC4;
                } else {
                    SWARN("Shader has unknown attribute '%s'\n", attribute);
                    return false;
                }

                // Skip until next token
                while (!string_contains(tokens[i], ';')) {
                    i++;
                }
                i++;
            }
        }

        // Search for resources
        if (string_equal(tokens[i], "layout")) {
            u32 set = 0;
            u32 binding = 0;
            shader_resource_type_t type = SHADER_RESOURCE_UNDEFINED;
            while (!string_contains(tokens[i], ';')) {
                const char* token = tokens[i];
                if (string_equal(token, "set")) {
                    set = atoi(tokens[i + 2]);
                } else if (string_equal(token, "binding")) {
                    binding = atoi(tokens[i + 2]);
                } else if (string_equal(token, "buffer")) {
                    type = SHADER_RESOURCE_SOTRAGE_BUFFER;
                } else if (string_equal(token, "sampler2D")) {
                    type = SHADER_RESOURCE_SAMPLER;
                } else if (string_equal(token, "uniform")) {
                    type = SHADER_RESOURCE_UNIFORM_BUFFER;
                } 
                i++;
            }

            out_config->layout[out_config->resource_count++] = (shader_resource_layout_t) {
                .type = type,
                    .binding = binding,
                    .set = set,
            };
        }

        // Search for renderpass
        if (string_equal_literal(tokens[i], "#renderpass")) {
            const char* renderpass = tokens[i] + sizeof("#renderpass");
            if (string_equal(renderpass, "world")) {
                out_config->type = BUILTIN_RENDERPASS_WORLD;
            } else if (string_equal(renderpass, "ui")) {
                out_config->type = BUILTIN_RENDERPASS_UI;
            } else if (string_equal(renderpass, "skybox")) {
                out_config->type = BUILTIN_RENDERPASS_SKYBOX;
            } else {
                printf("Undefined renderpass '%s'\n", renderpass);
                return false;
            }
            i++;
        }
    }

    // builtin_renderpass_t type;

    return true;;
}

b8 append_file_contents(FILE* file, const char* file_path, u32* out_length) {
    // Open file and get length
    FILE* src = fopen(file_path, "r");
    if (!src) {
        return false;
    }

    fseek(src, 0, SEEK_END);
    *out_length = ftell(src);
    fseek(src, 0, SEEK_SET);

    void* buffer = malloc(*out_length);
    fread(buffer, *out_length, 1, src);
    fwrite(buffer, *out_length, 1, file);

    return true;
}

b8 compile_shader(const char* file_path, const char* file_name, const char* output, const char* stage) {
    // Read source text 
    FILE* file = fopen(file_path, "r");

    fseek(file, 0, SEEK_END);
    u32 file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* text = malloc(file_size);
    fread(text, file_size, 1, file);

    // Parse to tokens
    u32 token_count = 0;
    const char** tokens = (const char**)tokenize(text, &token_count);

    // Generate config
    shader_config_t config = {};
    if (!create_shader_config(file_name, tokens, token_count, &config)) {
        printf("Failed to create config for shader '%s'.\n", file_path);
        return false;
    }

    // Compile shader stages
    char* vertex_filename = NULL;
    if (!compile_stage(file_name, tokens, token_count, SHADER_STAGE_VERTEX, &vertex_filename)) {
        remove(vertex_filename);
        printf("Failed to compile vertex stage in shader %s.\n", file_path);
        return false;
    }

    char* fragment_filename = NULL;
    if (!compile_stage(file_name, tokens, token_count, SHADER_STAGE_FRAGMENT, &fragment_filename)) {
        remove(vertex_filename);
        remove(fragment_filename);
        printf("Failed to compile fragment stage in shader %s.\n", file_path);
        return false;
    }

    // Create shader resource
    binary_shader_resource_t res = {
        .config = config,
    };

    FILE* out_file = fopen("test.bshd", "w");
    fwrite(&res, sizeof(binary_shader_resource_t), 1, out_file);

    // Append spv file contents
    append_file_contents(out_file, vertex_filename, &res.config.vertex_spv_size);
    append_file_contents(out_file, fragment_filename, &res.config.fragment_spv_size);

    // Rewrite updated header
    res.config.vertex_spv = (void*)(sizeof(binary_shader_resource_t));
    res.config.fragment_spv = res.config.vertex_spv + res.config.vertex_spv_size;

    fseek(out_file, 0, SEEK_SET);
    fwrite(&res, sizeof(binary_shader_resource_t), 1, out_file);

    remove(vertex_filename);
    remove(fragment_filename);

    printf("Successfully compile shader '%s'\n", file_name);

    return true;
}
