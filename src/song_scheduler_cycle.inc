<?php // $Id: song_scheduler_cycle.inc 55 2007-04-11 13:58:15Z mouke $

  /**
   *  Song scheduler for structured random programs.
   *
   *  @author Serge Danzanvilliers <mouke@bide-et-musique.com>
   *  @package Schedulers
   *
   */

  /**
   *  The cycle program song scheduler.
   *
   *  This kind of programs schedule songs according
   *  to a predefined cycle of categories.
   *
   */
function D_Song_Scheduler_CYCLE ($grid_slot, $iteration, $queue, $config, $dbo)
{
    global $LOG, $MSGS;

    $queue_id = count ($queue);
    $program = $grid_slot['program'];

    $LOG->log ($MSGS['K_SCHEDULER_CYCLE_NEW_SONG']);
    $schema = $config['programs'][$program]['prog_schema'];
    if (array_key_exists ('criteria', $schema['playlist']))
    {
        $playlists = 0;
    }
    else
    {
        $playlists = $schema['playlist'];
    }
    
    $policy = $config['programs'][$program]['policy'];
    $rules = $config['rules'][$policy];

    // Prepare the supplementary parameters for the rules
    if ($playlists)
    {
        $p = $playlists;
    }
    else
    {
        $p = array ();
    }
    if ($schema['cycle_type'] == 'playlists')
    {
        $p = array_merge ($p, $schema['cycle']);
    }
    $rconf = array ('playlists' => $p,
                    'average_length' => $config['programs'][$program]['average_length']);
    
    $start_filter = array ();
    if ($playlists == 0)
    {
        $start_filter['criteria'][] =
            array ('criteria' => $schema['playlist']['criteria'],
                   'value'    => $schema['playlist']['value'],
                   'operator' => '=');
    }
    
    $category = ($iteration - 1) % $schema['cycle_length'];
    if ($schema['cycle_type'] != 'playlists')
    {
        $start_filter['criteria'][] =
            array ('criteria' => $schema['cycle_type']['criteria'],
                   'value'    => $schema['cycle'][$category],
                   'operator' => '=');
    }
    else
    {
        $playlist = trim ($schema['cycle'][$category], '^');
        $songs = D_SQL_Get_Playlist_Songs ($dbo,
                                           $playlist,
                                           $playlists);
        if (is_numeric ($schema['cycle'][$category]))
        {
            $start_filter['songs_in'] = $songs;
        }
        else
        {
            // means a ^ list
            $start_filter['songs_not_in'] = $songs;
        }
    }
        
    $try = count($rules) +  1;
    while (!$song && $try)
    {
        $filter = $start_filter;

        D_Apply_Rules (& $filter, $rules, $queue, $rconf, $dbo);

        if ($schema['rotation'] == 'RANDOMIZED' && mt_rand (1, 2) == 1)
        {
            // Get a random song
            $song = D_SQL_Song_Get_Random ($dbo, $playlists, $filter);
        }
        else
        {
            // Get the oldest song
            $song = D_SQL_Song_Get_Oldest ($dbo, $playlists, $filter);
        }

        if (!$song)
        {
            $dr = array_pop ($rules);
            --$try;
            if ($try)
            {
                $LOG->log (sprintf ($MSGS['K_RULE_REMOVE'], $dr['rule_def']));
            }
        }
    }

    $song['program']           = $program;
    $song['playlist']          = -1;
    $song['tag']               = D_TAG_Song;
    $song['song_type']         = D_TYPE_Song;
    if ($schema['cycle_type'] != 'playlists')
    {
        $note = "c: {$schema['cycle_type']['criteria']}; v: ";
    }
    else
    {
        $note = "p: ";
    }
    $song['note'] = $note . $schema['cycle'][$category] . ';';

    $ratio = $song['length'] / $schema['base_length'];
    if ($ratio > 1.75)
    {
        $song['skipped_iteration'] = round ($ratio) - 1;
    }
    else if ($ratio < 0.5)
    {
        $song['skipped_iteration'] = -1;
    }

    return $song;
}

/*****************************************************************************/

?>
