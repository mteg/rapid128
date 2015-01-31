<?
  
  $i = 0;
  while($l = fgets(STDIN))
  {
    $l = trim($l);
    $n = strlen($l);
    if($n == 11) 
      $codes[$i++] = $l;
  }
  
  if($i != 107)
    die("Code list must contain 107 codes (106 + end)\n");
  
  function gen_tree($a, $level)
  {
    $indent = "\n" . str_repeat(" ", $level);
    $res = $indent . "&(struct codetree_node) {";
//    $res = $indent . "{";
   
    if($level == 12)
    {
      if(count($a) != 1)
        die("Ambigious code at level 12, something went wrong!\n");
      $res .= key($a) . ", NULL, NULL}";
      return $res;
    }
    
    $res .= "-1, ";
    
    /* Split codes */
    $split = array("0" => array(), "1" => array());
    foreach($a as $k => $code)
      $split[substr($code, 0, 1)][$k] = substr($code, 1);
    
    foreach($split as $digit => $new_codes)
    {
      if(count($new_codes))
        $res .= gen_tree($new_codes, $level + 1);
      else
        $res .= $indent . " NULL";
        
      if($digit == "0")
        $res .= ",";
      else
        $res .= $indent . "}";
    }
    return $res;
  }
  
  echo '#include <string.h>' . "\n";
  echo '#include "read128.h"' . "\n";
  echo "struct codetree_node * code_tree = " . gen_tree($codes, 1) . ";\n";
  
  