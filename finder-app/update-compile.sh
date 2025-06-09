OUTDIR=/tmp/aeld

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

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
