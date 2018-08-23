sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

$sbin/hosts.sh $sbin/scale-bench.sh $@
