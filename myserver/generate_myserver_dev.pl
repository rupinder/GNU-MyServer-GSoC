#!/usr/bin/perl

## MyServer
##
## http://www.myserverproject.net
##
## Copyright (C) 2006, 2007, 2008 The MyServer Team
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Generate Dev-C++ project file for MyServer.

use File::Find;

my @files = ('stdafx.cpp');
my @directories = ('src');

$head = "[Project]
FileName=myserver.dev
Name=myserver
UnitCount=\$COUNT
Type=1
Ver=1
ObjFiles=
Includes=
Libs=
PrivateResource=myserver_private.rc
ResourceIncludes=
MakeIncludes=
Compiler=
CppCompiler=
Linker=-lz.dll -llibxml2 -liconv -lintl -lssl -lcrypto -lrx -lgdi32 -lwininet -lwsock32 -luserenv -levent -rdynamic_@@_
IsCpp=1
Icon=
ExeOutput=binaries
ObjectOutput=source
OverrideOutput=0
OverrideOutputName=myserver.exe
HostApplication=
Folders=
CommandLine=
IncludeVersionInfo=1
SupportXPThemes=0
CompilerSet=0
CompilerSettings=0000001000000101000000
UseCustomMakefile=0
CustomMakefile=

[VersionInfo]
Major=0
Minor=8
Release=12
Build=1
LanguageID=1033
CharsetID=1252
CompanyName=MyServer Project
FileVersion=0.8.12
FileDescription=MyServer webserver
InternalName=
LegalCopyright=MyServer Team
LegalTrademarks=
OriginalFilename=myserver.exe
ProductName=MyServer
ProductVersion=0.8.12
AutoIncBuildNr=0";

$unit_options = "CompileCpp=1
Folder=myserver
Compile=1
Link=1
Priority=1000
OverrideBuildCmd=0
BuildCmd=";

my @result = ();

for $file (@files)
{
    add($file);
}

find(\&check, @directories);

sub check()
{
    if($File::Find::name=~/.*cpp$/)
    {
	$File::Find::name =~ s/\//\\/gi;
	add($File::Find::name);
    }
} 

sub add
{
    push(@result, "$_[0]");
}

$count = @result;

$head =~ s/\$count/$count/ig;
print("$head\n");

$counter = 1;

foreach $file (@result)
{
    print ("[Unit$counter]\nFileName=$file\n$unit_options\n\n");
    $counter++;
}
