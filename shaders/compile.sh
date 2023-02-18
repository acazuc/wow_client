#!/bin/sh

gcc compile.c -O2 -o compile || exit 1

vs_shaders=`ls *.vs.gfx`
fs_shaders=`ls *.fs.gfx`

if [ "$API" = "" ]
then
	API="gl3 gl4 gles3 vk d3d9 d3d11"
fi

GL3=0
GL4=0
GLES3=0
VK=0
D3D9=0
D3D11=0

for api in $API
do
	case $api in
		"gl3")
			GL3=1
			;;
		"gl4")
			GL4=1
			;;
		"gles3")
			GLES3=1
			;;
		"vk")
			VK=1
			;;
		"d3d9")
			D3D9=1
			;;
		"d3d11")
			D3D11=1
			;;
		*)
			echo "unknown API: $api"
			exit 1
	esac
done

if [ "$GL3" -ne "0" ]
then
	mkdir -p gl3
fi

if [ "$GL4" -ne "0" ]
then
	mkdir -p gl4
fi

if [ "$GLES3" -ne "0" ]
then
	mkdir -p gles3
fi

if [ "$VK" -ne "0" ]
then
	mkdir -p vk
fi

if [ "$D3D9" -ne "0" ]
then
	mkdir -p d3d9
fi

if [ "$D3D11" -ne "0" ]
then
	mkdir -p d3d11
fi

for shader in $vs_shaders
do
	glsl_output=$(echo "$shader" | sed 's/.gfx/.glsl/')
	hlsl_output=$(echo "$shader" | sed 's/.gfx/.hlsl/')

	if [ "$GL3" -ne "0" ]
	then
		./compile -t vs -x gl3 -i "$shader" -o "gl3/$glsl_output" || exit 1
	fi

	if [ "$GL4" -ne "0" ]
	then
		./compile -t vs -x gl4 -i "$shader" -o "gl4/$glsl_output" || exit 1
	fi

	if [ "$GLES3" -ne "0" ]
	then
		./compile -t vs -x gles3 -i "$shader" -o "gles3/$glsl_output" || exit 1
	fi

	if [ "$VK" -ne "0" ]
	then
		./compile -t vs -x vk -i "$shader" -o "vk/$glsl_output" || exit 1
		glslangValidator -V -S vert -o "vk/$glsl_output.spv" "vk/$glsl_output" || exit 1
	fi

	if [ "$D3D9" -ne "0" ]
	then
		./compile -t vs -x d3d9 -i "$shader" -o "d3d11/$hlsl_output" || exit 1
	fi

	if [ "$D3D11" -ne "0" ]
	then
		./compile -t vs -x d3d11 -i "$shader" -o "d3d11/$hlsl_output" || exit 1
	fi
done

for shader in $fs_shaders
do
	glsl_output=$(echo "$shader" | sed 's/.gfx/.glsl/')
	hlsl_output=$(echo "$shader" | sed 's/.gfx/.hlsl/')

	if [ "$GL3" -ne "0" ]
	then
		./compile -t fs -x gl3 -i "$shader" -o "gl3/$glsl_output" || exit 1
	fi

	if [ "$GL4" -ne "0" ]
	then
		./compile -t fs -x gl4 -i "$shader" -o "gl4/$glsl_output" || exit 1
	fi

	if [ "$GLES3" -ne "0" ]
	then
		./compile -t fs -x gles3 -i "$shader" -o "gles3/$glsl_output" || exit 1
	fi

	if [ "$VK" -ne "0" ]
	then
		./compile -t fs -x vk -i "$shader" -o "vk/$glsl_output" || exit 1
		glslangValidator -V -S frag -o "vk/$glsl_output.spv" "vk/$glsl_output" || exit 1
	fi

	if [ "$D3D9" -ne "0" ]
	then
		./compile -t fs -x d3d9 -i "$shader" -o "d3d11/$hlsl_output" || exit 1
	fi

	if [ "$D3D11" -ne "0" ]
	then
		./compile -t fs -x d3d11 -i "$shader" -o "d3d11/$hlsl_output" || exit 1
	fi
done
