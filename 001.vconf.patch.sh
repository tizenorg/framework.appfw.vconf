#!/bin/sh

# Make lock file for fota from old one binary
if [ ! -e /opt/var/kdb/.vconf_lock ]; then
    DB_KEY_LIST=`find /opt/var/kdb/db/ -type f; find /opt/var/kdb/file -type f; find /opt/var/kdb/memory_init -type f`

    for i in $DB_KEY_LIST;do
        P=`dirname $i`
        F=`basename $i`
        L=`echo $P"/."$F".lck"`
        touch $L
        chsmack -a 'system::vconf' $L
        chmod 666 $L
        chgrp 5000 $L
   done

   touch /opt/var/kdb/.vconf_lock
fi

