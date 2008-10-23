#!/bin/sh
#Create a clean binaries package.

if [  $# = 0 ]; then
  echo "Usage: $0 dir_to_create [tar.bz2]"
  exit
fi

mkdir $1
cp myserver $1
cp myserver-daemon $1
cp *.default $1
cp readme.txt $1
cp ../COPYING $1
cp ../INSTALL $1
cp ../NEWS $1
cp ../README $1
cp ../TODO $1

mkdir $1/web
cp web/*.html $1/web
cp web/*.png $1/web

mkdir $1/certificates
cp certificates/*.txt $1/certificates

mkdir $1/system
cp system/security $1/system

mkdir $1/system/css
cp system/css/*.css $1/system

mkdir $1/system/errors
cp system/errors/*.html $1/system/errors

mkdir $1/system/icons

mkdir $1/system/icons/codes
cp system/icons/codes/*.png $1/system/icons/codes

mkdir $1/languages
cp languages/*.xml $1/languages

mkdir $1/languages/configure
cp languages/configure/*.xml $1/languages/configure

mkdir $1/web/cgi-bin
cp web/cgi-bin/*.html $1/web/cgi-bin
cp web/cgi-bin/*.readme $1/web/cgi-bin
cp web/cgi-bin/*.mscgi $1/web/cgi-bin

mkdir $1/logs
touch $1/logs/.keep_me

mkdir $1/web/downloads
cp web/downloads/*.txt $1/web/downloads
cp web/downloads/*.php $1/web/downloads
cp web/downloads/*.sh $1/web/downloads

mkdir $1/web/documentation
cp ../documentation/myserver/*.html $1/web/documentation/

if [  $# = 2 ]; then
    tar --create --bzip2 --file=$2.tar.bz2 $1
    rm -rf $1
fi
