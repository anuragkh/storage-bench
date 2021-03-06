sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

sys=$1
bin="/tmp/build"
conf="$sbin/../conf/storage_bench.conf"
bench="$sbin/../lambda_benchmark.py"
mode="scale:read:200:20:5"
size="1024"
nops="1000000"
dist="zipf"
log="/tmp/${sys}-${size}-${nops}-${dist}"

stdbuf -o0 python $bench --quiet --conf $conf --invoke-local --bin-path $bin\
  --system $sys --mode $mode --obj-size $size --num-ops $nops --dist $dist\
  1>${log}.stdout 2>${log}.stderr
