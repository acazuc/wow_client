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

OUT_SHADERS =

ifeq ($(WITH_GL3), YES)

OUT_SHADERS+= $(addprefix gl3/, $(addsuffix .vs.glsl, $(SHADERS)))
OUT_SHADERS+= $(addprefix gl3/, $(addsuffix .fs.glsl, $(SHADERS)))

endif

ifeq ($(WITH_GL4), YES)

OUT_SHADERS+= $(addprefix gl4/, $(addsuffix .vs.glsl, $(SHADERS)))
OUT_SHADERS+= $(addprefix gl4/, $(addsuffix .fs.glsl, $(SHADERS)))

endif

ifeq ($(WITH_GLES3), YES)

OUT_SHADERS+= $(addprefix gles3/, $(addsuffix .vs.glsl, $(SHADERS)))
OUT_SHADERS+= $(addprefix gles3/, $(addsuffix .fs.glsl, $(SHADERS)))

endif

ifeq ($(WITH_VK), YES)

OUT_SHADERS+= $(addprefix vk/, $(addsuffix .vs.glsl, $(SHADERS)))
OUT_SHADERS+= $(addprefix vk/, $(addsuffix .fs.glsl, $(SHADERS)))

endif

ifeq ($(WITH_D3D9), YES)

OUT_SHADERS+= $(addprefix d3d9/, $(addsuffix .vs.glsl, $(SHADERS)))
OUT_SHADERS+= $(addprefix d3d9/, $(addsuffix .fs.glsl, $(SHADERS)))

endif

ifeq ($(WITH_D3D11), YES)

OUT_SHADERS+= $(addprefix d3d11/, $(addsuffix .vs.glsl, $(SHADERS)))
OUT_SHADERS+= $(addprefix d3d11/, $(addsuffix .fs.glsl, $(SHADERS)))

endif

all: compile $(OUT_SHADERS)

gl3/%.vs.glsl: %.vs.gfx
	@mkdir -p $(dir $@)
	@echo "compile gl3 $<"
	@./compile -t vs -x gl3 -i "$<" -o "$@"

gl3/%.fs.glsl: %.fs.gfx
	@mkdir -p $(dir $@)
	@echo "compile gl3 $<"
	@./compile -t fs -x gl3 -i "$<" -o "$@"

gl4/%.vs.glsl: %.vs.gfx
	@mkdir -p $(dir $@)
	@echo "compile gl4 $<"
	@./compile -t vs -x gl4 -i "$<" -o "$@"

gl4/%.fs.glsl: %.fs.gfx
	@mkdir -p $(dir $@)
	@echo "compile gl4 $<"
	@./compile -t fs -x gl4 -i "$<" -o "$@"

gles3/%.vs.glsl: %.vs.gfx
	@mkdir -p $(dir $@)
	@echo "compile gles3 $<"
	@./compile -t vs -x gles3 -i "$<" -o "$@"

gles3/%.fs.glsl: %.fs.gfx
	@mkdir -p $(dir $@)
	@echo "compile gles3 $<"
	@./compile -t fs -x gles3 -i "$<" -o "$@"

vk/%.vs.glsl: %.vs.gfx
	@mkdir -p $(dir $@)
	@echo "compile vk $<"
	@./compile -t vs -x vk -i "$<" -o "$@"
	@glslangValidator -V -S vert -o "$@" "$<"

vk/%.fs.glsl: %.fs.gfx
	@mkdir -p $(dir $@)
	@echo "compile vk $<"
	@./compile -t fs -x vk -i "$<" -o "$@"
	@glslangValidator -V -S frag -o "$@" "$<"

d3d9/%.vs.glsl: %.vs.gfx
	@mkdir -p $(dir $@)
	@echo "compile d3d9 $<"
	@./compile -t vs -x d3d9 -i "$<" -o "$@"

d3d9/%.fs.glsl: %.fs.gfx
	@mkdir -p $(dir $@)
	@echo "compile d3d9 $<"
	@./compile -t fs -x d3d9 -i "$<" -o "$@"

d3d11/%.vs.glsl: %.vs.gfx
	@mkdir -p $(dir $@)
	@echo "compile d3d11 $<"
	@./compile -t vs -x d3d11 -i "$<" -o "$@"

d3d11/%.fs.glsl: %.fs.gfx
	@mkdir -p $(dir $@)
	@echo "compile d3d11 $<"
	@./compile -t fs -x d3d11 -i "$<" -o "$@"

compile: compile.c
	@echo "CC $<"
	@$(CC) -O2 -o $@ $<
