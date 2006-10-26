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
<title>MyServer - Hello World PHP Example</title>
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
<big>MyServer - Hello World PHP Example</big>
<br /><br />
<a href="http://www.myserverproject.net"><img src="/icons/logo.png" alt="" style="border: 0px;" /></a>
<div class="right">
<small>
<a href="?showsource">Show Source</a>
</small>
</div>
<table width="100%" class="center">
<tr>
<td>');

print('Hello world from a PHP page!');
print ('<br /><br />'.getenv("SERVER_SOFTWARE").' on '.getenv("SERVER_NAME").' Port '.getenv("SERVER_PORT")."\n");
print ('<br />Connected via <span class="italic">'.getenv("SERVER_PROTOCOL").'</span>');
if(getenv("QUERY_STRING"))
{
	print('<br /><br />Query string: '.getenv("QUERY_STRING"));	
}
print("\r\n");

print ('</td></tr>
</table>
</div>
</body>
</html>');
}
?>