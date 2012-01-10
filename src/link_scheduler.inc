<?php // $Id: link_scheduler.inc 52 2007-04-08 18:48:58Z mouke $

/*****************************************************************************/

function D_Has_Links ($queue, $margin, $dbo)
{
    $tags = array (D_TAG_Link, D_TAG_RequestLink);
    $qid = count ($queue) - 1;
    if (is_numeric ($margin))
    {
        if (!D_Is_In_Queue ($queue, $qid, array ('number' => $margin), $tags, ''))
        {
            $left = $margin - $qid - 1;
            if ($left > 0)
            {
                return D_SQL_Has_Tags ($dbo, $tags, $left, '', true);
            }

            // else
            return false;
        }
        
        // else
        return true;
    }
    else
    {
        if (!D_Is_In_Queue ($queue, $qid, 
                    array ('time' => strtotime($margin)), 
                    $tags, ''))
        {
            if (strtotime ($margin) < $queue[0]['broadcast_time'])
            {
                return D_SQL_Has_Tags ($dbo, $tags, $margin, '', true);
            }

            // else
            return false;
        }
        
        // else
        return true;
    }
}

/*****************************************************************************/

function D_Add_Link (&$queue, $l_song, $config, $dbo)
{
    global $LOG, $MSGS;
    $LOG->log ("Testing for links", PEAR_LOG_DEBUG);

    $broadcast_time = $l_song['broadcast_time'];
    $p_song = $queue[count($queue) - 1];

    $l_schema = $config['programs'][$l_song['program']]['links_schema'];
    $p_schema = $config['programs'][$p_song['program']]['links_schema'];

    if (!D_Links_Allowed ($l_schema) && !$p_schema['at_end'])
    {
        // Means there is no link to compute, even if we're in between
        // programs
        return false;
    }
    
    $force = false;
    $prog  = false;
    $jstack = array ();
    
    // 1. Check for jingles at start / end of programs
    if ($l_song['program'] != $p_song['program']
        && ($l_schema['at_start'] || $p_schema['at_end']))
    {
        $prog = true;
        D_Select_Start_End_Jingle ($jstack, $queue,
                                   $l_schema, $p_schema,
                                   $l_song, $p_song, $dbo);
        if (!$jstack)
        {
            $force = true;
        }
    }

    // 2. Check for events jingles
    $e_tested = false;
    if (D_Is_Event ($l_song)
        && ($force
            || $l_schema['before_events']
            || D_Verify_Event_List ($l_song, $l_schema['before_events_list'])))
    {
        if (!D_Select_Event_Jingle ($jstack, $broadcast_time, $l_schema, $l_song, $dbo))
        {
            $force = true;
        }
        else
        {
            $force = false;
        }
        $e_tested = true;
    }

    // 3. Check for forced jingles
    $s_tested = false;
    if (   ($force && $l_schema['specific_jingle'])
        || (!$jstack && $l_schema['force_specific']))
    {   
        $jingle = D_Get_Specific_Jingle ($queue, $l_schema, $l_song, $dbo);
        if ($jingle)
        {
            $jstack[] = $jingle;
            $force = false;
        }
        $s_tested = true;
    }

    // 3.5. if no jingle has been found for forced events, then drop the
    // force (event jingle are forced only if we find a specific jingle
    // (event or whatever))
    if ($e_tested && $force)
    {
        $force = false;
    }

    // 3.75 a command forces a jingle
    if ($config['commands']['jingle'])
    {
        $force = true;
	    $LOG->log (sprintf ($MSGS['K_COMMAND'], 'jingle'));
    }

    // 4. Check for non forced jingles
    if ($force
        || (!$jstack && ($prog || D_Is_Time_For_Link ($queue, $l_schema, $l_song, $dbo))))
    {
        // The test is just to get Event jingles according to the normal jingle
        // flow when jingles before events are not enforced.
        if ((!$e_tested
             && !(D_Is_Event ($l_song)
                  && D_Select_Event_Jingle ($jstack, $broadcast_time, $l_schema, $l_song, $dbo)))
            || $force
    	    || $prog)
        {
            if (!$s_tested && $l_schema['specific_jingle'])
            {
                $jingle = D_Get_Specific_Jingle ($queue, $l_schema, $l_song, $dbo);
            }
            
            if (!$jingle && $l_schema['has_prog'] && $l_schema['prog_jingle'])
            {
                $jingle = D_SQL_Get_Jingle ($dbo,
                                            $l_song['program'],
                                            'PROGRAM',
                                            $l_schema['margin'],
                                            D_Date ($broadcast_time),
                                            D_Get_Links_From_Queue ($queue, 
                                                $broadcast_time - $l_schema['margin']));
                if ($jingle)
                {
                    $LOG->log ($MSGS['K_LINK_SCHEDULER_PROGRAM']);
                }
            }

            if (!$jingle && $p_schema['has_prog'] && $p_schema['prog_jingle'])
            {
                $jingle = D_SQL_Get_Jingle ($dbo,
                                            $p_song['program'],
                                            'PROGRAM',
                                            $p_schema['margin'],
                                            D_Date ($broadcast_time),
                                            D_Get_Links_From_Queue ($queue,
                                                $broadcast_time - $p_schema['margin']));
                if ($jingle)
                {
                    $LOG->log ($MSGS['K_LINK_SCHEDULER_PROGRAM']);
                }
            }
        
            if ($jingle)
            {
                $jstack[] = $jingle;
            }
        }
    }

    // 5. Fill the queue
    $r = $jstack ? true : false;
    
    while ($jstack)
    {
        $j = array_shift ($jstack);

        // Missing data
        $j['note']           = "n: {$l_song['song']};";
        $j['tag']            = D_TAG_Link;
        $j['broadcast_time'] = $broadcast_time;
        if (!$j['program'])
        {
            $j['program'] = $l_song['program'];
        }
        $j['playlist']          = -1;
        $j['song_type']         = D_TYPE_Jingle;
        $j['request']           = 0;
        $j['program_iteration'] = 0;

        $queue[] = $j;
        $broadcast_time += $j['length'] - $decider['config']['length_correction'];
    }
    
    return $r;
}

/*****************************************************************************/

function D_Links_Allowed ($schema)
{
    if ($schema['at_start']
        || $schema['at_end']
        || $schema['prog_jingle']
        || $schema['other_jingle'])
    {
        return true;
    }

    // else
    return false;
}

/*****************************************************************************/

function D_Verify_Event_List ($song, $event_notes)
{
    foreach ($event_notes as $event_note)
    {
        if (strstr ($song['note'], $event_note))
        {
            return true;
        }
    }
    
    return false;
}

/*****************************************************************************/

function D_Select_Start_End_Jingle(& $jstack,
                                   $queue,
                                   $l_schema, $p_schema,
                                   $l_song, $p_song,
                                   $dbo)
{
    global $LOG, $MSGS;
    $LOG->log ('Try for start/end links', PEAR_LOG_DEBUG);
    if ($l_schema['at_start'] && $l_schema['has_start'])
    {
        $jingle = D_SQL_Get_Jingle ($dbo,
                                    $l_song['program'],
                                    'PROGRAM_BEGIN',
                                    $l_schema['margin'],
                                    D_Date ($l_song['broadcast_time']),
                                    D_Get_Links_From_Queue (
                                        $queue,
                                        $l_song['broadcast_time'] - $l_schema['margin']
                                        )
                                    );
        if ($jingle)
        {
            $LOG->log ($MSGS['K_LINK_SCHEDULER_PROGRAM_BEGIN']);
            $jstack[] = $jingle;
        }
    }
    if ($p_schema['at_end'] && $p_schema['has_end'])
    {
        $jingle = D_SQL_Get_Jingle ($dbo,
                                    $p_song['program'],
                                    'PROGRAM_END',
                                    $p_schema['margin'],
                                    D_Date ($l_song['broadcast_time']),
                                    D_Get_Links_From_Queue (
                                        $queue,
                                        $l_song['broadcast_time'] - $p_schema['margin']
                                        )
                                    );
        if ($jingle)
        {
            $LOG->log ($MSGS['K_LINK_SCHEDULER_PROGRAM_END']);
            $jingle['program'] = $p_song['program'];
            array_unshift ($jstack, $jingle);
        }
    }
    if (!$jstack && false)
    {
        if ($l_schema['at_start'] && $l_schema['has_prog'])
        {
            $jingle = D_SQL_Get_Jingle ($dbo,
                                        $l_song['program'],
                                        'PROGRAM',
                                        $l_schema['margin'],
                                        D_Date ($l_song['broadcast_time']),
                                        D_Get_Links_From_Queue (
                                            $queue,
                                            $l_song['broadcast_time'] - $l_schema['margin']
                                            )
                                        );
            if ($jingle)
            {
                $LOG->log ($MSGS['K_LINK_SCHEDULER_PROGRAM_BEGIN']);
                $jstack[] = $jingle;
            }
        }
        else if ($p_schema['at_end'] && $p_schema['has_prog'])
        {
            $jingle = D_SQL_Get_Jingle ($dbo,
                                        $p_song['program'],
                                        'PROGRAM',
                                        $p_schema['margin'],
                                        D_Date ($l_song['broadcast_time']),
                                        D_Get_Links_From_Queue (
                                            $queue,
                                            $l_song['broadcast_time'] - $p_schema['margin']
                                            )
                                        );
            if ($jingle)
            {
                $LOG->log ($MSGS['K_LINK_SCHEDULER_PROGRAM_END']);
                $jingle['program'] = $p_song['program'];
                array_unshift ($jstack, $jingle);
            }
        }
    }
}

/*****************************************************************************/

function D_Select_Event_Jingle(& $jstack, $queue, $schema, $song, $dbo)
{
    global $LOG, $MSGS;
    $LOG->log ('Try for event links', PEAR_LOG_DEBUG);
    $jingle = false;
    $excluded = D_Get_Links_From_Queue (
            $queue,
            $song['broadcast_time'] - $schema['margin']
            );

    if (!$schema['priority_events'] && $schema['specific_jingle'])
    {
        $jingle = D_Get_Specific_Jingle ($queue, $schema, $song, $dbo);
    }

    if (!$jingle)
    {
        $jingle = D_Get_Specific_Event_Jingle ($excluded, $schema, $song, $dbo);
    }
                
    if (!$jingle)
    {
        $jingle = D_SQL_Get_Jingle ($dbo,
                                    $song['program'],
                                    'PROGRAM_EVENT',
                                    $schema['margin'],
                                    D_Date ($song['broadcast_time']),
                                    $excluded
                                    );
        
    }

    if (!$jingle && $schema['has_prog'])
    {
        $jingle = D_SQL_Get_Jingle ($dbo,
                                    $song['program'],
                                    'PROGRAM',
                                    $schema['margin'],
                                    D_Date ($song['broadcast_time']),
                                    $excluded
                                    );
        
    }

    if ($jingle)
    {
        $LOG->log ($MSGS['K_LINK_SCHEDULER_EVENT']);
        $jstack[] = $jingle;
    }

    return $jstack ? true : false;
}

/*****************************************************************************/

function D_Get_Specific_Jingle ($queue, $schema, $song, $dbo)
{
    global $LOG, $MSGS;
    $LOG->log ('Try for specific links', PEAR_LOG_DEBUG);
    
    $date = D_Date ($song['broadcast_time']);
    $margin = $schema['margin'];
    $excluded = D_Get_Links_From_Queue (
            $queue,
            $song['broadcast_time'] - $margin
            );


    $jingles = array ();
    $jingle = D_SQL_Get_Jingle ($dbo, $song['song'], 'SONG', $margin, $date,
                                $excluded);
    if ($jingle)
    {
        $LOG->log ($MSGS['K_LINK_SCHEDULER_SONG']);
        if ($schema['strict'])
            return $jingle;
        else
            $jingles[] = $jingle;
    }

    $jingle = D_SQL_Get_Jingle ($dbo,
                                D_SQL_Get_Artists ($dbo, array ($song['song']), false),
                                'ARTIST', $margin, $date, $excluded);
    if ($jingle)
    {
        $LOG->log ($MSGS['K_LINK_SCHEDULER_ARTIST']);
        if ($schema['strict'])
            return $jingle;
        else
            $jingles[] = $jingle;
    }

    $jingle = D_SQL_Get_Jingle ($dbo,
                                D_SQL_Get_Playlists ($dbo, array ($song['song']), false),
                                'PLAYLIST', $margin, $date, $excluded);
    if ($jingle)
    {
        $LOG->log ($MSGS['K_LINK_SCHEDULER_PLAYLIST']);
        if ($schema['strict'])
            return $jingle;
        else
            $jingles[] = $jingle;
    }

    if ($jingles)
    {
        return $jingles[array_rand($jingles)];
    }

    return false;
}

/*****************************************************************************/

function D_Get_Specific_Event_Jingle ($excluded, $schema, $song, $dbo)
{
    global $LOG;
    $LOG->log ('Try for specific event links', PEAR_LOG_DEBUG);

    $note = $song['note'];

    if (strstr ($note, 't: '))
    {
        list ($dummy, $note) = explode (';', $note);
    }
    
    if (strstr ($note, 'c: '))
    {
        $type = 'SONG_EVENT';
        $linked = $song['song'];
    }
    else
    {
        $linked = trim ($note, 'pas;: ');
        if (strstr ($note, 'p: '))
        {
            $type = 'PLAYLIST_EVENT';
        }
        else if (strstr ($note, 'a: '))
        {
            $type = 'ARTIST_EVENT';
        }
        else if (strstr ($note, 's: '))
        {
            $type = 'SONG_EVENT';
        }
    }

    return D_SQL_Get_Jingle ($dbo, $linked, $type, $schema['margin'],
                             D_Date ($song['broadcast_time']),
                             $excluded
                             );
}

/*****************************************************************************/

function D_Is_Time_For_Link ($queue, $schema, $song, $dbo)
{
    if (!$song['program_iteration']
        || $song['program_iteration'] % $schema['iteration'] != 0)
    {
        return false;
    }
        
    $tags = array (D_TAG_Link, D_TAG_RequestLink);
    if ($schema['freq_unit'] == 'SONG')
    {
        $min = $schema['freq_min'] - 1;
        $max = $schema['freq_max'] - 1;
    }
    else
    {
        $min = D_Date ($song['broadcast_time'] - $schema['freq_min']);
        $max = D_Date ($song['broadcast_time'] - $schema['freq_max']);
    }

    if (D_Has_Links ($queue, $min, $dbo))
    {
        return false;
    }

    if (!D_Has_Links ($queue, $max, $dbo))
    {
        return true;
    }
    
    // else
    if ($schema['freq_unit'] == 'SONG')
    {
        $r = mt_rand (0, $max - $min);
    }
    else
    {
        $r = mt_rand (0, intval (($schema['freq_max'] - $schema['freq_min']) / 180));
    }

    return $r == 0;
}

/*****************************************************************************/

function D_Get_Links_From_Queue ($queue, $limit)
{
    $links = array ();
    $qid = count ($queue) - 1;
    while ($queue[$qid]['broadcast_time'] > $limit and $qid >= 0)
    {
        if (D_Is_Link ($queue[$qid]))
        {
            $links[] = $queue[$qid]['song'];
        }
        --$qid;
    }
    return $links;
}

/*****************************************************************************/
    
?>
