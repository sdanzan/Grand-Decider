<?php // $Id: schema_parser_ordered.inc 16 2006-07-20 22:22:13Z mouke $

/**
 *  Parse a string with the following structure:
 *
 *    PLAYLIST n
 *    TOP TO BOTTOM | BOTTOM TO TOP
 *
 *  In case of errors, playlist will be set to default and random
 *  will be set to 1.
 */
function D_Schema_Parser_ORDERED ($schema_string, $prg, $dbo)
{
    global $CONFIG, $LOG, $MSGS;

    // Not required but useful anyway for debugging
    $schema = array ('type' => 'ORDERED');

    $LOG->log ("Schema prog $prg : \n$schema_string", PEAR_LOG_DEBUG);

    // And now, let's parse
    list ($playlist, $order) = explode ("\n", $schema_string);
    
    list ($keyword, $playlist) = preg_split ('/\s+/', $playlist);
    if (!is_numeric ($playlist))
    {
        $playlist = trim ($playlist, '][');
    }
    if ($keyword != 'PLAYLIST' || !is_numeric ($playlist))
    {
        $schema['playlist'] = $CONFIG['default_playlist'];
        $LOG->log (sprintf ($MSGS['K_PARSE_ORDERED_PLAYLIST_ERROR'],
                            $prg, $keyword, $playlist),
                   PEAR_LOG_ERR);
    }
    else
    {
        $schema['playlist'] = intval ($playlist);
    }

    if (!preg_match ('/^((TOP\s+TO\s+BOTTOM)|(BOTTOM\s+TO\s+TOP))$/',
                     $order))
    {
        $schema['order'] = 1;
        $LOG->log (sprintf ($MSGS['K_PARSE_ORDERED_ORDER_ERROR'],
                            $prg, $order),
                   PEAR_LOG_ERR);
    }
    else
    {
        if (!preg_match ('/^BOTTOM/', $order))
        {
            $schema['order'] = 1;
        }
        else
        {
            $schema['order'] = -1;
        }
    }

    return $schema;
}

?>
