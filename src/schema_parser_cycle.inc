<?php // $Id: schema_parser_cycle.inc 50 2007-04-07 11:49:22Z mouke $

/*****************************************************************************/

/**
 *  Parse a string with the following structure:
 *
 *    PLAYLIST [n]
 *    CYCLE PLAYLISTS | CYCLE CRITERIA [xxx]
 *    [n1]..[nx] | [v1]..[vx]
 *    BASE LENGTH [mean|nnn]
 *    (STRICT|RANDOMIZED) ROTATION
 *
 *  In case of errors, playlist will be set to default.
 */
function D_Schema_Parser_CYCLE ($schema_string, $prg, $dbo)
{
    global $CONFIG, $LOG, $MSGS;

    // Not required but useful anyway for debugging
    $schema = array ('type' => 'CYCLE');

    // And now, let's parse
    $lines = explode ("\n", $schema_string);
    
    $playlist = array_shift ($lines);
    $tokens = array ();

    // 1. Parse the playlist definition
    if (!D_Parse_Playlist_Definition ($schema, $playlist))
    {
        $LOG->log (sprintf ($MSGS['K_PARSE_CYCLE_PLAYLIST_ERROR'],
                            $prg, $playlist),
                   PEAR_LOG_ERR);
    }
    
    // 2. Parse the cycle definition
    $cycle = array_shift ($lines);
    if (preg_match ('/^\s*CYCLE\s+PLAYLISTS\s*$/', $cycle))
    {
        // A plain list of playlists
        $playlists = array_shift ($lines);
        if (preg_match ('/^\s*(\[\^?[0-9]+\])+\s*$/', $playlists))
        {
            $schema['cycle_type'] = 'playlists';
            $schema['cycle'] = explode ('][', trim (trim($playlists), ']['));
        }
        else
        {
            // Error
            $schema['cycle_type'] = 'playlists';
            $schema['cycle'] = array ($CONFIG['default_playlist']);
            $LOG->log (sprintf ($MSGS['K_PARSE_CYCLE_CYCLE_ERROR'],
                                $prg, $playlists),
                       PEAR_LOG_ERR);
        }
    }
    else if (preg_match ('/^\s*CYCLE\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s*$/',
                         $playlist, $tokens))
    {
        // A playlist defined by a criteria
        $schema['cycle_type'] = array ('criteria' => $tokens[1]);
        $playlists = array_shift ($lines);
        
        if (preg_match ('/^\s*(\[[A-Za-z0-9_]+\])+\s*$/', $playlists))
        {
            $schema['cycle'] = explode ('][', trim (trim($playlists), ']['));
        }
        else
        {
            // Error
            $schema['cycle_type'] = 'playlists';
            $schema['cycle'] = array ($CONFIG['default_playlist']);
            $LOG->log (sprintf ($MSGS['K_PARSE_CYCLE_CYCLE_ERROR'],
                                $prg, $playlists),
                       PEAR_LOG_ERR);
        }
    }
    else
    {
        // Error
        $schema['cycle_type'] = 'playlists';
        $schema['cycle'] = array ($CONFIG['default_playlist']);
        $LOG->log (sprintf ($MSGS['K_PARSE_CYCLE_CYCLE_ERROR'],
                            $prg, $cycle),
                   PEAR_LOG_ERR);
    }
    $schema['cycle_length'] = count ($schema['cycle']);

    // 3. Cycle base length
    $base_length = array_shift ($lines);
    if (!preg_match ('/^\s*BASE\s+LENGTH\s+\[(mean|[0-9]+)\]\s*$/', 
                     $base_length, $tokens))
    {
        $schema['base_length'] = 'mean';
        $LOG->log (sprintf ($MSGS['K_PARSE_CYCLE_BASE_ERROR'],
                            $prg, $base_length),
                   PEAR_LOG_ERR);
    }
    else
    {
        $schema['base_length'] = $tokens[1];
    }

    // 4. Rotation type
    $strict = array_shift ($lines);
    if (!D_Parse_Strict_Or_Randomized ($schema, $strict))
    {
        $LOG->log (sprintf ($MSGS['K_PARSE_ROTATION_ERROR'], $prg, $strict),
                   PEAR_LOG_ERR);
        return false;
    }
    
    // 5. Get the average length
    $schema['average_length'] =
        D_SQL_Get_Average_Song_Length ($dbo, $schema['playlist']);
    if ($schema['base_length'] == 'mean')
    {
        $schema['base_length'] = $schema['average_length'];
    }

    return $schema;
}

/*****************************************************************************/

?>
