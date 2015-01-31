<?
  $tab = array();
  while($l = fgets(STDIN))
    $tab[] = trim($l);
  
  if(count($tab) != (106 * 3))
    die("Code tables must contain 106 codes x 3\n");
  
  
  function gen_table($a)
  {
    $res = array();
    foreach($a as $n => $symbol)
    {
      if($symbol == '"') $symbol = "\\\"";
      if($symbol == "\\") $symbol = "\\\\";
      if($symbol{0} == "-" && strlen($symbol) > 1) $symbol = " " . $symbol . " ";
      $res[]= "  \"$symbol\" /* {$n} */";
    }
    $res = "{" . implode(",\n", $res) . "\n}";
    return $res;
  }
  
  echo '#include <string.h>' . "\n";
  echo '#include "read128.h"' . "\n";
  echo "char * code_tableA[] = " .  gen_table(array_slice($tab, 106 * 0, 106)) . ";\n";
  echo "char * code_tableB[] = " .  gen_table(array_slice($tab, 106 * 1, 106)) . ";\n";
  echo "char * code_tableC[] = " .  gen_table(array_slice($tab, 106 * 2, 106)) . ";\n";
  echo "char ** code_tables[] = {code_tableA, code_tableB, code_tableC};\n";
  
  