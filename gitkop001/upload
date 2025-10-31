#!/bin/bash
set -e

pushd build
echo "Building project..."
make clean
make -j$(nproc)

echo "Converting to binary..."
arm-none-eabi-objcopy -O binary *.elf firmware.bin

echo "Programming device..."
STM32_Programmer_CLI -c port=SWD -w *.elf -v -rst

echo "Done!"
popd
