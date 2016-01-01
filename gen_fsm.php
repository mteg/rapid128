<?
  $state_max = 0;
  $st_tab = array();
  while($l = fgets(STDIN))
  {
    list($state, $zero, $one, $junk) = explode("\t", strtr(trim($l), array("," => "\t")), 4);
    $state = trim($state);
    $zero = trim($zero);
    $one = trim($one);
    
    if(!is_numeric($state)) continue;
    if(!is_numeric($zero)) continue;
    if(!is_numeric($one)) continue;
    
    $T[$state] = array($zero, $one);
    $state_max = max($state, $state_max);
  }
  
  echo "const static u_int8_t fsm_tab[] = {";
  for($i = 0; $i<= $state_max; $i++)
  {
     if($i) echo ", ";
     if(!isset($T[$i][0]))
       echo "0, ";
     else
       echo ($T[$i][0] * 2) . ", ";

     if(!isset($T[$i][1]))
       echo "0";
     else
       echo $T[$i][1] * 2;
  }
  echo "};\n";

