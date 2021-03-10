#!/bin/bash
## Convert a rpm file in to a tar.gz
## Similar to `rpm2archive`
##

usage() {
    echo "Usage: $0 <rpmfile>" >&2
    exit 1
}

function cleanup {
    rm -rf "$work_dir"
}

if [[ ! -f $1 ]]; then
    echo "$1 does not exist"
    exit 1
fi

rpmfile_abs=`readlink -f $1`
rpmfile=$(basename $1)
tarfile=${rpmfile%.*}.tar.gz

work_dir=`mktemp -d -t ${rpmfile}_XXX`

if [[ ! "$work_dir" || ! -d "$work_dir" ]]; then
    echo "Failed to create temp directory"
    exit 1
fi

pushd $work_dir
rpm2cpio ${rpmfile_abs} | cpio -idmv
popd
tar -C $(dirname $work_dir) -czf ${tarfile} $(basename $work_dir)
