CURRENT=$(pwd)
SRC=~/embedded-linux/u-boot/u-boot_v2026.07
BUILD_R5=~/embedded-linux/u-boot/build/beagley-ai-r5-v2026.07
BUILD_A53=~/embedded-linux/u-boot/build/beagley-ai-a53-v2026.07

export CC32=arm-linux-gnueabihf-
export CC64=aarch64-linux-gnu-
export LNX_FW_PATH=${CURRENT}/ti-linux-firmware
export TFA_PATH=${CURRENT}/arm-trusted-firmware
export OPTEE_PATH=${CURRENT}/optee_os

export UBOOT_CFG_CORTEXR=am67a_beagley_ai_r5_defconfig
export UBOOT_CFG_CORTEXA=am67a_beagley_ai_a53_defconfig
export TFA_BOARD=lite
# we dont use any extra TFA parameters
unset TFA_EXTRA_ARGS
export OPTEE_PLATFORM=k3-am62x

cd arm-trusted-firmware
make \
    CROSS_COMPILE=$CC64 \
    ARCH=aarch64 \
    PLAT=k3 \
    SPD=opteed \
    $TFA_EXTRA_ARGS \
    TARGET_BOARD=$TFA_BOARD \
    -j"$(nproc)" \
    || exit 1
cd -

cd optee_os
make \
    CROSS_COMPILE=$CC32 \
    CROSS_COMPILE64=$CC64 \
    CFG_ARM64_core=y \
    $OPTEE_EXTRA_ARGS \
    PLATFORM=$OPTEE_PLATFORM \
    -j"$(nproc)" \
    || exit 1
cd -

# R5
make \
    -C "$SRC" \
    O="$BUILD_R5" \
    $UBOOT_CFG_CORTEXR \
    || exit 1

make \
    -C "$SRC" \
    O="$BUILD_R5" \
    CROSS_COMPILE=$CC32 \
    BINMAN_INDIRS=$LNX_FW_PATH \
    -j"$(nproc)" \
    || exit 1

# A53
make \
    -C "$SRC" \
    O="$BUILD_A53" \
    $UBOOT_CFG_CORTEXA \
    || exit 1

make \
    -C "$SRC" \
    O="$BUILD_A53" \
    CROSS_COMPILE=$CC64 \
    BINMAN_INDIRS=$LNX_FW_PATH \
    BL31=$TFA_PATH/build/k3/$TFA_BOARD/release/bl31.bin \
    TEE=$OPTEE_PATH/out/arm-plat-k3/core/tee-raw.bin \
    -j"$(nproc)" \
    || exit 1

mkdir -p build/result
cp ${BUILD_A53}/u-boot.img ${BUILD_A53}/tispl.bin build/result
cp ${BUILD_R5}/tiboot3-j722s-hs-fs-evm.bin build/result/tiboot3.bin 