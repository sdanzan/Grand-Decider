<?php // $Id: song_scheduler_structured_random.inc 57 2007-05-04 08:48:54Z mouke $

  /**
   *  Song scheduler for structured random programs.
   *
   *  @author Serge Danzanvilliers <mouke@bide-et-musique.com>
   *  @package Schedulers
   *
   */

  /**
   *  The structured random program song scheduler.
   *
   *  Structured random programs schedule songs according
   *  to weights distributed among some categories. Categories
   *  do not follow a cycle but are decided by their weights.
   *
   */
function D_Song_Scheduler_STRUCTURED_RANDOM ($grid_slot,
                                             $iteration,
                                             $queue,
                                             $config,
                                             $dbo)
{
    global $LOG, $MSGS;

    $queue_id = count ($queue);
    $program = $grid_slot['program'];
    
    $LOG->log ($MSGS['K_SCHEDULER_STRUCTURED_RANDOM_NEW_SONG']);
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
    $rconf = array ('playlists' => ($playlists ? $playlists : array ()),
                    'average_length' => $config['programs'][$program]['average_length']);
    $songs = false;

    $start_filter = array ();
    if ($playlists == 0)
    {
        $start_filter['criteria'][] =
            array ('criteria' => $schema['playlist']['criteria'],
                   'value'    => $schema['playlist']['value'],
                   'operator' => '=');
    }

    $category = D_Song_Scheduler_Select_Category ($schema['rotation_schema']);
    $LOG->log ("Category #$category chosen", PEAR_LOG_DEBUG);
    
    $category = $schema['rotation_schema']['categories'][$category];
    if (array_key_exists ('criteria', $category))
    {
        $start_filter['criteria'][] = array ('criteria' => $category['criteria'],
                                             'value'    => $category['value'],
                                             'operator' => '=');
    }
    else
    {
        $songs = D_SQL_Get_Playlist_Songs ($dbo,
                                           $category['playlist'],
                                           $playlists);
        $start_filter['songs_in'] = $songs;
    }
        
    $try = count($rules) +  1;
    while (!$song && $try)
    {
        $filter = $start_filter;

        D_Apply_Rules (& $filter, $rules, $queue, $rconf, $dbo);

        if (preg_match ('/RANDOMIZED/', $schema['rotation_schema']['rotation']))
        {
            $box = intval ($category['number']) / intval ($schema['rotation_schema']['rotation_block']) + 1;
            if ($schema['rotation_schema']['rotation'] == 'RANDOMIZED')
            {
                $r = mt_rand (1, 3);
            }
            else
            {
                $r = mt_rand (1, 2);
            }
            if ($box == 1 || $r == 1)
            {
                // Get the bottom song in the played pile
                $LOG->log ("Get oldest song", PEAR_LOG_DEBUG);
                $song = D_SQL_Song_Get_Oldest ($dbo, $playlists, $filter);
            }
            else if ($box == 2 || $r == 2)
            {
                // Get a song in the bottom 1/3 part of the played pile
                $time =
                    $queue[$queue_id - 1]['broadcast_time']
                    + $queue[$queue_id - 1]['length'];
                $time -= 2 * $category['roll_time'] / 3;
                $date = D_Date (intval ($time));
                $filter['criteria'][] = array ('criteria' => 'Last_Aired_On',
                                               'value'    => $date,
                                               'operator' => '<');
                $LOG->log ("Get song in the bottom half", PEAR_LOG_DEBUG);
                $song = D_SQL_Song_Get_Random ($dbo, $playlists, $filter);
            }
            else
            {
                // Get a random song
                $r = mt_rand (1, $box - 1);
                $time =
                    $queue[$queue_id - 1]['broadcast_time']
                    + $queue[$queue_id - 1]['length'];
                $time -= $r * $category['roll_time'] / $box;
                $date = D_Date (intval ($time));
                $filter['criteria'][] = array ('criteria' => 'Last_Aired_On',
                                               'value'    => $date,
                                               'operator' => '<');
                $LOG->log ("Get song in bottom from box $r", PEAR_LOG_DEBUG);
                $song = D_SQL_Song_Get_Random ($dbo, $playlists, $filter);
            }
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
    $song['note']              = '';

    return $song;
}

/*****************************************************************************/

function D_Song_Scheduler_Select_Category ($rotation_schema)
{
    global $LOG;
        
    $weights = array ();
    $total_weight = $rotation_schema['total_weight'];
    foreach ($rotation_schema['categories'] as $category)
    {
        if ($rotation_schema['weight'] == 'SONG')
            $w = $category['number'] * $category['weight'];
        else
            $w = $category['weight'];

        $weights[] = $w;
    }

    $dice = mt_rand (1, $total_weight);
    $LOG->log ("Random choice for category : $dice / $total_weight",
               PEAR_LOG_DEBUG);
    
    $accu = 0;
    foreach ($weights as $n => $w)
    {
        $accu += $w;
        if ($dice <= $accu)
        {
            return $n;
        }
    }
    
    // else
    return 0;
}

/*****************************************************************************/

?>
