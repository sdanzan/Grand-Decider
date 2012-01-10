<?php // $Id: schema_parser_structured_random.inc 47 2007-03-16 16:47:34Z mouke $

/**
 *  Parse a string with the following structure:
 *
 *    (PLAYLISTS [n1]..[nn]|PLAYLIST DEFINED AS CRITERIA [xxx] VALUE [vvv])
 *    (STRICT|(BOTTOM\s+|)RANDOMIZED) ROTATION
 *    (ROTATION THRESHOLD n SONGS)
 *    ROTATION WEIGHT: (SONG|CATEGORY)
 *    CATEGORY (PLAYLIST [m]|CRITERIA [ccc] VALUE [vvv]) WEIGTH x
 *
 *  In case of errors, playlist will be set to default.
 */
function D_Schema_Parser_STRUCTURED_RANDOM ($schema_string, $prg, $dbo)
{
    global $CONFIG, $LOG, $MSGS;

    // Not required but useful anyway for debugging
    $schema = array ('type' => 'STRUCTURED_RANDOM');

    // And now, let's parse
    $lines = explode ("\n", $schema_string);
    
    $playlist = array_shift ($lines);
    $tokens = array ();

    // 1. Parse the playlist definition
    if (!D_Parse_Playlist_Definition ($schema, $playlist))
    {
        $LOG->log (sprintf ($MSGS['K_PARSE_STRUCTURED_RANDOM_PLAYLIST_ERROR'],
                            $prg, $playlist),
                   PEAR_LOG_ERR);
    }

    // 2. Get the average length
    $schema['average_length'] =
        D_SQL_Get_Average_Song_Length ($dbo, $schema['playlist']);
                
    // 3. Parse the definitions of rotation weights
    $schema['rotation_schema'] = D_Parse_Rotation_Weights ($lines,
                                                           $prg,
                                                           $schema['playlist'],
                                                           $schema['average_length'],
                                                           $dbo);
    
    return $schema;
}

/*****************************************************************************/

/**
 * D_Parse_Rotation_Weights 
 *
 *  Parse a string with the following structure:
 *
 *    (PLAYLISTS [n1]..[nn]|PLAYLIST DEFINED AS CRITERIA [xxx] VALUE [vvv])
 *    (STRICT|RANDOMIZED) ROTATION
 *    (ROTATION THRESHOLD n SONGS)
 *    ROTATION WEIGHT: (SONG|CATEGORY)
 *    CATEGORY (PLAYLIST [m]|CRITERIA [ccc] VALUE [vvv]) WEIGTH x
 *
 * @param mixed $lines 
 * @access public
 * @return void
 */
function D_Parse_Rotation_Weights ($lines, $program, $playlist, $average, $dbo)
{
    global $CONFIG, $MSGS, $LOG;

    $tokens = array ();

    // 1. The rotation description
    $strict = array_shift ($lines);
    if (!D_Parse_Strict_Or_Randomized ($rotation_schema, $strict))
    {
        $LOG->log (sprintf ($MSGS['K_PARSE_ROTATION_ERROR'], $program, $strict),
                   PEAR_LOG_ERR);
        return false;
    }

    // 2. If RANDOMIZED, the rotation threshold
    if (preg_match ('/RANDOMIZED/', $rotation_schema['rotation']))
    {
        $rotation_schema['percent_on_air'] =
            D_SQL_Program_Get_Percent_On_Air ($dbo, $program);
        if (preg_match ('/^\s*ROTATION\s+THRESHOLD\s+([0-9]+)\s+SONGS\s*$/',
                        $lines[0], $tokens))
        {
            $rotation_schema['rotation_block'] = $tokens[1];
            array_shift ($lines);
        }
        else
        {
            $rotation_schema['rotation_block'] = $CONFIG['default_rotation_block'];
        }
    }

    // 3. The rotation weight strategy
    $rotation = array_shift ($lines);
    if (!preg_match ('/^\s*ROTATION\s+WEIGHT:\s+(SONG|CATEGORY)\s*$/',
                     $rotation, $tokens))
    {
        $LOG->log (sprintf ($MSGS['K_PARSE_ROTATION_ERROR'], $program, $rotation),
                   PEAR_LOG_ERR);
        return false;
    }
    $rotation_schema['weight'] = $tokens[1];

    // 4. The categories description
    $categories = array ();
    $total = 0;
    foreach ($lines as $line)
    {
        if (!preg_match ('/^\s*CATEGORY\s+(PLAYLIST\s+\[([0-9]+)\]|CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+\[([A-Za-z0-9_]+)\])\s+WEIGHT\s+([0-9]+)\s*$/',
                         $line, $tokens))
        {
            $LOG->log (sprintf ($MSGS['K_PARSE_ROTATION_ERROR'], $program, $line),
                       PEAR_LOG_ERR);
            return false;
        }

        if ($tokens[2])
        {
            $number = D_SQL_Program_Get_SubPlaylist_Count ($dbo,
                                                           $playlist,
                                                           $tokens[2]);   
            $p = array ('playlist' => $tokens[2],
                        'weight'   => $tokens[5],
                        'number'   => $number);
        }
        else
        {
            $p = array ('criteria' => $tokens[3],
                        'value'    => $tokens[4],
                        'weight'   => $tokens[5]);
            $p['number'] = D_SQL_Program_Get_SubPlaylist_Count ($dbo,
                                                                $playlist,
                                                                $p);
        }

        if ($rotation_schema['weight'] == 'SONG')
        {
            $total += $p['weight'] * $p['number'];
        }
        else
        {
            $total += $p['weight'];
        }
        $categories[] = $p;
    }

    // 5. Compute some stats on categories
    foreach ($categories as & $category)
    {
        $category['proba'] = $category['weight'] / $total;
        if ($rotation_schema['weight'] == 'SONG')
        {
            $category['proba'] *= $category['number'];
        }

        if (preg_match ('/RANDOMIZED/', $rotation_schema['rotation']))
        {
            // In a given program, the number of scheduled songs required to roll
            // over all the songs of a category is:
            // N_song_in_category / probability_for_the_category_to_be_chosen
            // The probability is different wether the weights are per song or per
            // category.
            // To get the total average time required to roll over a given category
            // in a given program, we have to take into account the on air percentage
            // time of the program.
            
            $category['roll_time'] =
                $average
                * $category['number']
                / $category['proba']
                / $rotation_schema['percent_on_air'];
        }
    }
    
    $rotation_schema['categories'] = $categories;
    $rotation_schema['total_weight'] = $total;
    
    return $rotation_schema;
}

?>
