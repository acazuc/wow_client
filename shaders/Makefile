SHADERS = aabb \
          basic \
          bloom \
          bloom_blur \
          bloom_merge \
          cel \
          chromaber \
          fsaa \
          fxaa \
          glow \
          gui \
          m2 \
          m2_bones \
          m2_collisions \
          m2_lights \
          mclq_magma \
          mclq_water \
          mcnk \
          mliq \
          particle \
          ribbon \
          sharpen \
          skybox \
          sobel \
          ssao \
          ssao_denoiser \
          text \
          ui \
          wdl \
          wmo \
          wmo_collisions \
          wmo_portals \

TYPES =

ifeq ($(WITH_GL3), YES)
TYPES+= gl3
endif

ifeq ($(WITH_GL4), YES)
TYPES+= gl4
endif

ifeq ($(WITH_GLES3), YES)
TYPES+= gles3
endif

ifeq ($(WITH_VK), YES)
TYPES+= vk
endif

ifeq ($(WITH_D3D9), YES)
TYPES+= d3d9
endif

ifeq ($(WITH_D3D11), YES)
TYPES+= d3d11
endif

EMPTY = 
SPACE = $(EMPTY) $(EMPTY)
COMMA = ,
SHADERS_TYPES = $(subst $(SPACE),$(COMMA),$(TYPES))

IN_FILES = $(addsuffix .vs.gfxs, $(SHADERS)) $(addsuffix .fs.gfxs, $(SHADERS))

OUT_FILES = $(addsuffix .vs.gfx, $(SHADERS)) $(addsuffix .fs.gfx, $(SHADERS))

all: $(OUT_FILES)

%.vs.gfx:%.vs.gfxs compile
	@mkdir -p $(dir $@)
	@echo "compile $<"
	@./compile -t vs -x $(SHADERS_TYPES) -i "$<" -o "$@"

%.fs.gfx:%.fs.gfxs compile
	@mkdir -p $(dir $@)
	@echo "compile $<"
	@./compile -t fs -x $(SHADERS_TYPES) -i "$<" -o "$@"

compile: compile.c
	@echo "CC $<"
	@$(CC) -O2 -g -o $@ $<

clean:
	@rm -rf gl3 gl4 gles3 vk d3d9 d3d11

.PHONY: clean
