#!/bin/bash
RAMDISK_TOP_DIR=$PWD/ramdisk_tmp
if [ -d ${RAMDISK_TOP_DIR} ];then
	echo "Please use initramfs-extrafile.sh to generate ramdisk_tmp!!"
	exit 1
fi
pushd $RAMDISK_TOP_DIR
find . | cpio -H newc -o > ramdisk.cpio
popd
gzip -f ramdisk.cpio
