#include "Spark/core/application.h"
#include "Spark/core/event.h"
#include "Spark/core/input.h"
#include "Spark/core/sstring.h"
#include "Spark/ecs/ecs_world.h"
#include "Spark/game_types.h"
#include "Spark/memory/linear_allocator.h"
#include "Spark/physics/physics_backend.h"
#include "Spark/platform/platform.h"
#include "Spark/core/logging.h"
#include "Spark/core/smemory.h"
#include "Spark/core/clock.h"
#include "Spark/renderer/renderer_frontend.h"
#include "Spark/resources/resource_loader.h"
#include "Spark/resources/resource_types.h"
#include "Spark/systems/core_systems.h"
#include "Spark/types/ecs_declarations.h"
#include "Spark/ui/ui_systems.h"

typedef enum : u8 {
    APPLICATION_STATE_OFF       = 0,
    APPLICATION_STATE_RUNNING   = 1,
    APPLICATION_STATE_SUSPENDED = 2,
} application_status_t;

typedef struct {
    platform_state_t platform;
    game_t* game_inst;
    spark_clock_t clock;

    linear_allocator_t systems_allocator;
    u64 memory_system_memory_requirement;
    void* memory_system_state;

    u64 job_system_memory_requirement;
    void* job_system_state;

    u64 logging_system_memory_requirement;
    void* logging_system_state;

    u64 texture_system_memory_requirement;
    void* texture_system_state;

    u64 material_system_memory_requirement;
    void* material_system_state;

    u64 geometry_system_memory_requirement;
    void* geometry_system_state;

    u64 resource_system_memory_requirement;
    void* resource_system_state;

    u64 transform_systems_memory_requirement;
    void* transform_systems_state;

    u64 camera_system_memory_requirement;
    void* camera_system_state;

    f64 last_time;
    s16 width;
    s16 height;
    application_status_t status;
} application_state_t;

static b8 initialized = false;
static application_state_t* app_state;

// Event handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context_t context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context_t context);
b8 application_on_resized(u16 code, void* sneder, void* listener_inst, event_context_t context);

b8 
application_create(game_t* game_inst) {
    SASSERT(!initialized, "Application already initialized: %i", initialized);
    STRACE("Initializing application");

    SASSERT(!game_inst->application_state, "application_create called more than once.");

    // Memory
    initialize_memory();

    // Application
    game_inst->application_state = sallocate(sizeof(application_state_t), MEMORY_TAG_GAME);
    app_state = game_inst->application_state;
    app_state->status = APPLICATION_STATE_OFF;

    app_state->game_inst = game_inst;
    app_state->width = game_inst->config.start_width;
    app_state->height = game_inst->config.start_height;

    // Systems
    u64 systems_allocator_total_size = 64 * KB;
    linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);

    // Logging
    initialize_logging(&app_state->logging_system_memory_requirement, 0);
    app_state->logging_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
    if (!initialize_logging(&app_state->logging_system_memory_requirement, app_state->logging_system_state)) {
        SERROR("Failed to initialize logging system; shutting down.");
        return false;
    }
    
    // Resources
    resource_loader_initialize(&app_state->systems_allocator);

    // ECS
    ecs_world_initialize(&app_state->systems_allocator);

    // Systems
    ecs_world_t* world = ecs_world_get();
    ecs_register_types(world);
    transform_system_initialize(world);
    camera_systems_initialize(world);
    render_system_initialize(world);
    ui_systems_init(world);

    // Input
    input_initialize();

    // Events
    if (!event_initialize()) {
        SCRITICAL("Failed to initialize event system");
        return false;
    }

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_RESIZED, 0, application_on_resized);

    // Update state
    app_state->status = APPLICATION_STATE_RUNNING;

    const application_config_t config = game_inst->config;
    if (!platform_init(&app_state->platform, config.name, config.start_pos_x, config.start_pos_y, config.start_width, config.start_height)) {
        return false;
    }

    // Renderer init
    spark_clock_t clock = {};
    clock_start(&clock);
    if (!renderer_initialize(game_inst->config.name, &app_state->platform, &app_state->systems_allocator)) {
        SCRITICAL("Failed to initialize renderer. Application closing...");
        return false;
    }
    clock_update(&clock);
    SDEBUG("Vulkan took %fms to startup", clock.elapsed_time * 1000);

    // Physics
    physics_backend_initialize(&app_state->systems_allocator);

    // User Game
    if (!app_state->game_inst->initialize(game_inst)) {
        SCRITICAL("Game failed to initialize");
    }

    app_state->game_inst->on_resize(game_inst, config.start_width, config.start_height);


    initialized = true;
    return true;
}

b8 
application_run() {
    app_state->status |= APPLICATION_STATE_RUNNING;
    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time = app_state->clock.elapsed_time;

    /*f64 running_time = 0;*/
    u64 frame_count = 0;
    f64 target_fps = 1.0 / 60.0;


    const u32 frame_time_count = 120;
    f32 frame_times[frame_time_count];
    szero_memory(frame_times, sizeof(frame_times));

    SINFO("Initialized memory usage:");
    SINFO("%s", get_memory_usage_string());

    while (app_state->status & APPLICATION_STATE_RUNNING) {
        if (!platform_pump_messages(&app_state->platform)) {
            app_state->status &= ~APPLICATION_STATE_RUNNING;
        }

        if (app_state->status & APPLICATION_STATE_SUSPENDED) {
            SDEBUG("Application suspended");
            app_state->status &= ~APPLICATION_STATE_RUNNING;
            continue;
        }

        // Update clock
        clock_update(&app_state->clock);
        f64 current_time = app_state->clock.elapsed_time;
        f64 delta = current_time - app_state->last_time;
        f64 frame_start_time = platform_get_absolute_time();
        const f32 delta_time = delta;

        // Update
        ecs_world_progress();
        b8 update_success = app_state->game_inst->update(app_state->game_inst, delta_time );
        if (!update_success) {
            SASSERT(update_success, "Game failed to update");
            app_state->status &= ~APPLICATION_STATE_RUNNING;
            break;
        }

        // b8 render_success = app_state->game_inst->render(app_state->game_inst, delta_time);
        // renderer_draw_frame(NULL);
        // if (!render_success) {
        //     SASSERT(update_success, "Game failed to render");
        //     app_state->status &= ~APPLICATION_STATE_RUNNING;
        //     break;
        // }

        // Calculate frame time
        f64 frame_end_time = platform_get_absolute_time();
        f64 frame_elapsed_time = frame_end_time - frame_start_time;
        f64 remaining_seconds = target_fps - frame_elapsed_time;

        // FPS Calculation
        app_state->last_time = current_time;
        if (frame_count % frame_time_count == 0) {
            f64 total_frame_time = 0;
            for (u32 i = 0; i < frame_time_count; i++) {
                total_frame_time += frame_times[i];
            }
        }

        for (int i = frame_time_count - 1; i > 0; i--) {
            frame_times[i] = frame_times[i - 1];
        }
        frame_times[0] = frame_elapsed_time;

        frame_count++;

        // Wait for remaining time
        if (remaining_seconds > 0) {
            u64 remaining_ms = remaining_seconds * 1000;

            const b8 limit_fps = false;
            if (remaining_ms > 0 && limit_fps) {
                platform_sleep(remaining_ms);
            }

            /*frame_count++;*/
        }

        input_update(delta_time);
        platform_set_cursor_position(&app_state->platform, 256, 256);
    }

    STRACE("App shutting down");
    app_state->status &= ~APPLICATION_STATE_RUNNING;

    SINFO("Shutting down client game");
    if (app_state->game_inst->shutdown) {
        app_state->game_inst->shutdown(app_state->game_inst);
    }

    resource_loader_shutdown();
    render_system_shutdown();
    renderer_shutdown();
    event_shutdown();
    input_shutdown();
    ecs_world_shutdown();
    physics_backend_shutdown();

    linear_allocator_destroy(&app_state->systems_allocator);

    platform_shutdown(&app_state->platform);
    shutdown_logging(app_state->logging_system_state);
    sfree(app_state->game_inst->application_state, sizeof(application_state_t), MEMORY_TAG_GAME);
    return true;
}

void 
application_get_framebuffer_size(u32* width, u32* height) {
    *width = app_state->width;
    *height = app_state->height;
}


b8 
application_on_event(u16 code, void* sender, void* listener_inst, event_context_t context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
                                              SINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
                                              app_state->status &= ~APPLICATION_STATE_RUNNING;
                                              return true;
                                          }
    }

    return false;
}

b8 
application_on_key(u16 code, void* sender, void* listener_inst, event_context_t context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            event_context_t data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return true;
        }
    }
    return false;
}

b8 
application_on_resized(u16 code, void* sender, void* listener_inst, event_context_t context) {
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Check if different. If so, trigger a resize event.
        if (width != app_state->width || height != app_state->height) {
            app_state->width = width;
            app_state->height = height;

            // Handle minimization
            if (width == 0 || height == 0) {
                SINFO("Window minimized, suspending application.");
                app_state->status |= APPLICATION_STATE_SUSPENDED;
                return true;
            } else {
                if (app_state->status & APPLICATION_STATE_SUSPENDED) {
                    SINFO("Window restored, resuming application.");
                    app_state->status &= ~APPLICATION_STATE_SUSPENDED;
                }
                app_state->game_inst->on_resize(app_state->game_inst, width, height);
                renderer_on_resize(width, height);
            }
        }
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}
