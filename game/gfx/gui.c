#include "gui.h"

//extern gui_vtable_t gui_window_vtable[1];
//extern gui_vtable_t gui_label_vtable[1];
//extern gui_vtable_t gui_button_vtable[1];
//extern gui_vtable_t gui_static_container_vtable[1];

#define GUI_MAKE_VTABLE(vtname_, draw_, calcdim_, destroy_) \
	gui_vtable_t vtname_ = {							\
		.draw = (gui_vtable_draw_fn)draw_,				\
		.calcdim = (gui_vtable_calcdim_fn)calcdim_,		\
		.destroy = (gui_vtable_destroy_fn)destroy_,		\
	};

void gui_window_calcdim(gui_window_t* win)
{
	win->base.child_position = VEC2F(10.f, 10.f);

	if (win->content != NULL) {
		gui_calculate_dimensions(win->content, win);
		win->base.size = VEC2F(win->content->size.x + 20.f, win->content->size.y + 20.f);
	}
	else
		win->base.size = VEC2F(20.f, 20.f);
}

void gui_window_draw(gui_window_t* win, gui_context_t* ctx)
{
	glBindTexture(GL_TEXTURE_2D, shader.white_texture);
	glBindVertexArray(shader.quad_vao);
	glUniform4f(shader.color_location, 0.f, 0.f, 0.f, 0.2f);

	mat4 model;
	mat4_translation(&model, VEC3F(ctx->pos.x + win->base.position.x, ctx->pos.y + win->base.position.y, 0.f));
	mat4_scale(&model, VEC3F(win->base.size.x, win->base.size.y, 1.f));

	glUniformMatrix4fv(shader.model_mat_location, 1, GL_TRUE, model.v);
	glDrawArrays(GL_QUADS, 0, 4);

	vec2f_add(&ctx->pos, win->base.child_position);

	if (win->content)
		gui_draw_element(win->content, ctx);
}

void gui_window_destroy(gui_window_t* win)
{
	if (win->content)
		gui_destroy_elements(win->content);
}

void gui_static_container_calcdim(gui_static_container_t* sc)
{
	sc->base.child_position = VEC2F(0, 0);

	float cx = 0.f, cy = 0.f;

	for (uint32_t i = 0; i < sc->n_children; i++) {
		gui_element_t* child = sc->children[i];

		child->position = VEC2F(cx, cy);
		gui_calculate_dimensions(child);

		if (sc->base.flags & GUI_STATIC_CONTAINER_HORIZONTAL_FLAG)
			cx += 5.f + child->size.x;
		else
			cy += 5.f + child->size.y;

		if (sc->base.flags & GUI_STATIC_CONTAINER_HORIZONTAL_FLAG) {
			sc->base.size.x += child->size.x;
			sc->base.size.y = max(sc->base.size.y, child->size.y);
		}
		else {
			sc->base.size.x = max(sc->base.size.x, child->size.x);
			sc->base.size.y += child->size.y;
		}
	}

	if (sc->base.flags & GUI_STATIC_CONTAINER_HORIZONTAL_FLAG)
		sc->base.size.x += (sc->n_children - 1) * 5.f;
	else
		sc->base.size.y += (sc->n_children - 1) * 5.f;
}

void gui_static_container_draw(gui_static_container_t* sc, gui_context_t* ctx)
{
	for (uint32_t i = 0; i < sc->n_children; i++) {
		gui_context_t new_ctx;
		new_ctx.pos = vec2f_sum(ctx->pos, sc->children[i]->position);
		gui_draw_element(sc->children[i], &new_ctx);
	}
}

void gui_static_container_destroy(gui_static_container_t* sc)
{
	for (uint32_t i = 0; i < sc->n_children; i++) {
		gui_destroy_elements(sc->children[i]);
	}

	if (sc->base.flags & GUI_STATIC_CONTAINER_DYNAMIC_MEM) {
		sys_free(sc->children);
	}
}

void gui_label_calcdim(gui_label_t* label)
{
	label->base.size = font_measure_text(label->font, label->buf);
}

void gui_label_draw(gui_label_t* label, gui_context_t* ctx)
{
	gfx_draw_text(label->buf, label->font, VEC3F(label->base.position.x + ctx->pos.x, label->base.position.y + ctx->pos.y, 0.f), VEC3F(1.f, 1.f, 1.f, 1.f));
}

void gui_label_destroy(gui_label_t* label)
{
	if (label->base.flags & GUI_LABEL_DEALLOCATE_ON_DESTROY_FLAG) {
		sys_free(label->buf);
	}
}

void gui_init_window(gui_window_t* win, gui_element_t* content, uint32_t flags)
{
	memset(win, 0, sizeof(gui_element_t));
	win->content = content;
	win->base.flags = flags;
	win->base.type = &gui_window_vtable;
}

void gui_init_label(gui_label_t* label, font_t* font, const char* buf, uint32_t flags)
{
	memset(label, 0, sizeof(gui_element_t));
	label->font = font;
	label->buf = buf;
	label->base.flags = flags;
	label->base.type = &gui_label_vtable;
}

void gui_init_static_container(gui_static_container_t* sc, uint32_t n_children, gui_element_t** children, uint32_t flags)
{
	memset(sc, 0, sizeof(gui_element_t));
	sc->n_children = n_children;
	sc->children = children;
	sc->base.flags = flags;
	sc->base.type = &gui_static_container_vtable;
}

GUI_MAKE_VTABLE(gui_window_vtable, gui_window_draw, gui_window_calcdim, gui_window_destroy);
GUI_MAKE_VTABLE(gui_static_container_vtable, gui_static_container_draw, gui_static_container_calcdim, gui_static_container_destroy);
GUI_MAKE_VTABLE(gui_label_vtable, gui_label_draw, gui_label_calcdim, gui_label_destroy);

void gui_calculate_dimensions(gui_element_t* element)
{
	element->type->calcdim(element);
}

void gui_draw_element(gui_element_t* element, gui_context_t* ctx)
{
	element->type->draw(element, ctx);
}

void gui_destroy_elements(gui_element_t* element)
{
	element->type->destroy(element);

	if (element->flags & GUI_DYNAMIC_MEMORY_FLAG) {
		sys_free(element);
	}
}