#!/bin/bash

mkdir ramdisk_tmp
cd ramdisk_tmp
gunzip -c < ../ramdisk.cpio.gz | while cpio -i; do :; done

