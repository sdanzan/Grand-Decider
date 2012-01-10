<?php // $Id: schema_parser_random.inc 16 2006-07-20 22:22:13Z mouke $

/**
 *  Parse a string with the following structure:
 *
 *    PLAYLIST n
 *    RANDOM m
 *
 *  In case of errors, playlist will be set to default and random
 *  will be set to 1.
 */
function D_Schema_Parser_RANDOM ($schema_string, $prg, $dbo)
{
    global $CONFIG, $LOG, $MSGS;

    // Not required but useful anyway for debugging
    $schema = array ('type' => 'RANDOM');

    $LOG->log ("Schema prog $prg : \n$schema_string", PEAR_LOG_DEBUG);

    // And now, let's parse
    list ($playlist, $random) = explode ("\n", $schema_string);

    // 1. The playlist
    list ($keyword, $playlist) = preg_split ('/\s+/', $playlist);
    if (!is_numeric ($playlist))
    {
        $playlist = trim ($playlist, '][');
    }
    if ($keyword != 'PLAYLIST' || !is_numeric ($playlist))
    {
        $schema['playlist'] = $CONFIG['default_playlist'];
        $LOG->log (sprintf ($MSGS['K_PARSE_RANDOM_PLAYLIST_ERROR'],
                            $prg, $keyword, $playlist),
                   PEAR_LOG_ERR);
    }
    else
    {
        $schema['playlist'] = intval ($playlist);
    }

    // 2. The groups length
    list ($keyword, $random) = preg_split ('/\s+/', $random);
    if ($keyword != 'RANDOM' || !is_numeric ($random))
    {
        $schema['random'] = 1;
        $LOG->log (sprintf ($MSGS['K_PARSE_RANDOM_RANDOM_ERROR'],
                            $prg, $keyword, $random),
                   PEAR_LOG_ERR);
    }
    else
    {
        $schema['random'] = intval ($random);
    }

    // 3. Get the average length
    $schema['average_length'] =
        D_SQL_Get_Average_Song_Length ($dbo, $schema['playlist']);

    return $schema;
}

?>
