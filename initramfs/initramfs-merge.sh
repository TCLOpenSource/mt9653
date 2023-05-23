#!/bin/bash
INITRAMFS_ROOT_DIR=$PWD
RAMDISK_LIB_MODULE="../OUT/initramfs-ko/"

pushd ${RAMDISK_LIB_MODULE}
find . | cpio -H newc -o > ${INITRAMFS_ROOT_DIR}/initramfs_ko.cpio
popd
gzip -f initramfs_ko.cpio

cat initramfs_basic.cpio.gz initramfs_ko.cpio.gz > ramdisk.cpio.gz 
cp ramdisk.cpio.gz ~/mfs/tftp/
printf "ramdisk size: 0x%x\n" `stat -c "%s" ramdisk.cpio.gz`
