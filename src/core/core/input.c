/**
 * @file input.c
 * @brief handles input states
 *
 */
#include "Spark/core/input.h"
#include "Spark/core/smemory.h"
#include "Spark/core/logging.h"
#include "Spark/core/event.h"

typedef struct {
    b8 keys[0xff];
} keyboard_state_t;

typedef struct {
    s16 x;
    s16 y;
    b8 buttons[MOUSE_BUTTON_MAX];
} mouse_state_t;

typedef struct {
    keyboard_state_t keyboard_current;
    keyboard_state_t keyboard_previous;
    mouse_state_t mouse_current;
    mouse_state_t mouse_previous;
} input_state_t;

static b8 is_initialized = false;
static input_state_t state = {};

// ==========================
// System Functions
// ==========================
void 
input_initialize() {
    szero_memory(&state, sizeof(input_state_t));
    is_initialized = true;
    STRACE("Initialized input system");
}

void 
input_shutdown() {
    is_initialized = false;
    STRACE("Shutdown input system");
}

void 
input_update(f64 delta_time) {
    SASSERT(is_initialized, "Input system cannot update; not initialized");

    // Copy current state to previous state
    state.keyboard_previous = state.keyboard_current;
    state.mouse_previous = state.mouse_current;
}

// ==========================
// input processing
// ==========================
void 
input_process_key(keycode_t key, b8 pressed) {
    // Only handle when state changed
    if (state.keyboard_current.keys[key] == pressed)
        return;

    state.keyboard_current.keys[key] = pressed;

    event_context_t context = {
        .data.u16[0] = key,
    };
    event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, NULL, context);
}

void 
input_process_button(mouse_button_t button, b8 pressed) {
    // Only handle when state changed
    if (state.mouse_current.buttons[button] == pressed)
        return;

    state.mouse_current.buttons[button] = pressed;

    event_context_t context = {
        .data.u16[0] = button,
    };
    event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, NULL, context);
}
void 
input_process_mouse_move(s16 x, s16 y) {
    // Only handle when state changed
    if (state.mouse_current.x == x && state.mouse_current.y == y)
        return;

    state.mouse_current.x = x;
    state.mouse_current.y = y;
    s16 x_delta = state.mouse_current.x - state.mouse_previous.x;
    s16 y_delta = state.mouse_current.y - state.mouse_previous.y;

    event_context_t context = {
        .data.s16[0] = x,
        .data.s16[1] = y,
        .data.s16[2] = x_delta,
        .data.s16[3] = y_delta,
    };

    event_fire(EVENT_CODE_MOUSE_MOVED, NULL, context);
}
void 
input_process_mouse_wheel(s32 delta) {
    // No internal state 
    event_context_t context = {
        .data.u16[0] = delta,
    };
    event_fire(EVENT_CODE_MOUSE_WHEEL, NULL, context);
}


// ==========================
// Keyboard 
// ==========================
b8 
input_is_key_down(keycode_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.keyboard_current.keys[key] == true && state.keyboard_previous.keys[key] == false;
}
b8 
input_is_key_up(keycode_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.keyboard_current.keys[key] == false && state.keyboard_previous.keys[key] == true;
}
b8 
input_was_key_down(keycode_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.keyboard_previous.keys[key] == true;
}
b8 
input_was_key_up(keycode_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.keyboard_previous.keys[key] == false;
}

// ==========================
// Mouse
// ==========================
b8 
input_is_mouse_down(mouse_button_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.mouse_current.buttons[key] == true;
}
b8 
input_is_mouse_up(mouse_button_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.mouse_current.buttons[key] == false;
}
b8 
input_was_mouse_down(mouse_button_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.mouse_previous.buttons[key] == true;
}
b8 
input_was_mouse_up(mouse_button_t key) {
    SASSERT(is_initialized, "Input system not initialized");
    return state.mouse_previous.buttons[key] == false;
}
void 
input_get_mouse_position(s32* x, s32* y) {
    SASSERT(is_initialized, "Input system not initialized");
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}
void 
input_get_previous_mouse_position(s32* x, s32* y) {
    SASSERT(is_initialized, "Input system not initialized");
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}
