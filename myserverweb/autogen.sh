#!/bin/sh

(aclocal --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need aclocal to compile MyServer";
    echo;
    exit;
}

(autoheader --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need autoheader to compile MyServer";
    echo;
    exit;
}

(automake --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need automake to compile MyServer";
    echo;
    exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 ||
{
    echo;
    echo "You will need autoconf to compile MyServer";
    echo;
    exit;
}

echo "Creating configuration files for MyServer. Please wait..."
echo;

aclocal -I m4
autoheader
automake -a
autoconf
