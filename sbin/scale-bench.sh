sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

sys=$1
bin="/tmp/build"
conf="$sbin/../conf/storage_bench.conf"
bench="$sbin/../lambda_benchmark.py"
mode="scale:read:40:20:5"
size="1024"
nops="1000000"
dist="zipf"
log="/tmp/${sys}-${size}-${nops}-${dist}"

python $bench --conf $conf --invoke-local --bin-path $bin --system $sys\
  --bench-mode $mode --object-size $size --num-ops $nops --distribution $dist\
  1>${log}.stdout 2>${log}.stderr
