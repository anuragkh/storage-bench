sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

sys=$1
bin="/tmp/build"
conf="$sbin/../conf/storage_bench.conf"
bench="$sbin/../lambda_benchmark.py"
mode="write_async{1000}"
size="1024"
nops="1000000"
log="/tmp/${sys}-${size}-${nops}"

python $bench --conf $conf --invoke-local --bin-path $bin --system $sys\
  --mode $mode --obj-size $size --num-ops $nops 1>${log}.stdout 2>${log}.stderr
