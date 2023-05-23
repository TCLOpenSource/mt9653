# mt9653

# 0. download an generic arm architecture toolchains
 
# 1. build kernel
 
## cd kernel path
cd kernel
 
## generate configs
cp arch/arm64/configs/mt5896_defconfig .config

make menuconfig
 
## compile with prebuilts toolchains
make -jN ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE}


