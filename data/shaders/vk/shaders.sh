#!/bin/sh
vs_shaders=`ls *.vs.glsl`
fs_shaders=`ls *.fs.glsl`
for shader in $vs_shaders
do
	sed -i -e "s/layout(std140) uniform mesh_block/layout(set = 0, binding = 0, std140) uniform mesh_block/g" "$shader"
	sed -i -e "s/layout(std140) uniform model_block/layout(set = 0, binding = 1, std140) uniform model_block/g" "$shader"
	sed -i -e "s/layout(std140) uniform scene_block/layout(set = 0, binding = 2, std140) uniform scene_block/g" "$shader"
	sed -i -e "s/^out fs_block/layout(location = 0) out fs_block/g" "$shader"
	sed -i -e "s/#version 330/#version 450/g" "$shader"
	sed -i -e "s/gl_VertexID/gl_VertexIndex/g" "$shader"
	glslangValidator -V -S vert -o "$shader.spv" "$shader" || exit 1
done

for shader in $fs_shaders
do
	sed -i -e "s/layout(std140) uniform mesh_block/layout(set = 0, binding = 0, std140) uniform mesh_block/g" "$shader"
	sed -i -e "s/layout(std140) uniform model_block/layout(set = 0, binding = 1, std140) uniform model_block/g" "$shader"
	sed -i -e "s/layout(std140) uniform scene_block/layout(set = 0, binding = 2, std140) uniform scene_block/g" "$shader"
	sed -i -e "s/^in fs_block/layout(location = 0) in fs_block/g" "$shader"
	sed -i -e "s/#version 330/#version 450/g" "$shader"
	sed -i -e "s/^uniform sampler2D color_tex/layout(set = 1, binding = 0) uniform sampler2D color_tex/g" "$shader"
	sed -i -e "s/^uniform sampler2D normal_tex/layout(set = 1, binding = 1) uniform sampler2D normal_tex/g" "$shader"
	sed -i -e "s/^uniform sampler2D position_tex/layout(set = 1, binding = 2) uniform sampler2D position_tex/g" "$shader"
	glslangValidator -V -S frag -o "$shader.spv" "$shader" || exit 1
done
