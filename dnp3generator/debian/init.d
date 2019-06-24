#!/bin/bash
### BEGIN INIT INFO
# Provides:          dnp3Generator
# Required-Start:    $network $local_fs
# Required-Stop:     $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# X-Interactive:     false
# Short-Description: DNP3Generator Service
# Description:       Start/stop Dnp3Generator
### END INIT INFO
DESC="Dnp3 Traffic Generator"
NAME=dnp3Generator
PIPELOC="/var/run/dnp3pipe"
DNP3PIPE="-p $PIPELOC"
DAEMON=/usr/bin/dnp3Generator
. /lib/lsb/init-functions

do_start()
{
		start-stop-daemon --start --chdir /etc/dnp3/ --background --quiet --name $NAME --exec $DAEMON -- $DNP3PIPE
		echo "Starting Dnp3Generator";
}

do_stop()
{
   start-stop-daemon --stop --quiet --signal QUIT --name $NAME 
      echo "Stopping Dnp3Generator";
   rm -rf $PIPELOC
}


case "$1" in
   start)
     do_start
     ;;
   stop)
     do_stop
     ;;
   restart)
   do_stop
   do_start
     ;;
   force-reload)
   do_stop
   do_start
     ;;
esac

exit 0
