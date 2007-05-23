#!/usr/bin/perl
#  CREATE THE SKELETON STRUCTURE FOR A PLUGIN SPECIFYING A NAMESPACE AND THE PLUGIN NAME.

use Cwd;
use File::Copy;

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

if(!(-d $namespace_dir) || !(-d $plugin_dir))
{
		mkdir($plugin_dir, 0755);
}

print "Creating Makefile\n";

if(-f "$plugin_dir/Makefile")
{
		$update_makefile = 1;
		open(OLD_MF, "<$plugin_dir/Makefile");
		do
		{
				$line =	<OLD_MF>;
		}while(!($line =~ /END_OF_CONFIGURATION/ ));
}



open(MF, ">$plugin_dir/Makefile.new");

print MF "# BEGIN_OF_CONFIGURATION #\n";
print MF "# DO NOT WRITE IN THIS SECTION OF THE MAKEFILE! #\n# Text here is handled by the create_skeleton.pl script. #\n";
print MF "# Text after this section is not updated automatically. #\n";


while (($key, $value) = each(%data))
{
		print MF "$key=$value\n";
}

print MF "\n# END_OF_CONFIGURATION #\n";



if($update_makefile)
{
		while(<OLD_MF>)
		{
				print MF $_;
		}

		close(OLD_MF);
}
else
{
		if(-f "$plugin_dir/Makefile.custom")
		{
				open(CUSTOM, "<$plugin_dir/Makefile.custom");
				while(<CUSTOM>) 
				{
						print MF $_;
				}
				close(CUSTOM);
				
		}
		else
		{
				print MF "\n\nall:\n";
				print MF "\t\$(CXX) -c $plugin.cpp -o $plugin.o -I\$(MYSERVER_HEADERS) -I\$(MSCGI_HEADERS) -I\$(MYSERVER_HEADERS)/.. \$(CFLAGS) \$(CXXFLAGS) \$(CPPFLAGS)\n";
				print MF "\t\$(CXX) $plugin.o -o $plugin.so -rdynamic \$(XML_LIBS) \$(LDFLAGS) -shared\n";

				print MF "\n\ninstall:\n";
				print MF "\tcp $plugin.so ..\n";
		}
}

close(MF);

unlink("$plugin_dir/Makefile");
move("$plugin_dir/Makefile.new", "$plugin_dir/Makefile");

if(! $update_makefile)
{
		print "Creating $plugin.cpp\n";
		if(-f "$plugin_dir/$plugin.cpp")
		{
				die("$plugin_dir/$plugin.cpp exists!\n");
		}

		open(SRC, ">$plugin_dir/$plugin.cpp");
		
		print SRC "#include <stdafx.h>\n";
		print SRC "#include <string.h>\n";

		print SRC "#ifdef WIN32\n";
		print SRC "#define EXPORTABLE(x) x _declspec(dllexport)\n";
		print SRC "#else\n";
		print SRC "#define EXPORTABLE(x) extern \"C\" x\n";
		print SRC "#endif\n\n";

		
		print SRC "EXPORTABLE(char*) name(char* name, u_long len)\n";
		print SRC "{\n\tchar* str = \"$plugin\";\n\tif(name)\n\t\t";
		print SRC "strncpy(name, str, len);\n\treturn str;\n}\n";

		print SRC "EXPORTABLE(int) load(void* server,void* parser)\n{\n\treturn 0;\n}\n";

		print SRC "EXPORTABLE(int) postLoad(void* server,void* parser)\n{\n\treturn 0;\n}\n";

		print SRC "EXPORTABLE(int) unLoad(void* parser)\n{\n\treturn 0;\n}\n";


		close(SRC);
}
