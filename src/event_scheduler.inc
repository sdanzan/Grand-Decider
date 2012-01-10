<?php // $Id: event_scheduler.inc 50 2007-04-07 11:49:22Z mouke $

/*****************************************************************************/

function D_Schedule_Timed_Event (& $reserve, $broadcast_time, $grid_slot,
                                 $queue, $config, $dbo)
{
    global $LOG;
    $LOG->log ("Scheduling a timed event", PEAR_LOG_DEBUG);

    $queue_id = count ($queue);
    $program = $grid_slot['program'];
    $events_schema = $config['programs'][$program]['events'];
    if (!$events_schema['pile'])
    {
        $LOG->log ("No event to schedule", PEAR_LOG_DEBUG);
        return false;
    }

    $pile = $events_schema['pile'];
    
    // 1. Search for a timed event to schedule
    $broadcast = D_Time_From_Midnight ($broadcast_time);
    
    $LOG->log ("Searching for $broadcast", PEAR_LOG_DEBUG);
    $found = false;
    $prev = false;
    $next = false;
    $nb = count ($pile);

    // 1. if only one event
    if ($nb == 1)
    {
        $prev = $pile;
        $next = $prev;
    }
    else
    {
        foreach ($pile as $time => $event)
        {
            // We search the first event to be scheduled after the current
            // broadcast time
            if ($time >= $broadcast)
            {
                $next = array ($time => $event);
                break;
            }

            // Keep the previous event
            $prev = array ($time => $event);
        }
        reset ($pile);
        
        if ($next === false)
        {
            $next = array (key ($pile) => current ($pile));
        }

        if ($prev === false)
        {
            end ($pile);
            $prev = array (key ($pile) => current ($pile));
        }
        reset ($pile);
    }

    // Now we should have both events surrounding the current broadcast time.
    // We will keep the closest one.

    $diff_n = key ($next) - $broadcast;
    $diff_n += ($diff_n < 0) ? 86400 : 0;
    $diff_p = $broadcast - key ($prev);
    $diff_p += ($diff_p < 0) ? 86400 : 0;
    $diff = min ($diff_p, $diff_n);
    if ($diff < $config['event_margin'])
    {
        // Mark $found as true. $time will hold the event time.
        $time = ($diff_n < $diff_p) ? key ($next) : key ($prev);
        $event = ($diff_n < $diff_p) ? current ($next) : current ($prev);
        $found = true;
    }
    $next_time = key ($next);
    if ($next_time < $broadcast) $next_time += 86400;
    $next_time = array ('next' => $next_time);
    
    // 2. If no event found
    if (!$found)
    {
        // Returns next event time
        $LOG->log ("None found", PEAR_LOG_DEBUG);
        return $next_time;
    }

    // 3. Check in the queue if not already scheduled
    $qid = $queue_id - 1;
    while ($qid >= 0)
    {
        if ($queue[$qid]['program'] != $program)
        {
            break;
        }

        if ($queue[$qid]['tag'] == D_TAG_Event
            && strstr ($queue[$qid]['note'], "t: $time;"))
        {
            $LOG->log("D_Schedule_Timed_Event : already scheduled ({$queue[$qid]['note']})",
                      PEAR_LOG_DEBUG);
            return $next_time;
        }
        
        --$qid;
    }

    // 4. Get from reserve
    if ($reserve)
    {
        if (!$config['enforce_rules_on_change'])
        {
            $tmpstack = array ();
            $rfound = false;
            while ($reserve)
            {
                $song = array_shift ($reserve);
            
                if ($song['program'] == $program
                    && $song['tag'] == D_TAG_Event
                    && strstr ($song['note'], "t: $time;"))
                {
                    $LOG->log("D_Schedule_Timed_Event : using reserve", PEAR_LOG_DEBUG);
                    $rfound = true;
                    break;
                }

                $tmpstack[] = $song;
            }

            while ($tmpstack)
            {
                array_unshift ($reserve, array_pop ($tmpstack));
            }
        
            if ($rfound)
            {
                return $song;
            }
        }
        else
        {
            $song = array_shift ($reserve);
            if ($song['program'] == $program
                && $song['tag'] == D_TAG_Event
                && strstr ($song['note'], "t: $time;"))
            {
                $LOG->log("D_Schedule_Timed_Event : using reserve", PEAR_LOG_DEBUG);
                return $song;
            }
            else
            {   
                $reserve = array ();
                $song = false;
            }
        }
    }

    // 5. Check daypart rules
    $rules = $config['rules'][$config['programs'][$program]['policy']];
    if ($event['type'] != 'CRITERIA'
        && D_Has_Daypart_Rule ($rules))
    {
        if (D_SQL_Is_Dayparted ($dbo, $event['value'], $event['type'], $broadcast_time))
        {
            return $next_time;
        }
    }
    
    // 6. At least, schedule a new event
    global $MSGS;
    $LOG->log (sprintf ($MSGS['K_TIMED_EVENT_SCHEDULER_NEW_SONG'],
                        date ('H:i',
                              $time + D_Midnight_Time ($broadcast_time))));
    
    $song = D_Select_Event_Song ($event, $rules, $queue, $dbo);
    if ($reserve)
    {
        // Means the events has been recently added to the schema
        // => discard the reserve because it was computed without this song
        $reserve = array ();
    }
    
    $song['program']           = $program;
    $song['playlist']          = ($event['type'] == 'PLAYLIST') ? $event['value'] : -1;
    $song['tag']               = D_TAG_Event;
    $song['song_type']         = D_TYPE_Song;
    $song['note']              = "t: $time; ";
    $song['note']             .= D_Annotate ($event);
    
    return $song;
}

/*****************************************************************************/

function D_Schedule_Rotated_Event ($broadcast_time, $grid_slot,
                                   $queue, $config, $dbo)
{
    global $LOG;

    $LOG->log ("Scheduling a rotated event", PEAR_LOG_DEBUG);

    $queue_id = count ($queue);
    $program = $grid_slot['program'];
    $events_schema = $config['programs'][$program]['events'];
    if (!$events_schema['rotated'])
    {
        $LOG->log ("No event to schedule", PEAR_LOG_DEBUG);
        return false;
    }

    $rules = $config['rules'][$config['programs'][$program]['policy']];
    $select = false;
    foreach ($events_schema['rotated'] as $event)
    {
        // Check daypart rules
        if ($event['type'] != 'CRITERIA'
            && D_Has_Daypart_Rule ($rules))
        {
            if (D_SQL_Is_Dayparted ($dbo, $event['value'], $event['type'], $broadcast_time))
            {
                continue;
            }
        }
    
        $note = D_Annotate ($event);
        if ($event['unity'] == 'SONG')
        {
            if (D_Is_In_Queue ($queue, $queue_id - 1,
                               array ('number'=> $event['min']),
                               array (D_TAG_Event, D_TAG_Rotated), 
                               $note))
            {
                continue;
            }
            if ($event['min'] > $queue_id)
            {
                if (D_SQL_Has_Tags ($dbo, array (D_TAG_Rotated, D_TAG_Event),
                                    $event['min'] - $queue_id,
                                    $note))
                {
                    continue;
                }
            }
            
            if (!D_Is_In_Queue ($queue, $queue_id - 1,
                                array ('number' => $event['max']),
                                array (D_TAG_Event, D_TAG_Rotated), 
                                $note))
            {
                if ($event['max'] > $queue_id)
                {
                    if (!D_SQL_Has_Tags ($dbo, array (D_TAG_Rotated, D_TAG_Event),
                                         $event['max'] - $queue_id,
                                         $note))
                    {
                        $select = true;
                        break;
                    }
                }
                else
                {
                    $select = true;
                    break;
                }
            }

            // We are in the [min, max] period, let's decide randomly
            $r = mt_rand (0, $event['max'] - $event['min']);
            if ($r == 0)
            {
                $select = true;
                break;
            }
        }
        else // unity == MINUTE
        {
            $min = $broadcast_time - $event['min'];
            $max = $broadcast_time - $event['max'];
            if (D_Is_In_Queue ($queue, $queue_id - 1,
                               array ('time'=> $min),
                               array (D_TAG_Event, D_TAG_Rotated), 
                               $note))
            {
                continue;
            }
            if ($min < $queue[0]['broadcast_time'])
            {
                if (D_SQL_Has_Tags ($dbo, array (D_TAG_Rotated, D_TAG_Event),
                                    D_Date ($min),
                                    $note))
                {
                    continue;
                }
            }
            
            if (!D_Is_In_Queue ($queue, $queue_id - 1,
                                array ('time' => $max),
                                array (D_TAG_Event, D_TAG_Rotated), 
                                $note))
            {
                if ($max < $queue[0]['broadcast_time'])
                {
                    if (!D_SQL_Has_Tags ($dbo, array (D_TAG_Rotated, D_TAG_Event),
                                         D_Date ($max),
                                         $note))
                    {
                        $select = true;
                        break;
                    }
                }
                else
                {
                    $select = true;
                    break;
                }
            }

            // We are in the [min, max] period, let's decide randomly
            $r = mt_rand (0, intval(($event['max'] - $event['min']) / 180));
            if ($r == 0)
            {
                $select = true;
                break;
            }
        }
    }

    if ($select)
    {
        global $MSGS;
        $LOG->log (sprintf ($MSGS['K_ROTATED_EVENT_SCHEDULER_NEW_SONG']));

        $song = D_Select_Event_Song ($event, $rules, $queue, $dbo);
        
        $song['program']           = $program;
        $song['playlist']          = ($event['type'] == 'PLAYLIST') ? $event['value'] : -1;
        $song['tag']               = D_TAG_Rotated;
        $song['song_type']         = D_TYPE_Song;
        $song['note']              = D_Annotate ($event);
    
        return $song;
    }
    
    // else
    return false;
}

/*****************************************************************************/

function D_Is_In_Queue ($queue, $qid, $limit, $tag, $note)
{
    while ($qid >= 0)
    {
        if ((   $queue[$qid]['tag'] == $tag
             || (is_array ($tag) && in_array ($queue[$qid]['tag'], $tag))
            )
            && ($note == '' || strstr ($queue[$qid]['note'], $note) !== false))
        {
            return true;
        }

        if (array_key_exists ('time', $limit))
        {
            if ($queue[$qid]['broadcast_time'] < $limit['time'])
            {
                return false;
            }
        }
        else
        {
            --$limit['number'];
            if (!$limit['number'])
            {
                return false;
            }
        }
        
        --$qid;
    }

    return false;
}

/*****************************************************************************/

function D_Annotate ($event)
{
    if ($event['type'] == 'PLAYLIST')
        $note = 'p: ';
    else if ($event['type'] == 'ARTIST')
        $note = 'a: ';
    else if ($event['type'] == 'SONG')
        $note = 's: ';
    else if ($event['type'] == 'CRITERIA')
        $note = "c: {$event['criteria']}; v: ";
    
    $note .= "{$event['value']};";
    return $note;
}

/*****************************************************************************/

function D_Select_Event_Song ($event, $rules, $queue, $dbo)
{
    global $LOG, $MSGS;

    $filter = array ();
    $song = false;
    
    if ($event['type'] == 'SONG')
    {
        $filter['songs_in'] = array ($event['value']);
        $song = D_SQL_Song_Get_Oldest ($dbo, array (0), $filter);
    }
    else
    {
        $playlist = array (0);
        if ($event['type'] == 'ARTIST')
        {
            $filter['artists_in'] = array ($event['value']);
        }
        else if ($event['type'] == 'CRITERIA')
        {
            $filter['criteria'][] = array ('criteria' => $event['criteria'],
                                           'value'    => $event['value'],
                                           'operator' => '=');
        }
        else
        {
            $playlist = array ($event['value']);
        }

        $try = count($rules) +  1;
        $start_filter = $filter;
        while (!$song && $try)
        {
            $filter = $start_filter;
            
            D_Apply_Rules (& $filter, $rules, $queue,
                           array ('playlists' => $playlist),
                           $dbo);
            $song = D_SQL_Song_Get_Oldest ($dbo, $playlist, $filter);
            
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

    return $song;
}

/*****************************************************************************/

function D_Has_Daypart_Rule ($rules)
{
    foreach ($rules as $rule)
    {
        if ($rule['type'] == 'song_dayparts')
        {
            return true;
        }
    }
    return false;
}

/*****************************************************************************/

?>
