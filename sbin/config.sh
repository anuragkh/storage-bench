# resolve links - $0 may be a softlink
this="${BASH_SOURCE:-$0}"
common_bin="$(cd -P -- "$(dirname -- "$this")" && pwd -P)"
script="$(basename -- "$this")"
this="$common_bin/$script"

# convert relative path to absolute path
config_bin="`dirname "$this"`"
script="`basename "$this"`"
config_bin="`cd "$config_bin"; pwd`"
this="$config_bin/$script"

export BENCH_PREFIX="`dirname "$this"`"/..
export BENCH_HOME="${BENCH_PREFIX}"
export BENCH_CONF_DIR="${BENCH_CONF_DIR:-"$BENCH_HOME/conf"}"
