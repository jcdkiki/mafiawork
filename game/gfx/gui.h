#ifndef RENG_GUI_H
#define RENG_GUI_H

#include "../gfx.h"

enum {
	GUI_DYNAMIC_MEMORY_FLAG = (1 << 31)
};

typedef struct gui_context {
	vec2f pos;
} gui_context_t;

typedef void (*gui_vtable_draw_fn)(void* element, gui_context_t* ctx);
typedef void (*gui_vtable_destroy_fn)(void* element);
typedef void (*gui_vtable_calcdim_fn)(void* element);

typedef struct gui_vtable {
	gui_vtable_draw_fn draw;
	gui_vtable_calcdim_fn calcdim;
	gui_vtable_destroy_fn destroy;
} gui_vtable_t;

typedef struct gui_element {
	gui_vtable_t* type;
	
	vec2f position;
	vec2f size;
	vec2f child_position;
	
	uint32_t flags;
} gui_element_t;

typedef struct gui_window {
	gui_element_t base;
	gui_element_t* content;
} gui_window_t;

enum {
	GUI_LABEL_DEALLOCATE_ON_DESTROY_FLAG = (1 << 0)
};

typedef struct gui_label {
	gui_element_t base;
	const char* buf;
	font_t* font;
} gui_label_t;

enum {
	GUI_BUTTON_STATIC_BUFFER_FLAG = (1 << 0),
	GUI_BUTTON_CLICKABLE_FLAG = (1 << 1)
};

typedef struct gui_button {
	gui_element_t base;
	const char* buf;
};

enum {
	GUI_STATIC_CONTAINER_HORIZONTAL_FLAG = (1 << 0),
	GUI_STATIC_CONTAINER_DYNAMIC_MEM = (1 << 1)
};

typedef struct gui_static_container {
	gui_element_t base;
	gui_element_t** children;
	uint32_t n_children;
} gui_static_container_t;

extern gui_vtable_t gui_window_vtable;
extern gui_vtable_t gui_label_vtable;
extern gui_vtable_t gui_button_vtable;
extern gui_vtable_t gui_static_container_vtable;

void gui_init_window(gui_window_t* win, gui_element_t* content, uint32_t flags);
void gui_init_label(gui_label_t* label, font_t* font, const char* buf, uint32_t flags);
void gui_init_static_container(gui_static_container_t* sc, uint32_t n_children, gui_element_t** children, uint32_t flags);

void gui_calculate_dimensions(gui_element_t* element);
void gui_draw_element(gui_element_t* element, gui_context_t* ctx);
void gui_destroy_elements(gui_element_t* element);

#endif