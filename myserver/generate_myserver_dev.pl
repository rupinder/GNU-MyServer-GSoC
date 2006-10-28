#!/usr/bin/perl
#Generate the Dev-C++ file for myserver.

use File::Find;

my @files = ('stdafx.cpp');
my @directories = ('source');

$head = "[Project]
FileName=myserver.dev
Name=myserver
UnitCount=\$COUNT
Type=1
Ver=1
ObjFiles=
Includes=
Libs=contrib/rx
PrivateResource=myserver_private.rc
ResourceIncludes=
MakeIncludes=
Compiler=
CppCompiler=
Linker=-lz.dll -llibxml2 -liconv -lintl -lssl -lcrypto -lrx -lgdi32 -lwininet -lwsock32 -luserenv -rdynamic_@@_
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
CustomMakefile=";

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
