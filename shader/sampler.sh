#!/bin/zsh

source $HOME/.zshrc

TARGET=buf_and_img

cls $TARGET.cl -o $TARGET.spv -samplermap=sampler_map -descriptormap=out.map
sd $TARGET.spv -o $TARGET.spvasm

echo "const uint32_t spirv[] = {" > spirv.inc
hexdump -v -e '8/4 "0x%08x, " "\n"' $TARGET.spv >> spirv.inc
echo "};" >> spirv.inc
