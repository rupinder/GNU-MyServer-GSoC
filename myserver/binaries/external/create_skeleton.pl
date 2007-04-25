#!/usr/bin/perl
#  CREATE THE SKELETON STRUCTURE FOR A PLUGIN SPECIFYING A NAMESPACE AND THE PLUGIN NAME.

use Cwd;
my %data;
open file, "<", "modules.template";

while(<file>) 
{
		my($key, $value) = /^(.+)*=(.+)*$/
				or die "Syntax error: $_";
		$data{$key} = $value;
}

if($#ARGV < 0)
{
		print "Namespace?\n";
		$namespace = <STDIN>;
		$namespace =~ s/\n//g;
}
else
{
		$namespace = $ARGV[0];
}

if($#ARGV < 1)
{
		print "Plugin name?\n";
		$plugin = <STDIN>;
		$plugin =~ s/\n//g;
}
else
{
		$plugin = $ARGV[1];
}

print "Creating $namespace" .  "::" . "$plugin\n";

$namespace_dir = getcwd() . "/"  . $namespace;
$plugin_dir = $namespace_dir . "/" . $plugin;
if((-d $namespace_dir) && (-d $plugin_dir))
{
		die "The plugin already exists\n";
}
else
{
		mkdir($plugin_dir, 0755);
}

print "Creating Makefile\n";

if(-f "$plugin_dir/Makefile")
{
		die ("$plugin_dir/Makefile exists!\n");
}

open(MF, ">$plugin_dir/Makefile");


while (($key, $value) = each(%data))
{
		print MF "$key=$value\n";
}

print MF "\n\nall:\n";
print MF "\t\$(CXX) -c $plugin.cpp -o $plugin.o -I\$(MYSERVER_HEADERS) -I\$(MSCGI_HEADERS) -I\$(MYSERVER_HEADERS)/.. \$(CFLAGS) \$(CXXFLAGS)\n";
print MF "\t\$(CXX) $plugin.o -o $plugin.so -rdynamic \$(XML_LIBS) \$(LDFLAGS) -shared\n";

print MF "\n\ninstall:\n";
print MF "\tcp $plugin.so ..\n";


close(MF);


print "Creating $plugin.cpp\n";

if(-f "$plugin_dir/$plugin.cpp")
{
		die ("$plugin_dir/$plugin.cpp exists!\n");
}

open(SRC, ">$plugin_dir/$plugin.cpp");

print SRC "#include <stdafx.h>\n";
print SRC "#include <string.h>\n";

print SRC "extern \"C\"\n";
print SRC "char* name(char* name, u_long len)\n{\n\tchar* str = \"$plugin\";\n\tif(name)\n\t\tstrncpy(name, str, len);\n\treturn str;\n}\n";


close(SRC);
