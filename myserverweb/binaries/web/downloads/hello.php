<?PHP
	header("Content-Type: text/html");
	print("<html><title>Hello</title><p>\r\nHello world from a PHP page!\r\n</html>");
	$sn=getenv("SERVER_NAME"). ":" . getenv("SERVER_PORT") ;
	$protocol = getenv("SERVER_PROTOCOL");
	print("<P>\r\n<address>Connected: " . $sn  . " via " .$protocol  .  "</address>");	
	$qs=getenv("QUERY_STRING");
	if($qs)
	{
		print("<P>Query string: ". $qs);	
	}
?>
