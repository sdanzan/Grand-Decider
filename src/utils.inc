<?php // $Id :$

/*****************************************************************************/

function D_Midnight_Time ($t = false)
{
    return ($t === false) ? strtotime (date ('Y-m-d 00:00:00')) : strtotime (date ('Y-m-d 00:00:00', $t));
}

/*****************************************************************************/

function D_Time_From_Midnight ($t)
{
    return $t - D_Midnight_Time ($t);
}

/*****************************************************************************/

function D_Date ($t)
{
    return date ('Y-m-d H:i:s', $t);
}

/*****************************************************************************/

function D_Time ($t)
{
    return date ('H:i:s', $t);
}

/*****************************************************************************/

function D_Quote ($item)
{
    return "'$item'";
}

/*****************************************************************************/
                 
/**
 * D_Is_Link 
 * 
 * @param mixed $item 
 * @access public
 * @return void
 */
function D_Is_Link ($item)
{
    return ($item['tag'] == D_TAG_Link || $item['tag'] == D_TAG_RequestLink);
}

/*****************************************************************************/

function D_Is_Link_Of ($link, $song)
{
    return strstr ($link['note'], "n: {$song['song']};");
}

/*****************************************************************************/

function D_Is_Event ($item)
{
    return ($item['tag'] == D_TAG_Event || $item['tag'] == D_TAG_Rotated);
}

/*****************************************************************************/

/**
 * D_Compare_Queues 
 * 
 * @param mixed $queue_1 
 * @param mixed $queue_2 
 * @access public
 * @return void
 */
function D_Compare_Queues ($queue_1, $queue_2)
{
    $count = count ($queue_1);
    if ($count != count($queue_2))
    {
        return false;
    }
    else
    {
        for ($i = 0 ; $i < $count ; ++$i)
        {
            $diff = array_diff ($queue_1[$i], $queue_2[$i]);
            if (!empty ($diff))
            {
                return false;
            }
        }
    }
    
    return true;
}

/*****************************************************************************/

/**
 * D_Get_Panic_Song 
 * 
 * @param mixed $dbo 
 * @access public
 * @return void
 */
function D_Get_Panic_Song ($dbo, $program)
{
    global $CONFIG;
    $playlist = $CONFIG['panic_playlist'];

    $song = D_SQL_Song_Get_Random ($dbo, array ($playlist), array ());
    $song['program_iteration'] = 0;
    $song['program']           = 0;
    $song['playlist']          = $playlist;
    $song['tag']               = D_TAG_Panic_Song;
    $song['song_type']         = D_TYPE_Song;
    $song['note']              = '';

    return $song;
}

/*****************************************************************************/

function D_Parse_Strict_Or_Randomized (& $out, $line)
{
    $tokens = array ();
    if (!preg_match ('/^\s*(STRICT|(BOTTOM\s+|)RANDOMIZED)\s+ROTATION\s*$/',
                     $line, $tokens))
    {
        return false;
    }
    
    $out['rotation'] = $tokens[1];
    return true;
}

/*****************************************************************************/

function D_Parse_Playlist_Definition (& $out, $playlist)
{
    $tokens = array ();
    
    if (preg_match ('/^\s*PLAYLIST(S|)\s+(\[[0-9]+\])+\s*$/',
                    $playlist))
    {
        // A plain list of playlists
        list ($k, $playlist) = preg_split ('/\s+/', $playlist);
        $out['playlist'] = explode ('][', trim (trim($playlist), ']['));
    }
    else if (preg_match ('/^\s*PLAYLIST\s+DEFINED\s+AS\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+\[([A-Za-z0-9_]+)\]\s*$/',
                         $playlist, $tokens))
    {
        // A playlist defined by a criteria
        $out['playlist'] = array ('criteria' => $tokens[1],
                                  'value'    => $tokens[2]);
    }
    else    
    {
        // Error
        global $CONFIG;
        $out['playlist'] = array ($CONFIG['default_playlist']);
        return false;
    }

    return true;
}

/*****************************************************************************/

function D_Check_Reserve (& $reserve, $program)
{
    if ($reserve)
    {
        $song = array_shift ($reserve);

        if ($song['program'] == $program && $song['tag'] != D_TAG_Event)
        {
            global $LOG;
            $LOG->log("Using reserve", PEAR_LOG_DEBUG);
            return $song;
        }
        else
        {
            $reserve = array ();
            return false;
        }
    }
    return false;
}

/*****************************************************************************/

function D_Compare_Requests ($a, $b)
{
    if ($a == $b) return 0;
    if ($a['priority'] > $b['priority']) return -1;
    if ($a['priority'] < $b['priority']) return 1;
    if ($a['priority'] == $b['priority'])
    {
        if ($a['request'] > $b['request']) return 1;
        else return -1;
    }
}

/*****************************************************************************/

?>
