<?
  
  $i = 0;
  while($l = fgets(STDIN))
  {
    $l = trim($l);
    $n = strlen($l);
    if($n == 11) 
      $codes[$l] = $i++;
  }
  
  if($i != 107)
    die("Code list must contain 107 codes (106 + end)\n");
    
  echo "char code_symbols[] = {\n";
  for($i = 0; $i<512; $i++)
  {
    $c = "1" . str_pad(decbin($i), 9, "0", STR_PAD_LEFT) . "0";
    if(isset($codes[$c]))
      echo $codes[$c] . ", /* " . $c . "*/\n";
    else
      echo "255, /* No code */\n";
  }
  echo "};\n";
  
  