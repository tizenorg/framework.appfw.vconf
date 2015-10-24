#!/bin/sh

CURRENTTIME=`date +%m%d-%T.%3N`
HISTORY="/opt/var/kdb/.restore/.history/history"
LOCK="/opt/var/kdb/.restore/.history/lock"

echo "vconf restore key sh start ($CURRENTTIME)" >> $HISTORY 

while [ -f $LOCK ]
do
    echo "vconf restore key sh waiting ($$) ..." >> $HISTORY
    sleep 0.2
done

touch $LOCK

while :; do
is_file=0

for file in `find /opt/var/kdb/.restore -maxdepth 1 -type f`
do
    key=`cat $file`
    d_key=${key#/}

    # platform does not support backend restoring routine.
    echo "need to restore $key" >> $HISTORY
    /bin/rm $file
done

if [ "$is_file" == 0 ]; then
    break;
fi

done

/bin/rm $LOCK

CURRENTTIME=`date +%m%d-%T.%3N`

echo "vconf restore key sh end ($CURRENTTIME)" >> $HISTORY
echo "===============================" >> $HISTORY
