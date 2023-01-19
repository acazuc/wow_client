#ifndef FONT_MODEL_H
#define FONT_MODEL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct FT_FaceRec_* FT_Face;

struct font_model
{
	FT_Face ft_face;
	void *data;
};

struct font_model *font_model_new(const char *data, size_t len);
void font_model_delete(struct font_model *model);
bool font_model_set_size(struct font_model *model, uint32_t size);

#endif
