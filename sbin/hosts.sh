# Run a shell command on all servers.
#
# Environment Variables
#
#   BENCH_SERVERS    File naming remote servers.
#     Default is ${BENCH_CONF_DIR}/servers.
#   BENCH_CONF_DIR  Alternate conf dir. Default is ${BENCH_HOME}/conf.
#   BENCH_HOST_SLEEP Seconds to sleep between spawning remote commands.
#   BENCH_SSH_OPTS Options passed to ssh when running remote commands.
##

usage="Usage: servers.sh [--config <conf-dir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/config.sh"

# If the servers file is specified in the command line,
# then it takes precedence over the definition in
# bench-env.sh. Save it here.
if [ -f "$BENCH_SERVERS" ]; then
  SERVERLIST=`cat "$BENCH_SERVERS"`
fi

# Check if --config is passed as an argument. It is an optional parameter.
# Exit if the argument is not a directory.
if [ "$1" == "--config" ]
then
  shift
  conf_dir="$1"
  if [ ! -d "$conf_dir" ]
  then
    echo "ERROR : $conf_dir is not a directory"
    echo $usage
    exit 1
  else
    export BENCH_CONF_DIR="$conf_dir"
  fi
  shift
fi

. "$BENCH_PREFIX/sbin/load-env.sh"

if [ "$SERVERLIST" = "" ]; then
  if [ "$BENCH_SERVERS" = "" ]; then
    if [ -f "${BENCH_CONF_DIR}/hosts" ]; then
      SERVERLIST=`cat "${BENCH_CONF_DIR}/hosts"`
    else
      SERVERLIST=localhost
    fi
  else
    SERVERLIST=`cat "${BENCH_SERVERS}"`
  fi
fi



# By default disable strict host key checking
if [ "$BENCH_SSH_OPTS" = "" ]; then
  BENCH_SSH_OPTS="-o StrictHostKeyChecking=no"
fi

for host in `echo "$SERVERLIST"|sed  "s/#.*$//;/^$/d"`; do
  if [ -n "${BENCH_SSH_FOREGROUND}" ]; then
    ssh $BENCH_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /"
  else
    ssh $BENCH_SSH_OPTS "$host" $"${@// /\\ }" \
      2>&1 | sed "s/^/$host: /" &
  fi
  if [ "$BENCH_HOST_SLEEP" != "" ]; then
    sleep $BENCH_HOST_SLEEP
  fi
done

wait
