system=$1
python lambda_benchmark.py --conf conf/storage_bench.conf --invoke-local --bin-path /tmp/build --system $system --bench-mode scale:read:200:20:5 --object-size 1024 --num-ops 1000000 --distribution zipf
