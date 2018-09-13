sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

stats() {
  sort -n $1 | awk '
    BEGIN {
      c = 0;
      sum = 0;
    }
    $3 ~ /^[0-9]*(\.[0-9]*)?$/ {
      a[c++] = $3;
      sum += $3;
    }
    END {
      ave = sum / c;
      if( (c % 2) == 1 ) {
        median = a[ int(c/2) ];
      } else {
        median = ( a[c/2] + a[c/2-1] ) / 2;
      }
      OFS="\t";
      print ave, median, a[0], a[c-1];
    }
  '
}

sys=${1:-"redis"}
num_listeners=${2:-"1"}
bin="/tmp/build"
conf="$sbin/../conf/storage_bench.conf"
bench="$sbin/../lambda_benchmark.py"
mode="create_destroy"
size="1024"
nops="1000000"
log="/tmp/${sys}-${size}-${nops}"

python $bench --bench-type notification_bench --conf $conf --invoke-local --bin-path $bin --system $sys\
  --mode $mode --obj-size $size --num-ops $nops --num-listeners $num_listeners 1>${log}.stdout 2>${log}.stderr

for i in `seq 0 $((num_listeners - 1))`; do
  stats /tmp/${sys}_0_${size}_$((i + 1))Of${num_listeners}.txt >> ${sys}_${num_listeners}.txt
done
