#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo, Jack Thomson

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
CROSS_COMPILE_PATH="$(which aarch64-none-linux-gnu-gcc)"
CROSS_COMPILE_LOCATION="$(dirname $CROSS_COMPILE_PATH)/../"
BASE_DIR=$(dirname $(realpath -s $0))

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
    # Removes the extra yylloc line
    sed --in-place --expression="41d" $OUTDIR/linux-stable/scripts/dtc/dtc-lexer.l
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
fi

echo "Adding the Image in outdir"
ln -sf ${OUTDIR}/linux-stable/arch/arm64/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create neccessary base dirs
mkdir -p "${OUTDIR}/rootfs/bin"
mkdir -p "${OUTDIR}/rootfs/dev"
mkdir -p "${OUTDIR}/rootfs/etc"
mkdir -p "${OUTDIR}/rootfs/home"
mkdir -p "${OUTDIR}/rootfs/lib"
mkdir -p "${OUTDIR}/rootfs/lib64"
mkdir -p "${OUTDIR}/rootfs/proc"
mkdir -p "${OUTDIR}/rootfs/sys"
mkdir -p "${OUTDIR}/rootfs/sbin"
mkdir -p "${OUTDIR}/rootfs/tmp"
mkdir -p "${OUTDIR}/rootfs/usr"
mkdir -p "${OUTDIR}/rootfs/var/log"
mkdir -p "${OUTDIR}/rootfs/usr/bin"
mkdir -p "${OUTDIR}/rootfs/usr/sbin"
mkdir -p "${OUTDIR}/rootfs/root"


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
else
    cd busybox
fi

make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# Add program intepreter to lib folder
cp $CROSS_COMPILE_LOCATION/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/

# Add library dependencies to rootfs
cp $CROSS_COMPILE_LOCATION/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
cp $CROSS_COMPILE_LOCATION/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/
cp $CROSS_COMPILE_LOCATION/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/

# Make device nodes

sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 5
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1

# Clean and build the writer utility
cd $BASE_DIR
make clean
make CROSS_COMPILE=$CROSS_COMPILE

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cp -r *.sh $OUTDIR/rootfs/home/
cp writer $OUTDIR/rootfs/home/
cp -r ../conf $OUTDIR/rootfs/home/

# Chown the root directory
sudo chown -R $UID $OUTDIR/rootfs

# Create initramfs.cpio.gz

cd "$OUTDIR/rootfs"

find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
