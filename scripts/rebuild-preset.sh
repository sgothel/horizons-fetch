#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
bname=`basename $0 .sh`

. $rootdir/jaulib/scripts/setup-machine-arch.sh

tripleid="$os_name-$archabi"

if [ ! -z "$1" ] ; then
    preset_name=$1
    shift 1
else
    echo "ERROR: No preset passed as 1st argument, use one of:"
    cmake --list-presets
    return 1
fi

logfile=$rootdir/${bname}-${preset_name}-${tripleid}.log
rm -f $logfile

CPU_COUNT=`getconf _NPROCESSORS_ONLN`

# run 'dpkg-reconfigure locales' enable 'en_US.UTF-8'
export LANG=en_US.UTF-8
export LC_MEASUREMENT=en_US.UTF-8

buildit() {
    echo rootdir $rootdir
    echo logfile $logfile
    echo CPU_COUNT $CPU_COUNT

    dist_dir="dist/${preset_name}-${tripleid}"
    build_dir="build/preset_name}"
    echo dist_dir $dist_dir
    echo build_dir $build_dir

    if [ -x /usr/bin/time ] ; then
        time_cmd="time"
        echo "time command available: ${time_cmd}"
    else 
        time_cmd=""
        echo "time command not available"
    fi

    cd $rootdir

    ${time_cmd} cmake --build --preset ${preset_name} --target install --parallel
    if [ $? -eq 0 ] ; then
        echo "REBUILD SUCCESS $bname, preset $preset_name, $tripleid"
        return 0
    else
        echo "REBUILD FAILURE $bname, preset $preset_name, $tripleid"
        return 1
    fi
}

buildit 2>&1 | tee $logfile

