#!/bin/sh
gcc compile.c -o compile || exit 1
vs_shaders=`ls *.vs.gfx`
fs_shaders=`ls *.fs.gfx`
mkdir -p gl3
mkdir -p gl4
mkdir -p gles3
mkdir -p vk
mkdir -p d3d9
mkdir -p d3d11
for shader in $vs_shaders
do
	glsl_output=$(echo "$shader" | sed 's/.gfx/.glsl/')
	hlsl_output=$(echo "$shader" | sed 's/.gfx/.hlsl/')
	./compile -t vs -x gl4 -i "$shader" -o "gl4/$glsl_output" || exit 1
	./compile -t vs -x vk -i "$shader" -o "vk/$glsl_output" || exit 1
	./compile -t vs -x d3d11 -i "$shader" -o "d3d11/$hlsl_output" || exit 1
done

for shader in $fs_shaders
do
	glsl_output=$(echo "$shader" | sed 's/.gfx/.glsl/')
	hlsl_output=$(echo "$shader" | sed 's/.gfx/.hlsl/')
	./compile -t fs -x gl4 -i "$shader" -o "gl4/$glsl_output" || exit 1
	./compile -t fs -x vk -i "$shader" -o "vk/$glsl_output" || exit 1
	./compile -t fs -x d3d11 -i "$shader" -o "d3d11/$hlsl_output" || exit 1
done
