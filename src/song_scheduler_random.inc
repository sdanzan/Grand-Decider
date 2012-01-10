<?php // $Id: song_scheduler_random.inc 24 2006-07-25 11:11:46Z mouke $

  /**
   *  Song scheduler for random programs.
   *
   *  @author Serge Danzanvilliers <mouke@bide-et-musique.com>
   *  @package Schedulers
   *
   */

  /**
   *  The random program song scheduler.
   *
   *  Random programs schedule groups of songs randomly but ensures some
   *  kind or rotation on songs by selecting the most ancient played song
   *  about every two groups. Rules are applied on the first song of a group.
   *
   */
function D_Song_Scheduler_RANDOM ($grid_slot,
                                  $iteration,
                                  $queue,
                                  $config,
                                  $dbo)
{
    global $LOG, $MSGS;

    $queue_id = count ($queue);
    $program = $grid_slot['program'];

    $LOG->log ($MSGS['K_SCHEDULER_RANDOM_NEW_SONG']);

    $schema = $config['programs'][$program]['prog_schema'];
    $group_length = $schema['random'];
    $playlist = $schema['playlist'];
    
    if ($group_length == 1 || $iteration % $group_length == 1)
    {
        $policy = $config['programs'][$program]['policy'];
        $rules = $config['rules'][$policy];
        $rconf = array ('playlists' => array($playlist),
                        'average_length' => $config['programs'][$program]['average_length']);
        $songs = false;
        
        $try = count($rules) +  1;
        while (!$song && $try)
        {
            $filter = array ();
            D_Apply_Rules (& $filter, $rules, $queue, $rconf, $dbo);
        
            if ($iteration == 1
                || ((($group_length > 1 && $iteration % $group_length == 1)
                     || $group_length == 1) && mt_rand (1, 2) == 1))
            {
                // At the start of the program, or once every two groups,
                // schedule oldest song.
                $song = D_SQL_Song_Get_Oldest ($dbo,
                                               array ($playlist),
                                               $filter,
                                               $group_length);
            }
            else
            {
                // Schedule random song.
                $song = D_SQL_Song_Get_Random ($dbo,
                                               array ($playlist),
                                               $filter,
                                               $group_length);
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
    }
    else
    {
        $position = $queue[$queue_id - 1]['position_in_playlist'] + 1;
        $song = D_SQL_Song_Get_Specific_Position ($dbo, $playlist, $position);
    }

    $song['program']           = $program;
    $song['playlist']          = $playlist;
    $song['tag']               = D_TAG_Song;
    $song['song_type']         = D_TYPE_Song;
    $song['note']              = '';

    return $song;
}

?>
