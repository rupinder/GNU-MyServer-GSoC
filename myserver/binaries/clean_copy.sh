#!/bin/sh
#Create a clean binaries package.

if [  $# = 0 ]; then
  echo "Usage: $0 dir_to_create [tar.bz2]"
  exit
fi

mkdir $1
cp -v myserver $1
cp -v myserver-daemon $1
cp -v *.default $1
cp -v ../COPYING $1
cp -v ../INSTALL $1
cp -v ../NEWS $1
cp -v ../README $1
cp -v ../TODO $1

mkdir $1/web
cp -v web/*.html $1/web
cp -v web/*.png $1/web

mkdir $1/certificates
cp -v certificates/*.txt $1/certificates

mkdir $1/system
cp -v system/security $1/system

mkdir $1/system/css
cp -v system/css/*.css $1/system

mkdir $1/system/errors
cp -v system/errors/*.html $1/system/errors

mkdir $1/system/icons

mkdir $1/system/icons/codes
cp -v system/icons/codes/*.png $1/system/icons/codes

mkdir $1/languages
cp -v languages/*.xml $1/languages

mkdir $1/languages/configure
cp -v languages/configure/*.xml $1/languages/configure

mkdir $1/web/cgi-bin
cp -v web/cgi-bin/*.html $1/web/cgi-bin
cp -v web/cgi-bin/*.readme $1/web/cgi-bin
cp -v web/cgi-bin/*.mscgi $1/web/cgi-bin

mkdir $1/logs
touch $1/logs/.keep_me

mkdir $1/web/downloads
cp -v web/downloads/*.txt $1/web/downloads
cp -v web/downloads/*.php $1/web/downloads
cp -v web/downloads/*.sh $1/web/downloads

mkdir $1/web/documentation
cp -v ../documentation/myserver.html/* $1/web/documentation/

if [  $# = 2 ]; then
    tar --create --bzip2 --file=$2.tar.bz2 $1
    rm -rf $1
fi
