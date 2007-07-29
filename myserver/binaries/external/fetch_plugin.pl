#!/usr/bin/perl
# FETCH A PLUGIN, COMPILE AND DEPLOY IT.
use XML::Simple;

$method = "svn";
$home = "http://myserver.svn.sourceforge.net/svnroot/myserver/trunk/plugins";

@fetched;

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

sub fetch_plugin
{
		local $namespace = @_[0];
		local $plugin = @_[1];

		print "Fetching $namespace\:\:$plugin\n";

		if(-d "$namespace/$plugin")
		{
				print "Plugin $namespace\:\:$plugin exists\n";
		}
		else
		{
				if($method == "svn")
				{
						system("svn co $home/$namespace/$plugin $namespace/$plugin");
				}
		}


		my $xml = new XML::Simple;

		my $data = $xml->XMLin("$namespace/$plugin/plugin.xml");

		my @depends = $data->{DEPENDS};

    #FETCH EVERY DEPENDS
		foreach(@depends)
		{
				if($_)
				{
						($a, $b) = /([a-z_0-9]*)::([a-z_0-9]*)/ig;
						print("$namespace\:\:$plugin needs $a\:\:$b\n");
						fetch_plugin($a, $b);
				}
		}

		push(@fetched, "$namespace::$plugin");

		system("./create_skeleton.pl $namespace $plugin");
		system("cd $namespace/$plugin; make; make install");

}

fetch_plugin($namespace, $plugin);

print "\n";

foreach(@fetched)
{
		print "Fetched $_\n";
}
