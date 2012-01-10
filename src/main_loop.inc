<?php // $Id: main_loop.inc 53 2007-04-09 14:49:44Z mouke $

/*****************************************************************************/

/**
 * D_Main_Loop 
 * 
 * @param & $&decider 
 * @access public
 * @return void
 */
function D_Main_Loop (& $decider)
{
    D_Load_Grid          ($decider);
    if (D_Load_Current_Queue ($decider))
    {
        D_Load_New_Requests  ($decider);
        D_Build_New_Queue    ($decider);
        D_Commit_Queue       ($decider);
    }
}

/*****************************************************************************/

/** Loads the current programmation grid from the database and
 *  store it in DECIDER_STATE['grid']
 */
function D_Load_Grid (& $decider)
{
    $rows = D_SQL_Load_Grid ($decider['db'], $decider['config']['queue_size']);
    
    $Grid = array ();
    $Grid[] = array ();
    $Grid[] = $rows[0];
    $Grid[0] = array ('program' => $decider['config']['grid_default'],
                      'start' => 0,
                      'end' => $Grid[1]['start'] - 1);

    $N = count ($rows);
    for ($i = 1 ; $i < $N ; ++$i)
    {
        // If two programs are separated by more than the program margin
        // this means that the default program should be inserted in between
        $t1 = $rows[$i - 1]['end'];
        $t2 = $rows[$i]['start'];
        if (($t2 - $t1) > $decider['config']['prog_margin'])
        {
            $Grid[] = array ('program' => $decider['config']['grid_default'],
                             'start' => $t1 + 1,
                             'end' => $t2 - 1);
        }
        
        $Grid[] = $rows[$i];
    }
    $Grid[] = array ('program' => $decider['config']['grid_default'],
                     'start' => $Grid[count($Grid) - 1]['end'] + 1,
                     'end' => $Grid[count($Grid) - 1]['end'] + 3600);

    $decider['grid'] = $Grid;
}

/*****************************************************************************/

/**
 *  Reloads the current queue for waiting songs. Performs various treatments
 *  in prevision of queue rearrangement.
 *
 */
function D_Load_Current_Queue (& $decider)
{
    // Get the current song
    $decider['current_song'] = D_SQL_Load_Current_Song ($decider['db']);
    $decider['current_song']['broadcast_time'] =
        strtotime ($decider['current_song']['broadcast_time']);

    // Get the current queue
    $fullqueue = D_SQL_Load_Current_Queue ($decider['db']);
    foreach ($fullqueue as & $item)
    {
        $item['broadcast_time'] = strtotime ($item['broadcast_time']);
    }
    unset ($item);

    // Save the current queue, we will need it to compare with the new
    // computed queue
    $decider['old_queue'] = $fullqueue;
    $decider['old_songs'] = array ();

    // Split the queue into jingles, program songs, and requests
    $decider['reserve'] = array ();
    $decider['reserved'] = array ();
    $decider['reserve']['links'] = array ();
    $decider['reserve']['requests'] = array ();
    $decider['reserve']['songs'] = array ();
    $reserve = &$decider['reserve'];

    $force_reserve_next = false;
    foreach ($fullqueue as $item)
    {
        if (!$item['song'])
        {
            // Something is broken in the queue, we just just repair it
            // by filtering
            continue;
        }

        $item['broadcast_time'] = strtotime ($item['broadcast_time']);
        if (!D_Is_Link ($item))
        {
            $decider['old_songs'][] = $item;
        }
        if ($item['reserved'] or $force_reserve_next)
        {
            $decider['reserved'][] = $item;
            if (D_Is_Link ($item))
            {
                // We should also reserve the next song in this case
                // since links are linked to their following song
                $force_reserve_next = true;
            }
            else
            {
                $force_reserve_next = false;
            }
        }
        else if (D_Is_Link ($item))
        {
            $reserve['links'][] = $item;
        }
        else if ($item['request'])
        {
            $reserve['requests'][] = $item;
        }
        else
        {
            $reserve['songs'][] = $item;
        }
    }
    
    return true;
}

/*****************************************************************************/

function D_Load_New_Requests (& $decider)
{
    if ($decider['config']['use_requests'])
    {
        $requests = &$decider['reserve']['requests'];
        $requests = array_merge ($requests, D_SQL_Load_New_Requests ($decider['db']));
        usort ($requests, 'D_Compare_Requests');
    }
}

/*****************************************************************************/

require_once ('rules_apply.inc');
require_once ('song_schedulers.inc');
require_once ('event_scheduler.inc');
require_once ('request_scheduler.inc');
require_once ('link_scheduler.inc');

/**
 *  Builds a new queue, taking into account the already programmed songs
 *  and scheduling new ones if needed.
 *
 *  @param [$decider] a reference to the current state of the Grand Decider
 */
function D_Build_New_Queue (& $decider)
{
    global $DECIDER_SONG_SCHEDULERS, $LOG, $MSGS;
        
    $broadcast_time = $decider['current_song']['broadcast_time']
                        + $decider['current_song']['length']
                        - $decider['config']['length_correction'];

    $now = time ();
    if (   $broadcast_time + $decider['config']['crash_margin'] < $now
        or $decider['config']['commands']['rebuild']
        )
    {
        if ($decider['config']['commands']['rebuild'])
        {
	        $decider['config']['commands']['rebuild'] = false;
            $LOG->log (sprintf ($MSGS['K_COMMAND'], 'rebuild'));
        }
        else
        {
            // The queue holds old data. Let's do a fresh start.
            $LOG->log ($MSGS['K_RESTART_LONG_TIME_AFTER'], PEAR_LOG_INFO);
        }

        if ($broadcast_time + $decider['config']['crash_margin'] < $now)
        {
            $broadcast_time = $now;
        }
        
        // Reset the reserve, except for the pending requests.
        $decider['reserve']['links'] = array ();
        $decider['reserve']['songs'] = array ();
    }

    $decider['new_queue'] = array ();
    $full_queue = &$decider['new_queue'];
    
    // 1. First build the normal queue

    $LOG->log ("Broadcast time: " . D_Date ($broadcast_time),
               PEAR_LOG_DEBUG);
    
    // This one contains all : songs and links
    $full_queue[0] = $decider['current_song'];
    
    // This one contains only songs
    $new_queue = $full_queue;
    $queue_id = 0;

    // The reserved songs should never change
    while ($decider['reserved'])
    {
        $item = array_shift ($decider['reserved']);
        $item['broadcast_time'] = $broadcast_time;
        $full_queue[] = $item;
        if (!D_Is_Link ($item))
        {
            $new_queue[] = $item;
            ++$queue_id;
        }
        $broadcast_time += $item['length'] - $decider['config']['length_correction'];
    }
    
    $config = $decider['config'];
    while (count($new_queue) <= $config['queue_size'])
    {
        ++$queue_id;

        $LOG->log ("Broadcast time: " . D_Date ($broadcast_time),
               PEAR_LOG_DEBUG);
        $LOG->log ("Queue position: $queue_id", PEAR_LOG_DEBUG);

        // Get the current program...
        $grid_slot = D_Find_Program ($broadcast_time, 
                                     $decider['grid'],
                                     $config);
        
        $program = $grid_slot['program'];
        // Compute iteration
        if ($queue_id > 0 && $new_queue[$queue_id - 1]['program'] != $program)
        {
            $iteration = 1;
        }
        else
        {
            $iteration = $new_queue[$queue_id - 1]['program_iteration'] + 1;
        }

        // Timed events
        global $MASK;
        $song = D_Schedule_Timed_Event ($decider['reserve']['songs'],
                                        $broadcast_time,
                                        $grid_slot,
                                        $new_queue,
                                        $decider['config'],
                                        $decider['db']);
                
        if (is_array ($song) && array_key_exists ('next', $song))
        {
            $next_event = $song['next'];
            $song = false;
        }
        else
        {
            $next_event = false;
        }

        // Requests
        if (!$song)
        {
            $song = D_Schedule_Request ($decider['reserve'],
                                        $iteration,
                                        $broadcast_time,
                                        $next_event,
                                        $grid_slot,
                                        $new_queue,
                                        $decider['config'],
                                        $decider['db']);
        }
        
        // Check if song available from the reserve
        if (!$song)
        {
            $song = D_Check_Reserve ($decider['reserve']['songs'], $program);
        }

        // Rotated events
        if (!$song)
        {
            $song = D_Schedule_Rotated_Event ($broadcast_time,
                                              $grid_slot,
                                              $new_queue,
                                              $decider['config'],
                                              $decider['db']);
        }

        // Programs
        if (!$song)
        {
            // Get the corresponding scheduler
            $scheduler = 
                $DECIDER_SONG_SCHEDULERS[$config['programs'][$program]['type']];
            $LOG->log ("Scheduler: {$config['programs'][$program]['type']} / $scheduler",
                       PEAR_LOG_DEBUG);

            // Call scheduler and pile the song into the queue
            $song = $scheduler ($grid_slot,
                                $iteration,
                                $new_queue,
                                $decider['config'],
                                $decider['db']);
        
            if ($song['grid_change'])
            {
                D_Load_Grid ($decider);
                continue;
            }
        }
        
        if (!$song['song'])
        {
            $LOG->log ($MSGS['K_NO_SONG_FOUND']);
            $song = D_Get_Panic_Song ($decider['db'], $program);
        }

        $song['broadcast_time'] = $broadcast_time;
        $song['request'] = $song['request'] ? $song['request'] : 0;
        if ($song)
        {
            $song['program_iteration'] = $iteration;
            if (D_Is_Event ($song) && $config['event_no_iteration'])
            {
                --$song['program_iteration'];
            }
            else if (!$song['request']
                     && $queue[$queue_id - 1]['request']
                     && $queue[$queue_id - 1]['program'] == $program
                     && !$config['programs'][$program]['request_replace'])
            {
                --$song['program_iteration'];
            }
            if (isset ($song['skipped_iteration']))
            {
                $song['program_iteration'] += $song['skipped_iteration'];
                unset ($song['skipped_iteration']);
            }
        }

        if ($decider['old_songs'][$queue_id - 1]['song'] == $song['song']
            && $decider['reserve']['links'])
        {
            // Try to find a link from the reserve
            $link = array_shift ($decider['reserve']['links']);
            while (D_Is_Link_Of ($link, $song))
            {
                $link['broadcast_time'] = $broadcast_time;
                $full_queue[] = $link;
                $broadcast_time += $link['length'] - $decider['config']['length_correction'];
                $link = array_shift ($decider['reserve']['links']);
            }
            array_unshift ($decider['reserve']['links'], $link);
        }
        else
        {
            $decider['reserve']['links'] = array ();
            if (D_Add_Link ($full_queue, $song, $config, $decider['db']))
            {
                $last = $full_queue[count($full_queue) - 1];
                $broadcast_time = $last['broadcast_time'] 
                    + $last['length'] 
                    - $decider['config']['length_correction'];
            }
        }

        $song['broadcast_time'] = $broadcast_time;
        $broadcast_time += $song['length'] - $decider['config']['length_correction'];
        
        $full_queue[] = $song;
        $new_queue[] = $song;
    }

    // Drop the current song from the queue
    array_shift ($full_queue);
}

/*****************************************************************************/

/**
 *  Finds the program in which the provided broadcast time belongs.
 *
 *  @param [$broadcast_time] the broadcast time of the next song
 *  @param [$grid] the current program grid
 *  @param [$config] the current global configuration array
 *  @return the index of the program in the grid item matching
 *          the given broadcast time
 */
function D_Find_Program ($broadcast_time, $grid, $config)
{
    global $LOG;
    $prev_grid_item = array ();
    foreach ($grid as $grid_item)
    {
        if ($broadcast_time < $grid_item['start'] - $config['prog_margin'])
        {
            $LOG->log ("Program found: {$prev_grid_item['program']} (" .
                       D_Date ($prev_grid_item['start']) . ' to ' .
                       D_Date ($prev_grid_item['end']) . ')', 
                       PEAR_LOG_DEBUG);
            return $prev_grid_item;
        }
        $prev_grid_item = $grid_item;
    }
    $LOG->log ('Program not found (' . 
                D_Date ($broadcast_time) .
                '), choosing default.', 
               PEAR_LOG_DEBUG);
    return array ('program' => $config['grid_default'],
                  'start' => 0,
                  'end' => 0);
}

/*****************************************************************************/

function D_Commit_Queue ($decider)
{
    global $LOG, $MSGS;

    if (D_Compare_Queues ($decider['new_queue'], $decider['old_queue']))
    {
        $LOG->log ("Queues are identical, nothing to do.\n", PEAR_LOG_DEBUG);
    }
    else
    {
        if (D_SQL_Commit_Queue ($decider['db'], $decider['new_queue'], $decider['current_song']))
        {    
            if ($decider['config']['change_hook'])
            {
                exec (escapeshellcmd ($decider['config']['change_hook']));
            }
            
            $Table = '';
            
            $Legend =
                "ID | Time     |  Song  | Length | Request | Prog | Iter | Pos | Tag  | Note           ";
            $Key =
                "position|broadcast_time|song|length|request|program|program_iteration|position_in_playlist|tag|note";
            
            $A_Legend = explode("|", $Legend);
            $A_Key = explode("|", $Key);
            $Line = '';

            for ($i=0 ; $i < count($A_Legend) ; ++$i)
            {
                $Line .= str_repeat("-", strlen ($A_Legend[$i])) . '+';
            }

            $Table .= "$Line\n";
            $Table .= "$Legend|\n";
            $Table .= "$Line\n";
            $Length = count ($decider['new_queue']);

            $Table .= str_pad (0, strlen ($A_Legend[0]), ' ') . '| ';
            
            for ($j = 1 ; $j < count ($A_Legend) ; ++$j)
            {
                $Text = $decider['current_song'][trim($A_Key[$j])];
                if ($j == 1)
                    $Text = D_Time ($Text);

                $Table .= str_pad ($Text,
                                   strlen($A_Legend[$j]) - 2,
                                   ' ',
                                   STR_PAD_LEFT) . ' | ';
            }
            $Table .= "\n$Line\n";

            for ($i = 0 ; $i < $Length ; ++$i)
            {
                $pad = $i + 1;
                
                $Table .= str_pad ($pad, strlen($A_Legend[0]), ' ') . '| ';
                for ($j = 1 ; $j < count ($A_Legend) ; ++$j)
                {
                    $Text = $decider['new_queue'][$i][trim($A_Key[$j])];
                    if ($j == 1)
                    {
                        $Text = D_Time ($Text);
                    }
                    

                    $Table .= str_pad ($Text,
                                       strlen($A_Legend[$j]) - 2,
                                       ' ',
                                       STR_PAD_LEFT) . ' | ';
                }
                $Table .= "\n";
            }
            $Table .= "$Line\n";

            $LOG->log ("\n$Table\n", PEAR_LOG_INFO);

            // Done requests
            $requests = array ();
            foreach ($decider['new_queue'] as $song)
            {
                if ($song['request'])
                {
                    $requests[] = $song['request'];
                }
            }
            D_SQL_Mark_Request_Done ($decider['db'], $requests);

            // Pending requests
            $requests = array ();
            foreach ($decider['reserve']['requests'] as $request)
            {
                $requests[] = $request['request'];
            }
            D_SQL_Mark_Request_Not_Done ($decider['db'], $requests);
        }
    }
}

/*****************************************************************************/

?>
