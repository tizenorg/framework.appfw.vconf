#!/bin/sh

VCONF_DUMP=$1/vconf

/bin/mkdir -p $VCONF_DUMP

/bin/cp -rf --preserve=all /var/run/memory/ $VCONF_DUMP
/bin/cp -rf --preserve=all /var/kdb/db/ $VCONF_DUMP
/bin/cp -rf --preserve=all /var/kdb/file/ $VCONF_DUMP
/bin/cp -rf --preserve=all /opt/var/kdb/.restore/ $VCONF_DUMP

vconftool get db/ -r >> $VCONF_DUMP/vconf_value
vconftool get file/ -r >> $VCONF_DUMP/vconf_value
vconftool get memory/ -r >> $VCONF_DUMP/vconf_value

