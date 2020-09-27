# Recipe for building the MDM9607 Kernel tree
# Biktorgj 2020
# Released under the MIT license (see COPYING.MIT for the terms)

inherit kernel
inherit pythonnative

DESCRIPTION = "Linux Kernel for Qualcomm MDM9607-MTP"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

# Set compatible machines for this kernel
COMPATIBLE_MACHINE = "(mdm9607|apq8009|apq8096|apq8053|msm8909w)"
# Dependencies: skales-native (provides mkbootimg,dtbtool), gcc
DEPENDS += "mkbootimg-native dtbtool-native libgcc python-native dtc-native"

# Base paths
SRC_URI   =  "git://github.com/Biktorgj/quectel_eg25_kernel.git;branch=linux-3.18.140"
SRCREV = "linux-3.18.140"
# SRC_URI   =  "git://github.com/Biktorgj/Linux-4.14.180-CAF.git;branch=master"
# SRCREV = "master"
S = "${WORKDIR}/git"

PR = "${DISTRO}"

KERNEL_IMAGETYPE ?= "zImage"
KERNEL_DEFCONFIG_mdm9607 ?= "${S}/arch/arm/configs/mdm9607-perf_defconfig"
KERNEL_DEVICETREE ?= "qcom/mdm9607-mtp.dtb"

QCOM_BOOTIMG_ROOTFS ?= "undefined"
SD_QCOM_BOOTIMG_ROOTFS ?= "undefined"
# set output file names
BOOT_IMAGE_BASE_NAME = "boot-${KERNEL_IMAGE_NAME}"
BOOT_IMAGE_SYMLINK_NAME = "boot-${KERNEL_IMAGE_LINK_NAME}"
# CMDLine params
QCOM_BOOTIMG_PAGE_SIZE = "2048"
KERNEL_CMDLINE = "noinitrd ro console=ttyHS0,115200,n8 androidboot.hardware=qcom ehci-hcd.park=3 msm_rtb.filter=0x37 lpm_levels.sleep_disabled=1 verbose"
QCOM_BOOTIMG_KERNEL_BASE = "0x80000000"
KERNEL_TAGS_ADDR = "0x81E00000"
kernel_conf_variable() {
	CONF_SED_SCRIPT="$CONF_SED_SCRIPT /CONFIG_$1[ =]/d;"
	if test "$2" = "n"
	then
		echo "# CONFIG_$1 is not set" >> ${B}/.config
	else
		echo "CONFIG_$1=$2" >> ${B}/.config
	fi
}


do_configure_prepend() {
	echo "" > ${B}/.config
	CONF_SED_SCRIPT=""

	kernel_conf_variable LOCALVERSION "\"${LOCALVERSION}\""
	kernel_conf_variable LOCALVERSION_AUTO y

	if [ -f '${WORKDIR}/defconfig' ]; then
		sed -e "${CONF_SED_SCRIPT}" < '${WORKDIR}/defconfig' >> '${B}/.config'
	else
		sed -e "${CONF_SED_SCRIPT}" < '${KERNEL_DEFCONFIG}' >> '${B}/.config'
	fi

	if [ "${SCMVERSION}" = "y" ]; then
		# Add GIT revision to the local version
		head=`git --git-dir=${S}/.git  rev-parse --verify --short HEAD 2> /dev/null`
		printf "%s%s" +g $head > ${B}/.scmversion
	fi

	yes '' | oe_runmake -C ${S} O=${B} oldconfig
        oe_runmake -C ${S} O=${B} savedefconfig && cp ${B}/defconfig ${WORKDIR}/defconfig.saved
}

# append DTB
do_compile_append() {
#    if ! [ -e ${B}/arch/arm/boot/dts/${KERNEL_DEVICETREE} ] ; then
#        oe_runmake ${KERNEL_DEVICETREE}
#    fi
   # cp arch/arm/boot/${KERNEL_IMAGETYPE} arch/arm/boot/${KERNEL_IMAGETYPE}.backup
    ${STAGING_BINDIR_NATIVE}/dtbTool ${B}/arch/arm/boot/dts/qcom/ -s 2048 -o ${B}/arch/arm/boot/dts/qcom/dtb.img -p scripts/dtc/ -v
   # cat arch/arm/boot/${KERNEL_IMAGETYPE}.backup arch/arm/boot/dts/qcom/masterDTB > arch/arm/boot/${KERNEL_IMAGETYPE}
  #  rm -f arch/arm/boot/${KERNEL_IMAGETYPE}.backup
}


# param ${1} partition where rootfs is located
# param ${2} output boot image file name
priv_make_image() {
    ${STAGING_BINDIR_NATIVE}/mkbootimg --kernel ${B}/arch/arm/boot/${KERNEL_IMAGETYPE} \
              --ramdisk ${B}/initrd.img \
              --output ${DEPLOYDIR}/${2}.img \
              --pagesize ${QCOM_BOOTIMG_PAGE_SIZE} \
              --base ${QCOM_BOOTIMG_KERNEL_BASE} \
              --tags-addr ${KERNEL_TAGS_ADDR} \
              --dt ${B}/arch/arm/boot/dts/qcom/dtb.img \
              --cmdline "${KERNEL_CMDLINE}"
}

do_deploy_append() {
    # mkbootimg requires an initrd file, make fake one that will be ignored
    # during boot
    echo "" > ${B}/initrd.img

    # don't build bootimg if rootfs partition is not defined
    if [ "${QCOM_BOOTIMG_ROOTFS}" == "undefined"]; then
        bbfatal "Rootfs partition must be defined"
    fi

    priv_make_image ${QCOM_BOOTIMG_ROOTFS} ${BOOT_IMAGE_BASE_NAME}
    ln -sf ${BOOT_IMAGE_BASE_NAME}.img ${DEPLOYDIR}/${BOOT_IMAGE_SYMLINK_NAME}.img
}
