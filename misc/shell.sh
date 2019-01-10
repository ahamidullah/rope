#!/bin/bash
export VULKAN_SDK_VERSION="1.1.70.1"
#export PROJECT_PATH=$(readlink -e ..)

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
	DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
	SOURCE="$(readlink "$SOURCE")"
	[[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
export PROJECT_PATH="$( cd -P "$( dirname "$SOURCE" )"/.. && pwd )"

cd "$PROJECT_PATH"
export VULKAN_SDK_PATH="$PROJECT_PATH/dependencies/vulkan_sdk/$VULKAN_SDK_VERSION"
export LD_LIBRARY_PATH="$VULKAN_SDK_PATH/vulkan/lib_x64:${LD_LIBRARY_PATH:-}"
export VK_LAYER_PATH="$VULKAN_SDK_PATH/vulkan/layers"
PATH="$PROJECT_PATH/build:$PROJECT_PATH/misc:$PATH"

