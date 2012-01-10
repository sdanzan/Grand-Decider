<?php // $Id: song_scheduler_ordered.inc 5 2006-07-14 22:33:25Z mouke $

/*****************************************************************************/

  /**
   *  The ordered scheduler schedules songs in the order of the songs
   *  in a given playlist, top to bottom or bottom to top. Rules are
   *  never taken into account.
   *
   */

function D_Song_Scheduler_ORDERED ($grid_slot, $iteration, $queue, $config, $dbo)
{
    global $LOG, $MSGS;

    $queue_id = count ($queue);
    $program = $grid_slot['program'];
    
    $schema = $config['programs'][$program]['prog_schema'];
    $playlist = $schema['playlist'];
    $order = $schema['order'];

    if ($queue_id > 0 && $queue[$queue_id - 1]['program'] != $program)
    {
        $position = 0;
    }
    else
    {
        $position = $queue[$queue_id - 1]['position_in_playlist'];
    }

    $LOG->log (sprintf ($MSGS['K_SCHEDULER_ORDERED_NEW_SONG'], $position));

    $song = array ();
    $try = 0;
    while (! $song && $try <= 1)
    {
        if ($order < 0 && $position = 0)
        {
            $LOG->log ('Getting last position for reverse order', PEAR_LOG_DEBUG);
            $song = D_SQL_Song_Get_Last_Position ($dbo, $playlist);
        }
        else
        {
            $song =
                D_SQL_Song_Get_Position_By_Order ($dbo, $playlist, $position, $order);
        }
        
        if (!$song)
        {
            $position = 0;
            ++$try;
        }
    }

    $song['program']           = $program;
    $song['playlist']          = $playlist;
    $song['tag']               = D_TAG_Song;
    $song['song_type']         = D_TYPE_Song;
    $song['note']              = '';

    return $song;
}

/*****************************************************************************/