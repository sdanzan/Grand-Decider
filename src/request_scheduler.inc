<?php // $Id: request_scheduler.inc 45 2007-02-28 12:54:46Z mouke $

/*****************************************************************************/

function D_Schedule_Request (& $reserve, $iteration, $broadcast_time, $next_event,
                             $grid_slot, $queue, $config, $dbo)
{
    $queue_id = count ($queue);
    $program = $grid_slot['program'];
        
    if (!$reserve['requests']
        || !$config['programs'][$program]['requests']
        || $iteration % $config['programs'][$program]['requests'] != 1)
    {
        return false;
    }

    if ($next_event !== false)
    {
        $time = $next_event + D_Midnight_Time ($broadcast_time);
    
        if ($next_event < D_Time_From_Midnight ($broadcast_time))
        {
            $time += 24 * 3600;
        }
    }
    else
    {
        $time = 0;
    }

    if ($time == 0 || $grid_slot['end'] == 0)
    {
        $time = max ($time, $grid_slot['end']);
    }
    else
    {
        $time = min ($time, $grid_slot['end']);
    }
    
    $m_length = ($time ? ($time - $broadcast_time) : -1);
        
    if ($m_length != -1
        && $reserve['requests'][0]['length'] > $m_length + $config['date_margin'])
    {
        return false;
    }

    // else
    $song = array_shift ($reserve['requests']);
    
    if ($queue_id > 0 && $queue[$queue_id - 1]['program'] != $program)
    {
        $song['position_in_playlist'] = 0;
    }
    else
    {
        $song['position_in_playlist'] = $queue[$queue_id - 1]['position_in_playlist'];
    }
    if ($config['programs'][$program]['request_replace'] and $song['new'])
    {
        $reserve['songs'] = array ();
    }
    
    $song['program']              = $program;
    $song['playlist']             = -1;
    $song['tag']                  = D_TAG_Song;
    $song['song_type']            = D_TYPE_Song;
    $song['note']                 = '';
    
    global $LOG, $MSGS;
    $LOG->log (sprintf ($MSGS['K_REQUEST_SCHEDULER_NEW_SONG'], $song['request']));

    return $song;
}

/*****************************************************************************/

?>
