<?PHP
	print("Hello world from a PHP page!");
	$sn=getenv("SERVER_NAME");
	print("<P><address>Running on:" . $sn  . "</address>");	
	$qs=getenv("QUERY_STRING");
	if($qs)
	{
		print("<P>Query string: ". $qs);	
	}
?>
