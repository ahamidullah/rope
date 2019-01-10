#!/bin/bash

# Exit on error.
set -e

pushd . >& /dev/null
cd "$PROJECT_PATH"/code

if [ ! -d ../build ]; then
	mkdir ../build
fi

if [ ! -d ../build/textures ]; then
	mkdir ../build/textures
fi

if [ ! -d ../build/sprite_data ]; then
	mkdir ../build/sprite_data
fi

COMPILER_FLAGS="-g -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -fno-exceptions -fno-threadsafe-statics -I/usr/include/freetype2"
LINKER_FLAGS="-lm -lX11 -lGL -lfreetype -lasound -lpthread"

# Compile shaders.
#"$VULKAN_SDK_PATH"/glslangValidator -V shader.vert -o ../build/vert.spirv
#"$VULKAN_SDK_PATH"/glslangValidator -V shader.frag -o ../build/frag.spirv

if [ "$#" -eq 1 ] && [ "$1" == "assets" ]; then
	pushd . >& /dev/null
	cd asset_packer
	g++ -std=c++17 -g asset_packer.cpp -o ../../build/asset_packer
	/usr/bin/time --format='Asset pack time: %es.' ../../build/asset_packer
	popd >& /dev/null
fi

/usr/bin/time --format='Build time: %es.' gcc -std=c++17 $COMPILER_FLAGS cge.cpp $LINKER_FLAGS -o ../build/cge

popd >& /dev/null

#../build/cge

