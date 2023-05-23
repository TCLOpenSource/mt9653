#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

green='\e[0;32m'
red='\e[0;31m'
eol='\e[0m'

BASE_DIR=$PWD
ABI_DIR=$BASE_DIR/scripts/abi
ABIGAIL_DIR=$BASE_DIR/../kernel/build/abi
ABIGAIL_BUILD_SCRIPT=$ABIGAIL_DIR/bootstrap_src_build
ABI_XML_DIR=$BASE_DIR/scripts/abi/abi_xml
#target_kernel must create outside current kernel's repo
TARGET_KERNEL_DIR=$BASE_DIR/../../target_kernel_quark5.4
TARGET_DEFCONFIG=mtk_gki_defconfig
TARGET_DEFCONFIG_PATCH=$ABI_DIR/mtk_gki_defconfig_patch
TARGET_ABI_XML=abi_$src_defconfig.xml
TARGET_VMLINUX_DIR=$TARGET_KERNEL_DIR/common/out_vmlinux

echo "Get ABIGAIL_VERSION from $ABIGAIL_BUILD_SCRIPT"
ABIGAIL_VERSION=`grep "ABIGAIL_VERSION=" $ABIGAIL_BUILD_SCRIPT | cut -f2- -d=`
ABIGAIL_DIR_RELEASE=$ABIGAIL_DIR/abigail-inst/$ABIGAIL_VERSION
echo "ABIGAIL_DIR_RELEASE=$ABIGAIL_DIR_RELEASE"


cd $TARGET_KERNEL_DIR
TARGET_KERNEL_DIR_RELATED_PATH=`pwd`
TARGET_KERNEL_OUT_DIR_RELATED_PATH=$TARGET_KERNEL_DIR_RELATED_PATH/common/temp/out/..
echo "TARGET_KERNEL_DIR_RELATED_PATH=$TARGET_KERNEL_DIR_RELATED_PATH"
echo "TARGET_KERNEL_OUT_DIR_RELATED_PATH=$TARGET_KERNEL_OUT_DIR_RELATED_PATH"
cd $BASE_DIR

function print_usage(){
	echo -e "${green}Script for auto generate target_branch's ABI xml \
based on src_defconfig ${eol}"
	echo ""
	echo -e "${red}Command for local test:${eol}"
	echo "[src_commit] [src_defconfig] [target_branch] [target_commit] \
mode=m ./scripts/abi/genOriABIxml.sh"
	echo ""
	echo -e "${green}Description:${eol}"
	echo "[src_commit]: source kernel commit id"
	echo "[src_defconfig]: source project defconfig"
	echo "[target_branch]: target branch"
	echo "[target_commit]: target kernel commit id"
	echo ""
	echo -e "${green}Example:${eol} ${red}src_commit=491f0e3 \
src_defconfig=k6873k_64_gki_defconfig \
target_branch=common-android11-5.4 target_commit=f232ce6 mode=m \
./scripts/abi/genOriABIxml.sh 2>&1 | tee buildOriABI.log${eol}"
	echo ""
	echo -e "${green}Script for auto generate target_branch's ABI xml \
for preflight based on src_defconfig ${eol}"
	echo ""
	echo -e "${red}Command for local test:${eol}"
	echo "[src_defconfig] [target_branch] mode=p \
./scripts/abi/genOriABIxml.sh"
	echo ""
	echo -e "${green}Description:${eol}"
	echo "[src_defconfig]: source project defconfig"
	echo "[target_branch]: target branch"
	echo ""
	echo -e "${green}Example:${eol} ${red}src_defconfig=\
k6873v1_64_gki_defconfig \
target_branch=common-android11-5.4 mode=p \
./scripts/abi/genOriABIxml.sh 2>&1 | tee buildOriABI.log${eol}"
	echo ""
	echo -e "${red}Command for delete temp files:${eol}"
	echo "mode=d ./scripts/abi/genOriABIxml.sh"
	echo ""
	echo -e "${green}Example:${eol} ${red}mode=d \
./scripts/abi/genOriABIxml.sh${eol}"
}

function del_temp_files(){
	echo "Start delete temp files..."
	echo "Delete temp files $BASE_DIR/temp/out"
	rm -rf $BASE_DIR/temp/out
	echo "Delete temp files $TARGET_KERNEL_DIR"
	rm -rf $TARGET_KERNEL_DIR
	#rm -rf $ABI_XML_DIR/$TARGET_ABI_XML
}

if [[ "$1" == "h" ]] || [[ "$1" == "help" ]] || [ -z "mode" ]
then
	print_usage
fi

if [ "$mode" == "d" ]
then
	del_temp_files
fi

if [ "$mode" == "p" ]
then
target_commit=7b6c8a87129dc4a5defb23d90420610e60a74540 #ACK5.4.61->latest
echo src_commit=$src_commit
echo target_commit=$target_commit
fi

if [ "$mode" == "m" ] || [ "$mode" == "p" ]
then
	#Check Terminal server support "mosesq" or "dockerq"
	echo "Start to test sever queue..."
	test_queue=$(mosesq ls 2>&1)
	if [[ "$test_queue" =~ \
		[Hh]ost\ group ]];
	then
		SERVER_QUEUE=dockerq
	else
		SERVER_QUEUE=mosesq
	fi
	echo "This sever is using queue: $SERVER_QUEUE"
	#Build libabigail first
	$ABIGAIL_BUILD_SCRIPT
	#remove temp files first
	del_temp_files
	cd ..
	export PATH=\
$PWD/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/:\
$PWD/prebuilts/clang/host/linux-x86/clang-r383902/bin/:$PATH

	echo "Generate ABI xml:$TARGET_ABI_XML of target_branch:$target_branch \
with commit id:$target_commit"
	mkdir $TARGET_KERNEL_DIR
	cd $TARGET_KERNEL_DIR
	git clone "http://gerrit.mediatek.inc:8080/dtv/fusion-bsp2/kernel/linaro" -b quark-5.4_generic-trunk

	ln -s linaro common
	cd common
	git checkout $target_commit
	echo "Move gki_defconfig from arch/arm64/configs/ to \
$PWD/arch/arm64/configs/$TARGET_DEFCONFIG"
	cp arch/arm64/configs/gki_defconfig arch/arm64/configs/$TARGET_DEFCONFIG
	#Modify $TARGET_DEFCONFIG configs according to $TARGET_DEFCONFIG_PATCH
	exec < $TARGET_DEFCONFIG_PATCH
	while read line
	do
		#get string before "="
		config_=${line%%=*}
		#replace matched string
		sed -i "/$config_/c\\$line" \
			$PWD/arch/arm64/configs/$TARGET_DEFCONFIG
	done

	make ARCH=arm64 CLANG_TRIPLE=aarch64-linux-gnu- \
	CROSS_COMPILE=aarch64-linux-android- CC=clang $TARGET_DEFCONFIG O=temp/out
	$SERVER_QUEUE make ARCH=arm64 CLANG_TRIPLE=aarch64-linux-gnu- \
	CROSS_COMPILE=aarch64-linux-android- CC=clang O=temp/out -j24 -k

	#Only use vmlinux to generate ABI xml
	echo "Copy vmlinux from $PWD/temp/out to $PWD/out_vmlinux"
	mkdir out_vmlinux
	cp temp/out/vmlinux out_vmlinux

	#Use abi_dump to generate $TARGET_ABI_XML
	#export $ABIGAIL_DIR bin and lib
	echo "Generate $ABI_XML_DIR/$TARGET_ABI_XML from vmlinux \
in $TARGET_VMLINUX_DIR"
	export PATH=${ABIGAIL_DIR_RELEASE}/bin:${PATH}
	export LD_LIBRARY_PATH=${ABIGAIL_DIR_RELEASE}/lib:\
${ABIGAIL_DIR_RELEASE}/lib/elfutils:${LD_LIBRARY_PATH}
	cd $ABIGAIL_DIR
	python dump_abi --linux-tree $TARGET_VMLINUX_DIR \
		--out-file $ABI_XML_DIR/$TARGET_ABI_XML

	# sanitize the abi.xml by removing any occurences of the kernel path
	cd $TARGET_KERNEL_DIR
        sed -i "s#$TARGET_KERNEL_OUT_DIR_RELATED_PATH/##g" \
	$ABI_XML_DIR/$TARGET_ABI_XML
	# now also do that with any left over paths sneaking in
	# (e.g. from the prebuilts)
        sed -i "s#$TARGET_KERNEL_DIR_RELATED_PATH/##g" \
	$ABI_XML_DIR/$TARGET_ABI_XML

	#remove temp files
	del_temp_files
fi
