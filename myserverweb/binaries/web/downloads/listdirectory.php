<?php
if(isset($_GET['showsource']))
{
	show_source(__FILE__);
}
else
{
print ('<?xml version="1.0" encoding="UTF-8" ?>');
print ("\n".'<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"'."\n".'"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">'."\n");
print ('<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head>
<title>MyServer PHP Directory Listing Example</title>
<meta http-equiv="content-type" content="text/html;charset=UTF-8" />
<style type="text/css">
body {font-family: verdana, arial, helvetica, tahoma, sans-serif;}
a {color: #666699; text-decoration: none;}
table{border: 1px solid #666699;}
.center {text-align: center;}
.right {text-align: right;}
.italic {font-style: italic;}
.bold {font-weight: bold;}
</style>
</head>
<body style="color: #666699;">
<div class="center">
<big>MyServer PHP Directory Listing Example</big>
<br /><br />
<a href="http://www.myserverproject.net"><img src="/icons/logo.png" alt="" style="border: 0px;" /></a>
<br /><br />
Contents of <span class="bold">downloads</span><br />
<div class="right">
<small>
<a href="?showsource">Show Source</a>
</small>
</div>
<table width="100%" class="center">
<tr class="bold">
<td>type</td>
<td>name</td>
<td>read</td>
<td>write</td>
<td>exec</td>
<td><a href="http://www.php.net/fileatime">fileatime()</a></td>
<td><a href="http://www.php.net/filemtime">filemtime()</a></td>
<td><a href="http://www.php.net/filectime">fliectime()</a></td>
</tr>');

$directory="./";
$dh=opendir($directory);
while(gettype($file=readdir($dh))!=boolean)
{
	print "<tr>";

	if(is_dir("$directory/$file"))
		print "<td>[dir]</td>";
	else print "<td></td>";

	
	print "<td><a href=\"$file\">$file</a></td>";

	if(is_readable($file))		print "<td>r</td>";	else print "<td></td>";
	if(is_writable($file))		print "<td>w</td>";	else print "<td></td>";
	if(is_executable($file))	print "<td>x</td>";	else print "<td></td>";

	print "<td>".date("D d M Y g:i A T",fileatime($file))."</td>";
	print "<td>".date("D d M Y g:i A T",filemtime($file))."</td>";
	print "<td>".date("D d M Y g:i A T",filectime($file))."</td>";
	print "</tr>";
}
closedir($dh);

print ('</table>
<div class="right">
<small>');

$myserver=getenv("SERVER_SIGNATURE");
preg_replace('@<address>.*?</address>@si','',$myserver);

print ($myserver.'</small>
</div>'."\n");

	print ('<br />'.getenv("SERVER_SOFTWARE").' on '.getenv("SERVER_NAME").' Port '.getenv("SERVER_PORT")."\n");
	print ('<br />Connected via <span class="italic">'.getenv("SERVER_PROTOCOL").'</span>');
print ('
</div>
</body>
</html>');
}
?>