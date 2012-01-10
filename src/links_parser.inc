<?php // $Id: links_parser.inc 44 2007-02-03 21:42:10Z mouke $

/*****************************************************************************/

function D_Init_Link_Schema ()
{
    return array ('at_start'           => false,
                  'at_end'             => false,
                  'prog_jingle'        => false,
                  'specific_jingle'    => false,
                  'has_start'          => false,
                  'has_end'            => false,
                  'has_prog'           => false,
                  'freq_min'           => 0,
                  'freq_max'           => 0,
                  'freq_unit'          => 'SONG',
                  'iteration'          => 1,
                  'strict'             => false,
                  'exclusion_list'     => 0,
                  'force_specific'     => 0,
                  'before_events'      => 0,
                  'before_events_list' => array (),
                  'priority_events'    => false,
                  'margin'             => 3600,
                  'starts_with'        => array (),
                  );
                     
}

/*****************************************************************************/

function D_Parse_Links ($links_string, $program, $dbo)
{
    $schema = D_Init_Link_Schema ();

    $schema['has_start'] = D_SQL_Program_Has_Start_Jingle ($dbo, $program);
    $schema['has_end']   = D_SQL_Program_Has_End_Jingle ($dbo, $program);
    $schema['has_prog']  = D_SQL_Program_Has_Prog_Jingle ($dbo, $program);

    $links_string = explode ("\n", $links_string);
    foreach ($links_string as $link_string)
    {
        if (preg_match ('/^(\s*|\s*#.*)$/', $link_string))
        {
            continue;
        }

        $tokens = array ();
        if (preg_match ('/^\s*(START|END|ALL|PROGRAM|SPECIFIC)\s+LINKS\s+(ON|OFF)\s*$/',
                        $link_string,
                        $tokens))
        {
            if ($tokens[1] == 'ALL')
            {
                $toggle = ($tokens[2] == 'ON') ? true : false;
                $schema['at_start']     = $toggle;
                $schema['at_end']       = $toggle;
                $schema['specific_jingle'] = $toggle;
                $schema['prog_jingle']  = $toggle;
                continue;
            }
            
            unset ($prop);
            switch ($tokens[1])
            {
                case 'START':
                    $prop = &$schema['at_start'];
                    break;

                case 'END':
                    $prop = &$schema['at_end'];
                    break;

                case 'SPECIFIC':
                    $prop = &$schema['specific_jingle'];
                    break;

                case 'PROGRAM':
                    $prop = &$schema['prog_jingle'];
                    break;
            }

            $prop = ($tokens[2] == 'ON') ? true : false;
        }
        else if (preg_match ('/^\s*FREQUENCY\s+\[([0-9]+)\-([0-9]+)\s+(SONG|MINUTE)\]\s*$/',
                             $link_string,
                             $tokens))
        {
            $MUL = ($tokens[3] == 'MINUTE') ? 60 : 1;
            $schema['freq_unit'] = $tokens[3];
            $schema['freq_max']  = $tokens[2] * $MUL;
            $schema['freq_min']  = $tokens[1] * $MUL;
        }
        else if (preg_match ('/^\s*ITERATION\s+STEP\s+([0-9]+)\s*$/',
                             $link_string,
                             $tokens))
        {
            $schema['iteration'] = $tokens[1];
        }
        else if (preg_match ('/^\s*(STRICT|ANY)\s+LINKS\s*$/',
                             $link_string,
                             $tokens))
        {
            $schema['strict'] = ($tokens[1] == 'STRICT') ? true : false;
        }
        else if (preg_match ('/^\s*FORCE\s+SPECIFIC\s+LINKS\s*$/',
                             $link_string))
        {
            $schema['force_specific'] = true;
        }
        else if (preg_match ('/^\s*FORCE\s+BEFORE\s+EVENTS\s*$/',
                             $link_string,
                             $tokens))
        {
            $schema['before_events'] = true;
        }
        else if (preg_match ('/^\s*FORCE\s+BEFORE\s+EVENT\s+((SONG|ARTIST|PLAYLIST)\s+\[([0-9]+)\]|CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+\[([A-Za-z0-9_]+)\])\s*$/',
                             $link_string,
                             $tokens))
        {
            if ($tokens[3])
            {
                if ($tokens[2] == 'SONG')
                    $note = 's: ';
                else if ($tokens[2] == 'ARTIST')
                    $note = 'a: ';
                else if ($tokens[2] == 'PLAYLIST')
                    $note = 'p: ';

                $note .= $tokens[3] . ';';
            }
            else
            {
                // Not used for the moment and probably never will,
                // use playlists instead to group your songs
                $note = "c: {$tokens[4]}; v: {$tokens[5]};";
            }
            $schema['before_events_list'][] = $note;
        }
        else if (preg_match ('/^\s*EVENT\s+LINK\s+CHOICE\s+\[(EVENT|OTHER)\]\s+THEN\s+\[(EVENT|OTHER)\]\s*$/',
                             $link_string,
                             $tokens))
        {
            if ($tokens[1] == 'EVENT' && $tokens[2] == 'OTHER')
            {
                $schema['priority_events'] = true;
            }
            else
            {
                $schema['priority_events'] = false;
            }
        }
        else if (preg_match ('/^\s*EXCLUSION\s+LIST\s+\[([0-9]+)\]\s*$/',
                             $link_string,
                             $tokens))
        {
            $schema['exclusion_list'] = $tokens[1];
        }
        else if (preg_match ('/^\s*ROTATION\s+MARGIN\s+\[([0-9]+)\s+(HOUR|MINUTE)\]\s*$/',
                             $link_string,
                             $tokens))
        {
            $schema['margin'] = $tokens[1] * (($tokens[2] == 'HOUR') ? 3600 : 60);
        }
        else
        {
            global $LOG, $MSGS;
            $LOG->log (sprintf ('K_LINK_SYNTAX_ERROR', $link_string));
        }
    }

    return $schema;
}
    
/*****************************************************************************/

?>
