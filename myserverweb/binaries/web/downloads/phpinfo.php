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
<title>MyServer - phpinfo() PHP Example</title>
<meta http-equiv="content-type" content="text/html;charset=UTF-8" />
<style type="text/css">
body {background-color: #ffffff; color: #666699;}
body, td, th, h1, h2 {font-family: verdana,arial,helvetica,tahoma,sans-serif;}
pre {margin: 0px; font-family: monospace;}
a {color: #666699;}
a:link {color: #666699; text-decoration: none; background-color: #ffffff;}
a:hover {text-decoration: underline;}
table {border-collapse: collapse;}
.center {text-align: center;}
.center table {margin-left: auto; margin-right: auto; text-align: left;}
.center th {text-align: center !important;}
.right {text-align: right;}
td, th { border: 1px solid #666699; font-size: 75%; vertical-align: baseline;}
h1 {font-size: 150%;}
h2 {font-size: 125%;}
.p {text-align: left;}
.e {background-color: #666699; font-weight: bold; color: #ffffff;}
.h {background-color: #666699; font-weight: bold; color: #ffffff;}
.v {background-color: #ffffff; color: #666699;}
i {color: #ffffff; background-color: #666699;}
img {float: right; border: 0px;}
hr {width: 600px; background-color: #666699; border: 0px; height: 1px; color: #666699;}
</style>
<div class="center">');
ob_start();
phpinfo();
$php_info .= ob_get_contents();
ob_end_clean();
$php_info    = str_replace(" width=\"600\"", " width=\"600\"", $php_info);
$php_info    = str_replace("</body></html>", "", $php_info);
$offset      = strpos($php_info, "<table");
print('<div class="right">
<small>
<a href="?showsource" style="text-decoration: none;">Show Source</a>
</small>
</div>');
print substr($php_info, $offset);
print('
</div>
</body>
</html>');
}
?>