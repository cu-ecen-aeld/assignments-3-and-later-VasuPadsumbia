#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
GCC_ARM_VERSION=13.3.rel1

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Building the kernel"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j4
    if [ ! -e arch/${ARCH}/boot/Image ]; then
        echo "Kernel Image not found. Build failed."
        exit 1
    else
        echo "Kernel Image built successfully."
    fi
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr/{bin,sbin},var/log}


cd "$OUTDIR"
wget -O gcc-arm.tar.xz https://developer.arm.com/-/media/Files/downloads/gnu/$GCC_ARM_VERSION/binrel/arm-gnu-toolchain-$GCC_ARM_VERSION-x86_64-aarch64-none-linux-gnu.tar.xz && \
if [ ! -d "install" ]; then
    echo "Creating install directory"
    mkdir install
    
else
    echo "Install directory already exists, removing old files"
    rm -rf install/*
fi
echo "Extracting gcc-arm.tar.xz to install directory"
tar x -C install -f gcc-arm.tar.xz 
rm -r gcc-arm.tar.xz
echo "Setting up the cross compiler"
if [ ! -d "${OUTDIR}/install/arm-gnu-toolchain-$GCC_ARM_VERSION-x86_64-aarch64-none-linux-gnu" ]; then
    echo "Cross compiler directory not found, please check the extraction."
    exit 1
else
    echo "Cross compiler directory found, setting up environment variables."
    export PATH=${OUTDIR}/install/arm-gnu-toolchain-$GCC_ARM_VERSION-x86_64-aarch64-none-linux-gnu/bin:$PATH
    export CROSS_COMPILE=aarch64-none-linux-gnu-
    export ARCH=arm64
    echo "Cross compiler set to ${CROSS_COMPILE} for architecture ${ARCH}"
fi
SYSROOT=${OUTDIR}/install/arm-gnu-toolchain-${GCC_ARM_VERSION}-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc
cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi
echo "Configuring busybox"
# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j4
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
if [ ! -e ${OUTDIR}/busybox/busybox ]; then
    echo "Busybox build failed."
    exit 1
else
    echo "Busybox built successfully."
fi

cd "$OUTDIR/busybox"
echo "Copying busybox to the staging directory"
cp -r _install/bin/* ${OUTDIR}/rootfs/bin/
cp -r _install/sbin/* ${OUTDIR}/rootfs/sbin/
cp -r _install/usr/* ${OUTDIR}/rootfs/usr/

echo "Library dependencies"
${CROSS_COMPILE}readelf -a _install/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a _install/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "Adding library dependencies to rootfs"

mkdir -p ${OUTDIR}/rootfs/lib
mkdir -p ${OUTDIR}/rootfs/lib64
cp -L ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp -L ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/
cp -L ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
cp -L ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/
cp -L ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib/
cp -L ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib/
cp -L ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib/


# TODO: Make device nodes
echo "Creating device nodes"
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty1 c 4 1
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty2 c 4 2
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty3 c 4 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty4 c 4 4

# TODO: Clean and build the writer utility
cd "$FINDER_APP_DIR"
echo "Writer utility directory: ${FINDER_APP_DIR}"
if [ ! -f "${FINDER_APP_DIR}/writer.c" ]
then
    echo "Writer directory does not exist"
    exit 1
else
    echo "Cleaning and building writer utility"
    make clean
    make CROSS_COMPILE=${CROSS_COMPILE} 
    if [ ! -e writer.elf ]; then
        echo "Writer utility build failed."
        exit 1
    else
        echo "Writer utility built successfully."
    fi
fi


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
mkdir -p ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/finder.sh
cp ${FINDER_APP_DIR}/writer.elf ${OUTDIR}/rootfs/home/writer.elf
cp ${FINDER_APP_DIR}/writer.o ${OUTDIR}/rootfs/home/writer.o
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/finder-test.sh
cp ${FINDER_APP_DIR}/Makefile ${OUTDIR}/rootfs/home/Makefile
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/autorun-qemu.sh
mkdir -p ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf/assignment.txt
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf/username.txt
echo "Copying finder related scripts and executables to ${OUTDIR}/rootfs/home successfully."

# TODO: Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
echo "Creating initramfs.cpio.gz in ${OUTDIR}"
find . | cpio -H newc -o | gzip > ${OUTDIR}/initramfs.cpio.gz

if [ ! -e ${OUTDIR}/initramfs.cpio.gz ]; then
    echo "Failed to create initramfs.cpio.gz"
    exit 1
else
    echo "initramfs.cpio.gz created successfully."
fi

