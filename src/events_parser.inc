<?php // $Id: events_parser.inc 18 2006-07-22 23:58:09Z mouke $

  /**
   * Parse a string with this structure :
   *
   * (H:m|*:m|* / n:m|x+* / n:m) ((SONG|ARTIST|PLAYLIST) [N]|CRITERIA [ccc] VALUE [vvv])
   * (ROTATE|CYCLE) [min:xx max:yy (SONG|MINUTE)] ((SONG|ARTIST|PLAYLIST) [N]|CRITERIA [ccc] VALUE [vvv])
   *
   */
function D_Parse_Events ($events_string)
{
    if (preg_match ('/^\s*$/', $events_string))
    {
        return false;
    }
        
    global $LOG, $MSGS;
    
    $item = '((SONG|ARTIST|PLAYLIST)\s+\[([0-9]+)\]|CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+\[([A-Za-z0-9_]+)\])';
    $regexp_time = '/^\s*((([0-9]+)|\*|\*\/([0-9]+)|\*\/([0-9]+)\+([0-9]+))\s+([0-9]+))\s+:\s+' . $item . '(\s*|\s+#.*)$/';
    $regexp_cycle = '/\s*ROTATE\s+\[([0-9]+)\-([0-9]+)\s+(SONG|MINUTE)\]\s+' . $item . '(\s*|\s+#.*)$/';

    $events = explode ("\n", $events_string);

    $tokens = array ();

    $events_bag = array ('pile' => array (), 'rotated' => array ());
    
    foreach ($events as $event)
    {
        if (preg_match ('/^(\s*|\s*#.*)$/', $event))
        {
            continue;
        }
        if (preg_match ($regexp_time, $event, $tokens))
        {
            $e = array ();
            if ($tokens[9])
            {
                $e['type']  = $tokens[9];
                $e['value'] = $tokens[10];
            }
            else
            {
                $e['type']     = 'CRITERIA';
                $e['criteria'] = $tokens[11];
                $e['value']    = $tokens[12];
            }
            
            $minute = $tokens[7];
            
            if ($tokens[3] || $tokens[3] == '0' || $tokens[3] == '00') // a given hour
            {
                $start = $tokens[3];
                $inc   = 24;
            }
            else if ($tokens[2] == '*') // all hours : *
            {
                $start = 0;
                $inc   = 1;
            }
            else if ($tokens[4]) // every N hours : */N
            {
                $start = 0;
                $inc   = $tokens[4];
            }
            else // every N hour, starting from hour H : */N+H
            {
                $start = $tokens[6];
                $inc   = $tokens[5];
            }
            
            for ($hour = $start ; $hour < 24 ; $hour += $inc)
            {
                $time = 60 * (60 * $hour + $minute);
                $events_bag['pile'][$time] = $e;
            }
        }
        else if (preg_match ($regexp_cycle, $event, $tokens))
        {
            $e = array ();
            if ($tokens[5])
            {
                $e['type']  = $tokens[5];
                $e['value'] = $tokens[6];
            }
            else
            {
                $e['type']     = 'CRITERIA';
                $e['criteria'] = $tokens[7];
                $e['value']    = $tokens[8];
            }
            $e['min']   = $tokens[1];
            $e['max']   = ($tokens[2] > $e['min']) ? $tokens[2] : ($e['min'] + 1);
            $e['unity'] = $tokens[3];
            if ($e['unity'] == 'MINUTE')
            {
                $e['min'] *= 60;
                $e['max'] *= 60;
            }

            $events_bag['rotated'][] = $e;
        }
        else
        {
            $LOG->log (sprintf ($MSGS['K_EVENT_SYNTAX_ERROR'], $event));
        }
    }
    
    ksort ($events_bag['pile']);
    return $events_bag;
}

?>
