#ifndef UI_BACKDROP_H
#define UI_BACKDROP_H

#include "ui/inset.h"
#include "ui/value.h"
#include "ui/color.h"

#include <gfx/objects.h>

#ifdef interface
# undef interface
#endif

struct blp_texture;
struct xml_backdrop;
struct interface;
struct ui_region;

struct ui_backdrop
{
	struct interface *interface;
	gfx_attributes_state_t edge_attributes_state;
	gfx_attributes_state_t bg_attributes_state;
	gfx_buffer_t edge_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t bg_uniform_buffers[RENDER_FRAMES_COUNT];
	gfx_buffer_t edge_vertexes_buffer;
	gfx_buffer_t edge_indices_buffer;
	gfx_buffer_t bg_vertexes_buffer;
	gfx_buffer_t bg_indices_buffer;
	struct ui_inset background_insets;
	struct ui_value edge_size;
	struct ui_value tile_size;
	struct ui_color border_color;
	struct ui_color color;
	char *edge_file;
	char *bg_file;
	bool tile;
	struct blp_texture *edge_texture;
	struct blp_texture *bg_texture;
	struct ui_region *parent;
	bool edge_initialized;
	bool bg_initialized;
	bool dirty_size;
};

struct ui_backdrop *ui_backdrop_new(struct interface *interface, struct ui_region *parent);
void ui_backdrop_delete(struct ui_backdrop *backdrop);
void ui_backdrop_load_xml(struct ui_backdrop *backdrop, const struct xml_backdrop *xml);
void ui_backdrop_render(struct ui_backdrop *backdrop);

#endif
