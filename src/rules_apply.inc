<?php // $Id: rules_apply.inc 56 2007-05-03 14:40:55Z mouke $

/****************************************************************************/

$RULE_APPLIERS = array (
    'separation_song'                        => 'D_Apply_Song_Separation',
    'separation_artist'                      => 'D_Apply_Artist_Separation',
    'separation_playlists'                   => 'D_Apply_Marked_Playlists_Separation',
    'separation_playlist'                    => 'D_Apply_Single_Playlist_Separation',
    'separation_criteria'                    => 'D_Apply_Criteria_Separation',
    'sequence_control_criteria_delta'        => 'D_Apply_Sequence_Control_Criteria_Delta',
    'sequence_control_criteria_sum'          => 'D_Apply_Sequence_Control_Criteria_Sum',
    'sequence_control_criteria_sawtooth'     => 'D_Apply_Sequence_Control_Criteria_Sawtooth',
    'sequence_control_criteria_peak'         => 'D_Apply_Sequence_Control_Criteria_Peak',
    'sequence_control_criteria_number'       => 'D_Apply_Sequence_Control_Criteria_Number',
    'sequence_control_criteria_multi_number' => 'D_Apply_Sequence_Control_Criteria_Multi_Number',
    'sequence_control_playlist_number'       => 'D_Apply_Sequence_Control_Playlist_Number',
    'sequence_control_playlists_number'      => 'D_Apply_Sequence_Control_Playlist_Number',
    'sequence_forbid_criteria'               => 'D_Apply_Sequence_Forbid_Criteria',
    'sequence_force_criteria'                => 'D_Apply_Sequence_Force_Criteria',
    'sequence_forbid_playlist'               => 'D_Apply_Sequence_Forbid_Playlist',
    'sequence_force_playlist'                => 'D_Apply_Sequence_Force_Playlist',
    'control_criteria'                       => 'D_Apply_Control_Criteria',
    'smart_dayparts'                         => 'D_Apply_Smart_Dayparts',
    'song_dayparts'                          => 'D_Apply_Song_Dayparts',
    'ignore'                                 => 'D_Apply_Ignore',
    );

/****************************************************************************/

$RULES_OPERATORS_REVERSE = array (
        '='  => '<>',
        '>'  => '<=',
        '<'  => '>=',
        '<>' => '=',
        '>=' => '<',
        '<=' => '>',
        );

/****************************************************************************/

function D_Apply_Rules (& $filter,
                        $rules,
                        $queue,
                        $rule_moreconf,
                        $dbo)
{
    global $RULE_APPLIERS, $LOG;

    foreach ($rules as $rule)
    {
        $parser = $RULE_APPLIERS[$rule['type']];
        if ($parser)
        {
            $parser ($filter, $rule, $queue, $rule_moreconf, $dbo);
        }
        else
        {
            $LOG->log ("No parser for rule\n{$rule['type']}\n{$rule['rule_def']}",
                       PEAR_LOG_DEBUG);
        }
    }
}

/****************************************************************************/

function D_Apply_Playlists_Separation (& $filter, $rule, $playlists, $queue,
                                       $conf, $dbo)
{
    $playlists = array_diff ($playlists, $conf['playlists']);

    if (!isset ($filter['songs_not_in']))
    {
        $filter['songs_not_in'] = array ();
    }

    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);

    $filter['songs_not_in'] =
        array_merge ($filter['songs_not_in'],
                     D_SQL_Get_Excluded_Playlist_Songs ($dbo, $playlists,
                                                        $from, $songs,
                                                        $conf['playlists']));
}

/****************************************************************************/

function D_Apply_Marked_Playlists_Separation (& $filter, $rule, $queue,
                                              $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply separation for playlists marked as '{$rule['mark']}'",
               PEAR_LOG_DEBUG);
    
    $playlists = D_SQL_Get_Marked_Playlists ($dbo, $rule['mark']);
        
    D_Apply_Playlists_Separation (& $filter, $rule, $playlists, $queue,
                                  $conf, $dbo);
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Single_Playlist_Separation (& $filter, $rule, $queue,
                                             $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply separation for playlists marked as '{$rule['mark']}'",
               PEAR_LOG_DEBUG);
    
    D_Apply_Playlists_Separation (& $filter, $rule, array ($rule['playlist']),
                                  $queue, $conf, $dbo);
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Criteria_Separation (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply separation for criteria '{$rule['criteria']}'",
               PEAR_LOG_DEBUG);
    if ($rule['value'])
    {
        $LOG->log ("Only for value '{$rule['value']}'", PEAR_LOG_DEBUG);
    }

    if (!isset ($filter['songs_not_in']))
    {
        $filter['songs_not_in'] = array ();
    }

    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);

    $filter['songs_not_in'] =
        array_merge ($filter['songs_not_in'],
                     D_SQL_Get_Excluded_Criteria_Songs ($dbo, 
		                                                $rule['criteria'], 
                                                        $rule['value'],
                                                        $from, $songs,
                                                        $conf['playlists']));
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Song_Separation (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply song separation", PEAR_LOG_DEBUG);

    D_Apply_Separation ($filter, $rule, $queue, $conf, $dbo,
                        'songs_not_in',
                        'D_SQL_Get_Previous_Time_Songs',
                        'D_SQL_Get_Previous_N_Songs');

    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Artist_Separation (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply artist separation", PEAR_LOG_DEBUG);

    D_Apply_Separation ($filter, $rule, $queue, $conf, $dbo,
                        'artists_not_in',
                        'D_SQL_Get_Previous_Time_Artists',
                        'D_SQL_Get_Previous_N_Artists');

    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Get_Queue_Songs_For_Rule ($rule, $queue)
{
    $queue_n = count ($queue);
    $songs = array ();
    
    if (isset ($rule['song']))
    {
        $n_to_check = $rule['song'];
        $in_table = 0;
        if ($n_to_check > $queue_n)
        {
            $in_table = $n_to_check - $queue_n;
            $n_to_check = $queue_n;
        }
        
        for ($i = 1 ; $i <= $n_to_check ; ++$i)
        {
            $songs[] = $queue[$queue_n - $i]['song'];
        }
        
        return array ($in_table, $songs);    
    }
    else
    {
        $interval = $rule['time'];
        $top_time = $queue[$queue_n - 1]['broadcast_time'] 
                    + $queue[$queue_n - 1]['length'];
        $bottom_time = $top_time - $interval;
        if ($queue[0]['broadcast_time'] > $bottom_time)
        {
            $from_time = D_Date ($bottom_time);
        }
        else
        {
            $from_time = '';
        }

        for ($i = 1 ;
             $queue[$queue_n - $i]['broadcast_time'] >= $bottom_time
                 && $i <= $queue_n ;
             ++$i)
        {
            $songs[] = $queue[$queue_n - $i]['song'];
        }

        return array ($from_time, $songs);
    }
}

/****************************************************************************/

function D_Apply_Separation (& $filter, $rule, $queue, $conf, $dbo,
                             $filter_key, $time_func, $n_func)
{
    $queue_n = count ($queue);
    $songs = array ();
    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);

    if (!isset ($filter[$filter_key]))
    {
        $filter[$filter_key] = array ();
    }

    $func = is_numeric ($from) ? $n_func : $time_func;

    $filter[$filter_key] = 
        array_merge ($filter[$filter_key],
                     $func ($dbo, $from, $songs, $conf['playlists']));
}

/****************************************************************************/

function D_Apply_Control_Criteria (& $filter, $rule,
                                   $queue = false, $conf = false, $dbo = false)
{
    global $LOG;
    $LOG->log ("Apply control criteria", PEAR_LOG_DEBUG);
    
    if (!isset ($filter['criteria']))
    {
        $filter['criteria'] = array ();
    }

    $filter['criteria'][] = $rule;
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}
    
/****************************************************************************/

/**
 *  Do not convert to 2 Control_Criteria rules because $filter should contain
 *  one element per rule to permit easy removing of rules.
 */
function D_Apply_Sequence_Control_Criteria_Delta (& $filter, $rule, $queue, $conf,
                                                 $dbo)
{
    global $LOG;
    $LOG->log ("Apply control criteria delta", PEAR_LOG_DEBUG);
        
    if (!isset ($filter['criteria_diff']))
    {
        $filter['criteria_diff'] = array ();
    }

    $control_value = D_SQL_Get_Criteria_Value ($dbo,
                                               $rule['criteria_1'],
                                               $queue[count($queue) - 1]['song']);
    $r = array ('criteria' => $rule['criteria_2'],
                'compare'  => $control_value,
                'operator' => $rule['operator'],
                'value'    => $rule['value'],
                'signed'   => $rule['signed']);
    $filter['criteria_diff'][] = $r;
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Control_Criteria_Sum (& $filter, $rule, $queue, $conf,
                                                $dbo)
{
    global $LOG;
    $LOG->log ("Apply control criteria sum", PEAR_LOG_DEBUG);
    
    $n_queue = count ($queue);
    $n = $rule['song'] - 1;
    while ($n_queue && $n)
    {
        $songs[] = $queue[$n_queue - 1]['song'];
        --$n;
        --$n_queue;
    }

    $sum = D_SQL_Get_Criteria_Sum ($dbo, $rule['criteria'], $n, $songs);
    $rule['value'] = $rule['sum'] - $sum;
    D_Apply_Control_Criteria ($filter, $rule);
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Control_Criteria_Multi_Number (& $filter, $rule,
                                                         $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply control criteria multi number", PEAR_LOG_DEBUG);
    
    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);
    $criterias = D_SQL_Get_Count_Criteria ($dbo,
                                           $rule['criteria'], $rule['number'],
                                           $from, $songs, 
                                           '', '=');
    foreach ($criterias as $criteria)
    {
        D_Apply_Control_Criteria (& $filter,
                                  array ('criteria' => $rule['criteria'],
                                         'operator' => '<>',
                                         'value'    => $criteria));
    }
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Control_Criteria_Number (& $filter, $rule, $queue,
                                                   $conf, $dbo)
{
    global $LOG, $RULES_OPERATORS_REVERSE;
    $LOG->log ("Apply control criteria number", PEAR_LOG_DEBUG);
    
    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);
    $criterias = D_SQL_Get_Count_Criteria_Global ($dbo,
                                                  $rule['criteria'], $rule['number'],
                                                  $from, $songs, 
                                                  $rule['value'], $rule['operator']);
    if ($criterias)
    {
        D_Apply_Control_Criteria (& $filter,
                                  array ('criteria' => $rule['criteria'],
                                         'operator' => $RULES_OPERATORS_REVERSE[$rule['operator']],
                                         'value'    => $rule['value']));
    }
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Control_Playlist_Number (& $filter, $rule, $queue,
                                                   $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply control playlist number", PEAR_LOG_DEBUG);
    
    if ($rule['mark'])
    {
        $playlists = D_SQL_Get_Marked_Playlists ($dbo, $rule['mark']);
    }
    else
    {
        $playlists = array ($rule['playlist']);
    }

    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);
    $playlists = D_SQL_Get_Count_Playlists ($dbo, $playlists, $rule['number'],
                                            $from, $songs, $rule['not']);
        
    $playlists = array_diff ($playlists, $conf['playlists']);
    $songs = D_SQL_Get_Playlist_Songs ($dbo, $playlists, $conf['playlists']);
    
    if (!isset ($filter['songs_not_in']))
    {
        $filter['songs_not_in'] = array ();
    }

    $filter['songs_not_in'] = array_merge ($filter['songs_not_in'], $songs);
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Forbid_Criteria (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG, $RULES_OPERATORS_REVERSE;
    $LOG->log ("Apply forbid criteria", PEAR_LOG_DEBUG);

    D_Apply_Sequence_Operator_Criteria (& $filter, $rule, $queue, $conf,
                                        $dbo, $RULES_OPERATORS_REVERSE[$rule['operator_1']]);

    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Force_Criteria (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply control force criteria", PEAR_LOG_DEBUG);

    D_Apply_Sequence_Operator_Criteria (& $filter, $rule, $queue, $conf,
                                        $dbo, $rule['operator_1']);
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Check_Operator ($v1, $v2, $operator)
{
    switch ($operator)
    {
        case '=':
            return $v1 == $v2;
            break;

        case '>':
            return $v1 > $v2;
            break;

        case '<':
            return $v1 < $v2;
            break;
    }
    
    return false;
}

/****************************************************************************/

function D_Apply_Sequence_Operator_Criteria (& $filter, $rule, $queue, $conf,
                                             $dbo, $operator)
{
    $control_value = D_SQL_Get_Criteria_Value ($dbo,
                                               $rule['criteria_2'],
                                               $queue[count($queue) - 1]['song']);
    if (D_Check_Operator ($control_value, $rule['value_2'], $rule['operator_2'])
        && mt_rand (1, 100) < $rule['percent'])
    {
        D_Apply_Control_Criteria ($filter,
                                  array ('criteria' => $rule['criteria_1'],
                                         'value'    => $rule['value_1'],
                                         'operator' => $operator));
    }
}

/****************************************************************************/

function D_Apply_Sequence_Forbid_Playlist (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply forbid playlist", PEAR_LOG_DEBUG);
    
    if (!in_array ($rule['playlist_2'], $conf['playlists']))
    {
        $playlists = D_SQL_Get_Playlists ($dbo, $queue[count($queue) - 1]['song']);
        if (in_array ($rule['playlist_1'], $playlists)
            && mt_rand (1, 100) < $rule['percent'])
        {
            if (!isset ($filter['songs_not_in']))
            {
                $filter['songs_not_in'] = array ();
            }

            $filter['songs_not_in'] =
                array_merge ($filter['songs_not_in'],
                             D_SQL_Get_Playlist_Songs ($dbo,
                                                       $rule['playlist_2'],
                                                       $conf['playlists']));
        }
    }
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Force_Playlist (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply force playlist", PEAR_LOG_DEBUG);
    
    $playlists = D_SQL_Get_Playlists ($dbo, $queue[count($queue) - 1]['song']);
    if (in_array ($rule['playlist_1'], $playlists)
        && mt_rand (1, 100) <= $rule['percent'])
    {
        if (!isset ($filter['songs_in']))
        {
            $filter['songs_in'] = array ();
        }

        $songs = D_SQL_Get_Playlist_Songs ($dbo,
                                           $rule['playlist_2'],
                                           $conf['playlists']);
        
        $filter['songs_in'] = ($filter['songs_in'] ?
                               array_intersect ($filter['songs_in'], $songs) :
                               $songs);
    }
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Smart_Dayparts (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply smart dayparts", PEAR_LOG_DEBUG);
    
    $n = count($queue) - 1;
    $parts = array (
        'when' => $queue[$n]['broadcast_time'] + $queue[$n]['length'],
        'parts' => $rule['parts'],
        'days' => $rule['days'],
        );
    $p = $rule['days'] ? $rule['days'] : $rule['parts'] - 2;
    $s = array ();
    while ($n > 0 and $p > 0)
    {
        $s[] = $queue[$n]['song'];
        --$p;
        --$n;
    }
    $parts['prevs'] = D_SQL_Get_Criteria_For_Songs ($dbo, $s, 'Last_Aired_On');
    
    $filter['smart_dayparts'] = $parts;
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Song_Dayparts (& $filter, $rule, $queue, $conf, $dbo)
{
    global $LOG;
    $LOG->log ("Apply song dayparts", PEAR_LOG_DEBUG);

    $n = count($queue) - 1;
    $broadcast_time = $queue[$n]['broadcast_time'] + $queue[$n]['length'];

    $results =
        D_SQL_Get_Dayparted_Songs ($dbo, $broadcast_time, $conf['playlists']);

    if ($results['songs'])
    {
        if (!isset ($filter['songs_not_in']))
        {
            $filter['songs_not_in'] = array ();
        }

        $filter['songs_not_in'] = array_merge ($filter['songs_not_in'],
                                               $results['songs']);
    }

    if ($results['artist'])
    {
        if (!isset ($filter['artists_not_in']))
        {
            $filter['artists_not_in'] = array ();
        }

        $filter['artists_not_in'] = array_merge ($filter['artists_not_in'],
                                                 $results['artists']);
    }
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Ignore (& $filter, $rule, $queue, $conf, $dbo)    
{
    global $LOG;
    $LOG->log ("Apply ignore rule", PEAR_LOG_DEBUG);

    $filter = array ();
    
    $LOG->log ("Done", PEAR_LOG_DEBUG);
}

/****************************************************************************/

function D_Apply_Sequence_Control_Criteria_Peak (& $filter, $rule, $queue,
                                                 $conf, $dbo)
{
    $rule['song'] = 2;
    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);
    $songs = D_SQL_Get_Previous_N_Songs ($dbo, $from, $songs);
    $criterias = D_SQL_Get_Criteria_For_Songs ($dbo, $songs, $rule['criteria']);

    if (count ($criterias) == 2)
    {
        $d = $criterias[0] - $criterias[1];
        if (abs ($d) >= $rule['threshold'])
        {
            $r = array ('criteria' => $rule['criteria']);
            if ($d > 0)
            {
                $r['operator'] = '>';
                $r['value'] = $criterias[0] - $rule['threshold'];
            }
            else
            {
                $r['operator'] = '<';
                $r['value'] = $criterias[0] + $rule['threshold'];
            }
            $filter['criteria'][] = $r;
        }
    }
}

/****************************************************************************/

function D_Apply_Sequence_Control_Criteria_Sawtooth (& $filter, $rule, $queue, 
                                                     $conf, $dbo)
{
    $rule['song'] = 3;
    list ($from, $songs) = D_Get_Queue_Songs_For_Rule ($rule, $queue);
    $songs = D_SQL_Get_Previous_N_Songs ($dbo, $from, $songs);
    $criterias = D_SQL_Get_Criteria_For_Songs ($dbo, $songs, $rule['criteria']);

    if (count ($criterias) == 3)
    {
        $d1 = $criterias[0] - $criterias[1];
        $d2 = $criterias[1] - $criterias[2];
        if (abs ($d1) >= $rule['threshold'] and abs ($d2) >= $rule['threshold'])
        {
            $s1 = $d1 ? ($d1 / abs($d1)) : 0;
            $s2 = $d2 ? ($d2 / abs($d2)) : 0;
            if ($s1 and $s2 and $s1 <> $s2)
            {
                // Means we've detected a condition for sawtooth form
                $r = array ('criteria' => $rule['criteria']);
                if ($s1 > 0)
                {
                    $r['operator'] = '>';
                    $r['value'] = $criterias[0] - $rule['threshold'];
                }
                else
                {
                    $r['operator'] = '<';
                    $r['value'] = $criterias[0] + $rule['threshold'];
                }
                $filter['criteria'][] = $r;
            }
        }
    }
}

/****************************************************************************/

?>
