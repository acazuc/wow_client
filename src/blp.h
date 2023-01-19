#ifndef BLP_TEXTURE_H
#define BLP_TEXTURE_H

#include <gfx/objects.h>

#include <stdbool.h>

struct blp_texture_mipmap;
struct wow_blp_file;

struct blp_texture
{
	struct blp_texture_mipmap *mipmaps;
	enum gfx_format format;
	uint32_t mipmaps_nb;
	bool load_asked;
	bool loading;
	bool loaded;
	bool initialized;
	gfx_texture_t texture;
	char *filename;
	uint32_t width;
	uint32_t height;
};

struct blp_texture *blp_texture_new(const char *filename);
struct blp_texture *blp_texture_from_data(const uint8_t *data, uint32_t width, uint32_t height);
void blp_texture_delete(struct blp_texture *texture);
void blp_texture_ask_load(struct blp_texture *texture);
void blp_texture_ask_unload(struct blp_texture *texture);
bool blp_texture_load(struct blp_texture *texture, struct wow_blp_file *file);
int blp_texture_initialize(struct blp_texture *texture);
void blp_texture_bind(struct blp_texture *texture, uint8_t slot);

bool blp_decode_rgba(const struct wow_blp_file *file, uint8_t mipmap_id, uint32_t *width, uint32_t *height, uint8_t **data);

#endif
