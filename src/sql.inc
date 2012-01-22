<?php // $Id: sql.inc 57 2007-05-04 08:48:54Z mouke $

/*****************************************************************************/

function D_Filter_Diff_In_Not_In (& $filter, $in, $not_in)
{
    if ($filter[$in] && $filter[$not_in])
    {
        $fin = array_diff ($filter[$in], $filter[$not_in]);
        
        // If the array is empty, we should let it as is and have
        // the corresponding request fail. So in/not_in are reduced
        // only if there are 'in' items left.
        if ($fin)
        {
            $filter[$not_in] = array ();
            $filter[$in] = $fin;
        }
    }
}

/*****************************************************************************/

function D_SQL_Quote_Array ($dbo, $array)
{
    $quoted = array ();
    foreach ($array as $a)
    {
        if (!is_object ($dbo))
        {
            $ctx = debug_backtrace();
            debug_print_backtrace();
            die();
        }
        $quoted[] = $dbo->quote ($a);
    }
    return $quoted;
}

/*****************************************************************************/

function D_SQL_Convert_Filter_To_SQL ($dbo, $filter)
{
    global $LOG;
    
    $sql = '';

    if ($filter['artists_not_in'])
    {
        $an = D_SQL_Get_Other_Songs_For_Artists ($dbo, $filter['artists_not_in']);
        if ($an)
        {
            if ($filter['songs_not_in'])
            {
                $filter['songs_not_in'] = array_merge ($filter['songs_not_in'], $an);
            }
            else
            {
                $filter['songs_not_in'] = $an;
            }
        }
    }
    
    D_Filter_Diff_In_Not_In ($filter, 'songs_in', 'songs_not_in');
    D_Filter_Diff_In_Not_In ($filter, 'artists_in', 'artists_not_in');
        
    if ($filter['songs_in'])
    {
        $sql .=
            " AND D_S_T.Song__ IN(" .
            implode (',', D_SQL_Quote_Array (
                        $dbo,
                        array_filter (
                            array_unique ($filter['songs_in'])
                            )
                        )
                    ) .
            ')';
    }

    if ($filter['artists_in'])
    {
        $sql .=
            " AND D_A_T.Artist__ IN(" .
            implode (',', D_SQL_Quote_Array (
                        $dbo,
                        array_filter (
                            array_unique ($filter['artists_in'])
                            )
                        )
                    ) .
            ')';
    }

    if ($filter['songs_not_in'])
    {
        $sql .=
            " AND D_S_T.Song__ NOT IN(" . 
            implode (',', D_SQL_Quote_Array (
                        $dbo,
                        array_filter (
                            array_unique ($filter['songs_not_in'])
                            )
                        )
                    ) .
            ')';
    }

    if ($filter['artists_not_in'])
    {
        $sql .=
            " AND D_A_T.Artist__ NOT IN(" .
            implode (',', D_SQL_Quote_Array (
                        $dbo,
                        array_filter (
                            array_unique ($filter['artists_not_in'])
                            )
                        )
                    ) .
            ')';
    }

    if ($filter['criteria'])
    {
        foreach ($filter['criteria'] as $rule)
        {
            $sql .= " AND D_S_T.{$rule['criteria']} {$rule['operator']} '{$rule['value']}'";
        }
    }

    if ($filter['criteria_diff'])
    {
        foreach ($filter['criteria_diff'] as $rule)
        {
            if (!is_numeric ($rule['compare']))
            {
                $rule['compare'] = 0;
            }
            if ($rule['signed'])
            {
                $sql .= " AND D_S_T.{$rule['criteria']} - {$rule['compare']} {$rule['operator']} '{$rule['value']}'";
            }
            else
            {
                $sql .= " AND ABS(D_S_T.{$rule['criteria']} - {$rule['compare']}) {$rule['operator']} '{$rule['value']}'";
            }
        }
    }

    if ($filter['smart_dayparts'])
    {
        $when = $filter['smart_dayparts']['when'];
        $margin = 12 / $filter['smart_dayparts']['parts'];
        $t1 = round ($when + $margin * 3600);
        $t2 = round ($when - $margin * 3600);
        $T1 = D_Date ($t1);
        $T2 = D_Date ($t2);
        $t1 = $t1 - D_Midnight_Time ($t1);
        $t2 = $t2 - D_Midnight_Time ($t2);
        if ($t2 <= $t1)
        {
            $sqlt = " (TIME(D_S_T.Last_Aired_On) > TIME('$T1')
                       OR TIME(D_S_T.Last_Aired_On) < TIME('$T2'))";
        }
        else
        {
            $sqlt = " (TIME(D_S_T.Last_Aired_On) > TIME('$T1')
                       AND TIME(D_S_T.Last_Aired_On) < TIME('$T2'))";
        }
        if ($filter['smart_dayparts']['prevs'])
        {
            foreach ($filter['smart_dayparts']['prevs'] as $prev)
            {
                if ($filter['smart_dayparts']['days'])
                {
                    $sqlt .= " AND DATE(D_S_T.Last_Aired_On) <> DATE('$prev') ";
                }
                else
                {
                    $sqlt .= " AND TIME(D_S_T.Last_Aired_On) <> TIME('$prev') ";
                }
            }
        }
        $sql .= " AND (D_S_T.Last_Aired_On = '0000-00-00 00:00:00' OR D_S_T.Last_Aired_On IS NULL OR ($sqlt)) "; 
    }
    
    return $sql;
}

/*****************************************************************************/

function D_SQL_Commit_Queue ($dbo, $queue, $current)
{
    global $CONFIG, $LOG, $MSGS;

    $state = true;
    
    $Q_T = $CONFIG['queue_table'];

    $dbo->beginTransaction ();
    $dbo->query ("DELETE FROM $Q_T");

    $current_current = D_SQL_Load_Current_Song($dbo);
    if ($current['song'] == $current_current['song'])
    {
        $request = "
INSERT INTO $Q_T (
 Position,
 Song__,
 Broadcast_Time,
 Request__,
 Program__,
 Playlist__,
 Program_Iteration,
 Position_In_Playlist,
 Song_Type,
 Tag,
 Note,
 Reserved
)
VALUES
";

        $position = 1;
        $values = array ();
        foreach ($queue as $song)
        {
            $value = "($position, '{$song['song']}', '" . D_Date ($song['broadcast_time']) . "', '{$song['request']}', '{$song['program']}', '{$song['playlist']}', '{$song['program_iteration']}', '{$song['position_in_playlist']}', '{$song['song_type']}', '{$song['tag']}', " . $dbo->quote ($song['note']) . ", '{$song['reserved']}')";
            ++$position;
            $values[] = $value;
        }

        $request .= implode (",\n", $values);

        try
        {
            $dbo->query ($request);
            $result = $dbo->query ("SELECT COUNT(*) FROM $Q_T");
            $n = $result->fetchColumn ();
            $result->closeCursor ();
    
            if ($n == 0)
            {
                $LOG->log (sprintf ($MSGS['K_COMMIT_FAILED'], $request),
                           PEAR_LOG_ERR);
                $dbo->rollBack ();
                $state = false;
            }
            else
            {
                $dbo->commit ();
            }
        }
        catch (PDOException $e)
        {
            $LOG->log (sprintf ($MSGS['K_COMMIT_REQUEST_FAILED'],
                                $e->getMessage(),
                                $request),
                       PEAR_LOG_ERR);
            $dbo->rollBack ();
            $state = false;
        }

        return $state;
    }

    $dbo->rollBack ();
    return false;
}

/*****************************************************************************/

/** 
 *  Grabs the program grid from the database and returns an ordered list
 *  of Grid elements :
 *    'program' => program UID
 *    'start'   => start timestamp
 *    'end'     => end timestamp
 */
function D_SQL_Load_Grid ($dbo, $nentries)
{
    global $CONFIG;
    
    $request = "
SELECT Program__      AS program,
       Date_Beginning AS start,
       Date_End       AS end
FROM   {$CONFIG['grid_table']}
WHERE      Date_Beginning > NOW() - INTERVAL 24 HOUR
       AND Date_End < NOW() + INTERVAL $nentries HOUR
       AND Date_End >= NOW()
ORDER BY Date_Beginning
";

    $result = $dbo->query ($request);
    
    $return_result = array ();
    while ($row = $result->fetch (PDO::FETCH_NUM))
    {
        $return_result[] = array ('program' => $row[0],
                                  'start' => strtotime ($row[1]),
                                  'end' => strtotime ($row[2]));
    }
    return $return_result;
}

/*****************************************************************************/

/**
 *  Fetch all the program definitions from the database,
 *  returns a list indexed by program UIDs
 *    UID => 'type'     => program type
 *           'policy'   => policy UID
 *           'schema'   => program schema, not parsed
 *           'links'    => linking strategy UID
 *           'requests' => accept song requests every N songs
 *           'events'   => events, not parsed
 */
function D_SQL_Load_Programs ($dbo)
{
    global $CONFIG;
    
    $request = "
SELECT Program__       AS program,
       Prog_Type       AS type,
       Policy__        AS policy,
       Prog_Schema     AS prog_schema,
       Events          AS events,
       Links_Schema    AS links_schema,
       Request_Every   AS requests,
       Request_Replace AS request_replace
FROM   {$CONFIG['programs_table']}
ORDER BY program
";

    $result = $dbo->query ($request);

    $return_result = array ();
    while ($row = $result->fetch (PDO::FETCH_ASSOC))
    {
        $puid = array_shift ($row);
        $return_result[$puid] = $row;
    }
    return $return_result;
}

/*****************************************************************************/

function D_SQL_Load_Rules ($dbo)
{
    global $CONFIG;
    $R_T = $CONFIG['rules_table'];

    $request = "
SELECT Policy__ AS policy,
       Rule     AS rule
FROM   $R_T
WHERE  Priority > 0
ORDER BY Priority DESC
";
    return $dbo->query($request)->fetchAll (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

/**
 *  Loads the current song queue from the database.
 *
 */
function D_SQL_Load_Current_Queue ($dbo)
{
    global $CONFIG;

    $Q_T = $CONFIG['queue_table'];
    $S_T = $CONFIG['full_library_table'];
    $R_T = $CONFIG['requests_table'];
    
    $request = "
SELECT $Q_T.Song__               AS song,
       $S_T.Length               AS length,
       $Q_T.Broadcast_Time       AS broadcast_time,
       $Q_T.Request__            AS request,
       $Q_T.Program__            AS program,
       $Q_T.Playlist__           AS playlist,
       $Q_T.Program_Iteration    AS program_iteration,
       $Q_T.Position_In_Playlist AS position_in_playlist,
       $Q_T.Song_Type            AS song_type,
       $Q_T.Tag                  AS tag,
       $Q_T.Note                 AS note,
       $Q_T.Reserved             AS reserved,
       $Q_T.Position             AS position,
       $R_T.Priority             AS priority
FROM   $Q_T LEFT JOIN $S_T ON $Q_T.Song__ = $S_T.Song__
            LEFT JOIN $R_T ON $Q_T.Request__ = $R_T.Request__
";
    
    return $dbo->query($request)->fetchAll (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

/**
 *  Loads the current song from the database.
 *
 */
function D_SQL_Load_Current_Song ($dbo)
{
    global $CONFIG;

    $N_T = $CONFIG['now_table'];
    $S_T = $CONFIG['full_library_table'];
    
    $request = "
SELECT $N_T.Song__               AS song,
       $S_T.Length               AS length,
       $N_T.Date_Begin           AS broadcast_time,
       $N_T.Request__            AS request,
       $N_T.Program__            AS program,
       $N_T.Song_Type            AS song_type,
       $N_T.Tag                  AS tag,
       $N_T.Program_Iteration    AS program_iteration,
       $N_T.Position_In_Playlist AS position_in_playlist,
       $N_T.Note                 AS note
FROM   $N_T LEFT JOIN $S_T ON $N_T.Song__ = $S_T.Song__
";
    
    return $dbo->query($request)->fetch (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Load_New_Requests ($dbo)
{
    global $CONFIG;

    $R_T = $CONFIG['requests_table'];
    $S_T = $CONFIG['song_table'];
    
    $request = "
SELECT $R_T.Request__ AS request,
       $R_T.Song__    AS song,
       $R_T.Priority  AS priority,
       $S_T.Length    AS length,
       1              AS new
FROM   $R_T LEFT JOIN $S_T ON $R_T.Song__ = $S_T.Song__
WHERE  $R_T.Done = 0
ORDER BY $R_T.Priority DESC, $R_T.Date
";

    return $dbo->query($request)->fetchAll (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Song_Get_Oldest ($dbo, $playlists, $filter, $random = 0)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get oldest song.", PEAR_LOG_DEBUG);

    $S_T = $CONFIG['song_table'];
    $A_T = $CONFIG['artists_songs_table'];
    $P_T = $CONFIG['playlist_songs_table'];
    
    if ($filter)
    {
        $FILTER = D_SQL_Convert_Filter_To_SQL ($dbo, $filter);
    }
    else
    {
        $FILTER = '';
    }

    if ($random > 1)
    {
        $FILTER .= " AND D_P_T.Position % $random = 1";
    }

    if (!$playlists || in_array (0, $playlists))
    {
        $LOG->log ("Using no playlist", PEAR_LOG_DEBUG);

        $request = "
SELECT D_S_T.Song__ AS song,
       D_S_T.Length AS length,
       0            AS position_in_playlist,
       D_S_T.Last_Aired_On
FROM   $S_T AS D_S_T INNER JOIN $A_T AS D_A_T ON D_S_T.Song__ = D_A_T.Song__
WHERE  1 $FILTER
ORDER BY D_S_T.Last_Aired_On, RAND()
LIMIT 1
";
    }
    else
    {
        $PLAYLISTS = implode (',', D_SQL_Quote_Array ($dbo, $playlists));
        $LOG->log ("Using playlists ($PLAYLISTS)", PEAR_LOG_DEBUG);

        $request = "
SELECT D_S_T.Song__ AS song,
       D_S_T.Length AS length,
       D_P_T.Position AS position_in_playlist,
       D_S_T.Last_Aired_On
FROM   $P_T AS D_P_T LEFT JOIN $S_T AS D_S_T ON D_P_T.Song__ = D_S_T.Song__
            INNER JOIN $A_T AS D_A_T ON D_S_T.Song__ = D_A_T.Song__
WHERE  D_P_T.Playlist__ IN ($PLAYLISTS)
       $FILTER
ORDER BY D_S_T.Last_Aired_On, RAND()
LIMIT 1
";
    }

    //echo "$request\n";
    return $dbo->query ($request)->fetch (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Song_Get_Random ($dbo, $playlists, $filter, $random = 0)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get random song.", PEAR_LOG_DEBUG);

    $S_T = $CONFIG['song_table'];
    $A_T = $CONFIG['artists_songs_table'];
    $P_T = $CONFIG['playlist_songs_table'];
    
    if ($filter)
    {
        $FILTER = D_SQL_Convert_Filter_To_SQL ($dbo, $filter);
    }
    else
    {
        $FILTER = '';
    }

    if ($random > 1)
    {
        $FILTER .= " AND D_P_T.Position % $random = 1";
    }

    if (!$playlists || in_array (0, $playlists))
    {
        $LOG->log ("Using no playlist", PEAR_LOG_DEBUG);
        $request = "
SELECT DISTINCT D_S_T.Song__ AS song,
       D_S_T.Length AS length,
       0            AS position_in_playlist,
       IF(D_S_T.Last_Aired_On = '0000-00-00 00:00:00', 0, 1) AS OOO,
       D_S_T.Last_Aired_On
FROM   $S_T AS D_S_T INNER JOIN $A_T AS D_A_T ON D_S_T.Song__ = D_A_T.Song__
WHERE  1 $FILTER
ORDER BY OOO, RAND(" . mt_rand (1,65536) . ")
LIMIT 1
";
    }
    else
    {
        $PLAYLISTS = implode (',', D_SQL_Quote_Array ($dbo, $playlists));
        $LOG->log ("Using playlists ($PLAYLISTS)", PEAR_LOG_DEBUG);
        
        $request = "
SELECT DISTINCT D_S_T.Song__   AS song,
       D_S_T.Length   AS length,
       D_P_T.Position AS position_in_playlist,
       IF(D_S_T.Last_Aired_On = '0000-00-00 00:00:00', 0, 1) AS OOO,
       D_S_T.Last_Aired_On
FROM   $P_T AS D_P_T LEFT JOIN $S_T AS D_S_T ON D_P_T.Song__ = D_S_T.Song__
            INNER JOIN $A_T AS D_A_T ON D_S_T.Song__ = D_A_T.Song__
WHERE  D_P_T.Playlist__ IN ($PLAYLISTS)
       $FILTER
ORDER BY OOO, RAND(" . mt_rand (1,65536) . ")
LIMIT 1
";
    }
    
    //echo "$request\n";
    return $dbo->query($request)->fetch (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Song_Get_Specific_Position ($dbo, $playlist, $position)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get song at specific position ($position)", PEAR_LOG_DEBUG);

    if (!is_numeric ($position))
    {
        $LOG->log ("No position !!", PEAR_LOG_DEBUG);
        $position = 1;
    }

    $S_T = $CONFIG['song_table'];
    $P_T = $CONFIG['playlist_songs_table'];
    
    $request = "
SELECT $S_T.Song__ AS song,
       $S_T.Length AS length,
       $P_T.Position AS position_in_playlist
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Playlist__ = $playlist AND $P_T.Position = $position
";

    return $dbo->query($request)->fetch (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Song_Get_Last_Position ($dbo, $playlist)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get song at last position of playlist $playlist",
               PEAR_LOG_DEBUG);

    $S_T = $CONFIG['song_table'];
    $P_T = $CONFIG['playlist_songs_table'];
    
    $request = "
SELECT $S_T.Song__ AS song,
       $S_T.Length AS length,
       $P_T.Position AS position_in_playlist
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Playlist__ = $playlist
ORDER BY $P_T.Position DESC
LIMIT 1
";
    
    return $dbo->query($request)->fetch (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Song_Get_Position_By_Order ($dbo, $playlist, $position, $order)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get song by order ($position, $order)", PEAR_LOG_DEBUG);
    if (!is_numeric ($position))
    {
        $LOG->log ("No position !!", PEAR_LOG_DEBUG);
        $position = 0;
    }

    $S_T = $CONFIG['song_table'];
    $P_T = $CONFIG['playlist_songs_table'];

    $OP = ($order > 0) ? '>' : '<';
    $OT = ($order > 0) ? 'ASC' : 'DESC';
    
    $request = "
SELECT $S_T.Song__ AS song,
       $S_T.Length AS length,
       $P_T.Position AS position_in_playlist
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Playlist__ = $playlist AND $P_T.Position $OP $position
ORDER BY Position $OT
LIMIT 1
";
    
    return $dbo->query($request)->fetch (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Update_Grid ($dbo, $grid_slot, $program)
{
    global $CONFIG;

    $request = "
UPDATE {$CONFIG['grid_table']}
SET Program__ = $program
WHERE Date_Beginning >= '" . D_Date ($grid_slot['start']) . "'
      AND Date_End <= '" . D_Date ($grid_slot['end']) . "'
      AND Program__ = {$grid_slot['program']}
";
    
    $dbo->query ($request);
}

/*****************************************************************************/

function D_SQL_Get_Previous_N_Songs ($dbo, $in_table, $songs, $playlists = false)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get previous " . ($in_table + count($songs)) . " songs.",
               PEAR_LOG_DEBUG);

    $S_T = $CONFIG['song_table'];
    $P_T = $CONFIG['playlist_songs_table'];

    $in_filter = array ();
    
    if ($in_table)
    {
        $request = "
SELECT $S_T.Song__
FROM   $S_T 
ORDER BY $S_T.Last_Aired_On DESC
LIMIT $in_table
";
        
        $result = $dbo->query ($request)->fetchAll (PDO::FETCH_COLUMN);
        $in_filter = array_merge ($in_filter, $result);
    }
    
    if ($songs)
    {
        $in_filter = array_merge ($songs, $in_filter);
    }

    if ($playlists)
    {
        $in_playlist = D_SQL_Get_Playlist_Songs ($dbo, $playlists, 0);
        $in_filter = array_intersect ($in_playlist, $in_filter);
    }

    return array_filter ($in_filter);
}

/*****************************************************************************/

function D_SQL_Get_Previous_Time_Songs ($dbo, $from_time, $songs, $playlists = false)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get played songs ($from_time).", PEAR_LOG_DEBUG);

    $S_T = $CONFIG['song_table'];

    $in_filter = array ();
    
    if ($from_time)
    {
        $request = "
SELECT $S_T.Song__
FROM   $S_T 
WHERE  $S_T.Last_Aired_On > '$from_time'
";

        $result = $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);
        $in_filter = array_merge ($in_filter, $result);
    }
    
    if ($songs)
    {
        $in_filter = array_merge ($in_filter, $songs);
    }

    if ($playlists)
    {
        $in_playlist = D_SQL_Get_Playlist_Songs ($dbo, $playlists, 0);
        $in_filter = array_intersect ($in_playlist, $in_filter);
    }

    return array_filter ($in_filter);
}

/*****************************************************************************/

function D_SQL_Get_Playlists ($dbo, $songs)
{
    global $CONFIG, $LOG;
    $LOG->log ("Get playlists.", PEAR_LOG_DEBUG);
    $P_T = $CONFIG['playlist_songs_table'];

    if (!is_array ($songs))
    {
        $songs = array ($songs);
    }

    $request = "
SELECT DISTINCT Playlist__ FROM $P_T WHERE Song__ IN(" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ')';

    return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);
}

/*****************************************************************************/

function D_SQL_Get_Artists ($dbo, $songs, $playlists)
{
    global $CONFIG, $LOG;
    $LOG->log ("Get artists.", PEAR_LOG_DEBUG);
    
    $A_T = $CONFIG['artists_songs_table'];
    $P_T = $CONFIG['playlist_songs_table'];
    
    if ($songs)
    {
        $request = "
SELECT DISTINCT $A_T.Artist__
FROM   $A_T 
WHERE  $A_T.Song__ IN(" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ')';

        if ($playlists && !in_array (0, $playlists))
        {
            $request .= 
" AND $A_T.Artist__ IN (SELECT Artist__ FROM $P_T LEFT JOIN $A_T USING(Song__) WHERE Playlist__ IN(" . implode (',', D_SQL_Quote_Array ($dbo, $playlists)) . '))';
        }
        return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);
    }

    // else
    return array ();
}

/*****************************************************************************/

function D_SQL_Get_Previous_N_Artists ($dbo, $in_table, $songs, $playlists)
{
    global $LOG;
    $LOG->log ("Get artists in previous " . ($in_table + count($songs)) . " songs =>",
               PEAR_LOG_DEBUG);
    
    $in_filter = D_SQL_Get_Previous_N_Songs ($dbo, $in_table, $songs);
    
    return D_SQL_Get_Artists ($dbo, $in_filter, $playlists);
}

/*****************************************************************************/

function D_SQL_Get_Previous_Time_Artists ($dbo, $from_time, $songs, $playlists)
{
    global $LOG;
    $LOG->log ("Get played artists ($from_time) =>", PEAR_LOG_DEBUG);
    
    $in_filter = D_SQL_Get_Previous_Time_Songs ($dbo, $from_time, $songs);
    return D_SQL_Get_Artists ($dbo, $in_filter, $playlists);
}

/*****************************************************************************/

function D_SQL_Get_Marked_Playlists ($dbo, $mark)
{
    global $CONFIG;
    $M_T = $CONFIG['marks_table'];

    $request = "
SELECT DISTINCT $M_T.Playlist__
FROM   $M_T
WHERE  $M_T.Mark = '$mark'
";
    
    return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);
}

/*****************************************************************************/

function D_SQL_Get_Playlist_Songs ($dbo, $playlists, $from_playlists)
{
    global $CONFIG, $LOG;
    $LOG->log ("Get playlist songs.", PEAR_LOG_DEBUG);
    $P_T = $CONFIG['playlist_songs_table'];

    if (!$playlists)
    {
        return array ();
    }

    if (!is_array ($playlists))
    {
        $playlists = array ($playlists);
    }
    
    $request = "
SELECT DISTINCT $P_T.Song__
FROM   $P_T
WHERE  $P_T.Playlist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $playlists)) . ")";

    if ($from_playlists && !in_array (0, $from_playlists))
    {
        $request .= "
       AND $P_T.Song__ IN (SELECT Song__ FROM $P_T WHERE Playlist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $from_playlists)) . '))';
    }
    
    return $dbo->query($request)->fetchAll(PDO::FETCH_COLUMN);
}

/*****************************************************************************/

function D_SQL_Get_Excluded_Playlist_Songs ($dbo, $excl_playlists, 
                                            $from, $songs, $playlists)
{
    global $LOG;
    $LOG->log ("Get excluded playlist songs.", PEAR_LOG_DEBUG);
    if (is_numeric ($from))
    {
        $songs = D_SQL_Get_Previous_N_Songs ($dbo, $from, $songs);
    }
    else
    {
        $songs = D_SQL_Get_Previous_Time_Songs ($dbo, $from, $songs);
    }

    if ($songs)
    {
        global $CONFIG;
        $P_T = $CONFIG['playlist_songs_table'];
        
        $request = "
SELECT DISTINCT $P_T.Playlist__
FROM   $P_T
WHERE  $P_T.Song__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ')';

        $lists = $dbo->query($request)->fetchAll(PDO::FETCH_COLUMN);
        

        $excl_playlists = array_intersect ($excl_playlists, $lists);
        
        if ($excl_playlists)
        {
            return D_SQL_Get_Playlist_Songs ($dbo, $excl_playlists, $playlists);
        }
    }

    // else
    return array ();
}

/*****************************************************************************/

function D_SQL_Get_Excluded_Criteria_Songs ($dbo, $criteria, $value,
                                            $from, $songs, $playlists)
{
    global $LOG;
    $LOG->log ("Get excluded criteria songs.", PEAR_LOG_DEBUG);
    if (is_numeric ($from))
    {
        $songs = D_SQL_Get_Previous_N_Songs ($dbo, $from, $songs);
    }
    else
    {
        $songs = D_SQL_Get_Previous_Time_Songs ($dbo, $from, $songs);
    }

    if ($songs)
    {
        global $CONFIG;
        $S_T = $CONFIG['song_table'];

        if (!$value)
        {
            $request = "
SELECT DISTINCT $S_T.$criteria
FROM   $S_T
WHERE  $S_T.Song__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ')';

            $values = $dbo->query($request)->fetchAll(PDO::FETCH_COLUMN);
        }
        else
        {
            $value= array ($value);
        }

        if ($values)
        {
            if ($playlists && !in_array(0, $playlists))
            {
                $request = "
SELECT $S_T.Song__
FROM   $S_T
WHERE  $S_T.$criteria IN (" . implode (',', D_SQL_Quote_Array ($dbo, $values)) . ')';
            }
            else
            {
                $P_T = $CONFIG['playlist_songs_table'];
                $request = "
SELECT $S_T.Song__
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $S_T.$criteria IN (" . implode (',', D_SQL_Quote_Array ($dbo, $values)) . ")
       AND $P_T.Playlist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $playlists)) . ')';
            }

            return $dbo->query($request)->fetchAll(PDO::FETCH_COLUMN);
        }
    }

    // else
    return array ();
}

/*****************************************************************************/

function D_SQL_Get_Criteria_Value ($dbo, $criteria, $song)
{
    global $CONFIG;
    global $LOG;
    $LOG->log ("Get criteria value.", PEAR_LOG_DEBUG);
    $S_T = $CONFIG['song_table'];

    $request = "
SELECT $S_T.$criteria
FROM   $S_T
WHERE  $S_T.Song__ = '$song'
LIMIT  1
";
    return $dbo->query($request)->fetchColumn ();
}

/*****************************************************************************/

function D_SQL_Get_Criteria_Sum ($dbo, $criteria, $n, $songs)
{
    global $CONFIG;
    global $LOG;
    $LOG->log ("Get criteria sum.", PEAR_LOG_DEBUG);
    $S_T = $CONFIG['song_table'];

    $songs = D_SQL_Get_Previous_N_Songs ($dbo, $n, $songs);

    if ($songs)
    {
        $request = "
SELECT SUM($S_T.$criteria)
FROM   $S_T
WHERE  $S_T.Song__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ')';

    	return mysql_result (mysql_query ($request), 0, 0);
        //return $dbo->query($request)->fetch (PDO::FETCH_COLUMN);
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************/

function D_SQL_Get_Count_Criteria ($dbo, $criteria, $number, $from, $songs, 
                                   $value = '', $operator = '=')
{
    if (!$songs && !$from) return array ();

    global $CONFIG;
    global $LOG;
    $LOG->log ("Get count criteria.", PEAR_LOG_DEBUG);
    $S_T = $CONFIG['song_table'];

    if (isset ($value) && $value <> '')
    {
        $VALUE = "AND $S_T.$criteria $operator '$value'";
    }
    
    if (is_numeric($from) or !$from)
    {
        if ($from)
        {
            $songs = D_SQL_Get_Previous_N_Songs ($dbo, $from, $songs);
        }
        $OR = '';
    }
    else
    {
        $OR = "OR $S_T.Last_Aired_On > '$from'";
    }
    
    $request = "
SELECT COUNT($S_T.Song__) AS s_count,
       $S_T.$criteria     AS criteria
FROM   $S_T
WHERE  ($S_T.Song__ IN(" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ")
       $OR)
       $VALUE
GROUP BY criteria
HAVING s_count >= $number
";

    return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN, 1);
}

/*****************************************************************************/

function D_SQL_Get_Count_Criteria_Global ($dbo, $criteria, $number, $from, $songs, 
                                          $value = '', $operator = '=')
{
    if (!$songs && !$from) return array ();

    global $CONFIG;
    global $LOG;
    $LOG->log ("Get count criteria global.", PEAR_LOG_DEBUG);
    $S_T = $CONFIG['song_table'];

    if (isset ($value) && $value <> '')
    {
        $VALUE = "AND $S_T.$criteria $operator '$value'";
    }
    
    if (is_numeric($from) or !$from)
    {
        if ($from)
        {
            $songs = D_SQL_Get_Previous_N_Songs ($dbo, $from, $songs);
        }
        $OR = '';
    }
    else
    {
        $OR = "OR $S_T.Last_Aired_On > '$from'";
    }
    
    $request = "
SELECT COUNT($S_T.Song__) AS s_count
FROM   $S_T
WHERE  ($S_T.Song__ IN(" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ")
       $OR)
       $VALUE
HAVING s_count >= $number
";

    return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN, 0);
}

/*****************************************************************************/

function D_SQL_Get_Count_Playlists ($dbo, $playlists, $number, $from, $songs, $inversed = false)
{
    if (!$songs && !$from) return array ();
    global $LOG;
    $LOG->log ("Get count playlist.", PEAR_LOG_DEBUG);

    global $CONFIG;
    $S_T = $CONFIG['song_table'];
    $P_T = $CONFIG['playlist_songs_table'];
    
    if (is_numeric($from) or !$from)
    {
        if ($from)
        {
            $songs = D_SQL_Get_Previous_N_Songs ($dbo, $from, $songs);
        }
        $OR = '';
    }
    else
    {
        $OR = "OR $S_T.Last_Aired_On > '$from'";
    }

    $NOT = $inversed ? 'NOT' : '';
    $request = "
SELECT COUNT($S_T.Song__) AS s_count,
       $P_T.Playlist__    AS playlist
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  ($S_T.Song__ IN(" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ")
        $OR)
       AND $P_T.Playlist__ $NOT IN (" . implode (',', D_SQL_Quote_Array ($dbo, $playlists)) . ")
GROUP BY playlist
HAVING s_count >= $number
";

    return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN, 1);
}

/*****************************************************************************/

function D_SQL_Program_Get_Percent_On_Air ($dbo, $program)
{
    global $CONFIG;

    $P_T = $CONFIG['played_songs_table'];
    $S_T = $CONFIG['song_table'];
    $week_length = 7 * 24 * 3600;
        
    $request = "
SELECT SUM($S_T.Length) / $week_length
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Program__ = $program
       AND $P_T.Tag = " . D_TAG_Song . "
       AND $P_T.Date > NOW() - INTERVAL 7 DAY
";
    
    $p = mysql_result (mysql_query ($request), 0, 0);
    //$p = $dbo->query($request)->fetchColumn ();
    return ($p ? $p : 100.0);
}

/*****************************************************************************/

function D_SQL_Program_Get_SubPlaylist_Count ($dbo, $in_playlists, $sub_playlist)
{
    global $CONFIG, $LOG;

    $LOG->log ("Get subplaylist count.", PEAR_LOG_DEBUG);

    $S_T = $CONFIG['song_table'];
    $P_T = $CONFIG['playlist_songs_table'];
    
    if (array_key_exists ('criteria', $in_playlists))
    {
        if (is_array ($sub_playlist))
        {
            $request = "
SELECT COUNT($S_T.Song__)
FROM   $S_T
WHERE  $S_T.{$in_playlists['criteria']} = '{$in_playlists['value']}'
       AND $S_T.{$sub_playlist['criteria']} = '{$sub_playlist['value']}'
";
        }
        else
        {
            $request = "
SELECT COUNT($P_T.Song__)
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Playlist__ = $sub_playlist
       AND $S_T.{$in_playlists['criteria']} = '{$in_playlists['value']}'
";
        }
    }
    else
    {
        if (is_array ($sub_playlist))
        {
            $request = "
SELECT COUNT($P_T.Song__)
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Playlist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $in_playlists)) . ")
       AND $S_T.{$sub_playlist['criteria']} = '{$sub_playlist['value']}'
";
        }
        else
        {
            $request = "
SELECT COUNT($P_T.Song__)
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Playlist__ = $sub_playlist
       AND $P_T.Song__ IN (SELECT Song__ FROM $P_T
                           WHERE $P_T.Playlist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $in_playlists)) . "))
";
        }
    }

    return $dbo->query($request)->fetchColumn ();
}

/*****************************************************************************/

function D_SQL_Get_Average_Song_Length ($dbo, $playlists, $from = false)
{
    global $CONFIG;
    $S_T = $CONFIG['song_table'];
    $P_T = $CONFIG['playlist_songs_table'];

    if ($playlists && !is_array ($playlists))
    {
        $playlists = array ($playlists);
    }
    
    if (!$playlists || in_array (0, $playlists))
    {
        $request = "
SELECT AVG(Length) FROM $S_T
";
    }
    else if (array_key_exists ('criteria', $playlists))
    {
        $request = "
SELECT AVG(Length)
FROM   $S_T
WHERE  $S_T.{$playlists['criteria']} = '{$playlists['value']}'
";
    }
    else
    {
        $request = "
SELECT AVG($S_T.Length)
FROM   $P_T LEFT JOIN $S_T ON $P_T.Song__ = $S_T.Song__
WHERE  $P_T.Playlist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $playlists)) . ')';
    }

    return mysql_result (mysql_query ($request), 0, 0);
    //return $dbo->query($request)->fetchColumn ();
}

/*****************************************************************************/

function D_SQL_Has_Tags ($dbo, $tags, $limit, $note, $full = false)
{
    global $CONFIG;
    
    $P_T = $CONFIG['full_played_table'];

    $request = "
SELECT COUNT($P_T.Played__)
FROM   $P_T
WHERE  $P_T.Tag IN (" . implode (',', D_SQL_Quote_Array ($dbo, $tags)) . ")
       AND $P_T.Note LIKE '%$note%'
";
    if (is_numeric ($limit))
    {
        if ($full)
        {
            $request .= "AND $P_T.Played__ > (SELECT MAX($P_T.Played__) FROM $P_T) - $limit";
        }
        else
        {
            $req = "SELECT $P_T.Played__ FROM $P_T WHERE $P_T.Tag NOT IN (" . D_TAG_Link . ',' . D_TAG_RequestLink . ") AND $P_T.Played__ > (SELECT MAX(Played__) - 2*$limit FROM $P_T) ORDER BY $P_T.Played__ DESC LIMIT $limit";
            $played = $dbo->query($req)->fetchAll (PDO::FETCH_COLUMN);
            
            $request .= "AND $P_T.Played__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $played)) . ')';
        }
    }
    else
    {
        $request .= "AND $P_T.Date > '$limit'";
    }

    return $dbo->query($request)->fetchColumn ();
}

/*****************************************************************************/

function D_SQL_Mark_Request ($dbo, $requests, $done)
{
    if ($requests)
    {
        global $CONFIG;
        $R_T = $CONFIG['requests_table'];
        
        $request = "
UPDATE $R_T SET Done = $done WHERE Request__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $requests)) . ')';
        $dbo->query ($request);
    }    
}

/*****************************************************************************/

function D_SQL_Mark_Request_Done ($dbo, $requests)
{
    D_SQL_Mark_Request ($dbo, $requests, 1);
}

/*****************************************************************************/

function D_SQL_Mark_Request_Not_Done ($dbo, $requests)
{
    D_SQL_Mark_Request ($dbo, $requests, 0);
}

/*****************************************************************************/

function D_SQL_Program_Has_Jingle_Type ($dbo, $program, $type)
{
    global $CONFIG;
    $L_T = $CONFIG['links_table'];
    
    $request = "
SELECT COUNT($L_T.Entry__)
FROM   $L_T
WHERE  $L_T.Linked__ = $program
       AND $L_T.Type = '$type'
       AND $L_T.Weight <> 0
";
    return $dbo->query($request)->fetchColumn ();
}

/*****************************************************************************/

function D_SQL_Program_Has_Start_Jingle ($dbo, $program)
{
    return D_SQL_Program_Has_Jingle_Type ($dbo, $program, 'PROGRAM_BEGIN');
}

/*****************************************************************************/

function D_SQL_Program_Has_End_Jingle ($dbo, $program)
{
    return D_SQL_Program_Has_Jingle_Type ($dbo, $program, 'PROGRAM_END');
}

/*****************************************************************************/

function D_SQL_Program_Has_Prog_Jingle ($dbo, $program)
{
    return D_SQL_Program_Has_Jingle_Type ($dbo, $program, 'PROGRAM');
}

/*****************************************************************************/

function D_SQL_Get_Jingle ($dbo, $linked, $type, $margin, $time, $j = false)
{
    if (!$linked) return false;

    global $CONFIG;
    $L_T = $CONFIG['links_table'];
    $J_T = $CONFIG['jingle_table'];

    if (!is_array ($type))
    {
        $type = array ($type);
        $type = array_map ('D_Quote', $type);
    }

    if (!is_array ($linked))
    {
        $linked = array ($linked);
    }

    $dayparted = D_SQL_Get_Dayparted_Songs ($dbo, $time, array ());
    $jnot = $dayparted['songs'];
    if (is_array ($j))
    {
        $jnot = array_merge ($jnot, $j);
    }
    $JNOT = '';
    if ($jnot)
    {
        $JNOT = "AND $L_T.Link__ NOT IN (" . implode (',', D_SQL_Quote_Array ($dbo, $jnot)) . ')';
    }

    $request = "
SELECT $L_T.Link__ AS song,
       $J_T.Length AS length,
       0           AS position_in_playlist
FROM   $L_T LEFT JOIN $J_T ON $L_T.Link__ = $J_T.Song__
WHERE  $L_T.Linked__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $linked)) . ")
       AND $L_T.Type IN (" . implode (',', $type) . ")
       AND $J_T.Last_Aired_On < '$time' - INTERVAL $margin SECOND
       AND $L_T.Weight <> 0
       $JNOT
ORDER BY $L_T.Weight * (UNIX_TIMESTAMP('$time') - IF($J_T.Last_Aired_On = '0000-00-00 00:00:00', 0, UNIX_TIMESTAMP($J_T.Last_Aired_On))) * RAND() DESC
LIMIT 1
";

    return $dbo->query($request)->fetch (PDO::FETCH_ASSOC);
}

/*****************************************************************************/

function D_SQL_Last_Is_Link ($dbo)
{
    global $CONFIG;
    $P_T = $CONFIG['full_played_table'];
    
    $request = "SELECT $P_T.Tag FROM $P_T ORDER BY $P_T.Played__ DESC LIMIT 1";
    $tag = $dbo->query($request)->fetch (PDO::FETCH_ASSOC);
    return D_Is_Link ($tag);
}
    
/*****************************************************************************/

function D_Build_Daypart_Where ($time)
{
    if (!is_numeric ($time))
    {
        $time = strtotime ($time);
    }
    
    $DAYS = array (0 => "'SUNDAY'",
                   1 => "'MONDAY'",
                   2 => "'TUESDAY'",
                   3 => "'WEDNESDAY'",
                   4 => "'THURSDAY'",
                   5 => "'FRIDAY'",
                   6 => "'SATURDAY'");
    $day = intval (date('w', $time));
    $TIME = D_Time ($time);

    global $CONFIG;
    $Parts_T = $CONFIG['dayparts_table'];
    
    $WHERE = "$Parts_T.Begin_Time <= '$TIME' AND '$TIME' <= $Parts_T.End_Time";
    
    $Daypart_Periods = array ("'WEEK'");
    $Daypart_Periods[] = $DAYS[$day];
    if ($day == 0 || $day == 6)
    {
        $Daypart_Periods[] = "'WEEKEND'";
    }
    else
    {
        $Daypart_Periods[] = "'WORKDAYS'";
    }

    $WHERE .= " AND $Parts_T.Day IN (" . implode (',', $Daypart_Periods) . ')';

    return $WHERE;
}


/*****************************************************************************/

function D_SQL_Get_Dayparted_Songs ($dbo, $time, $from_playlists)
{
    global $CONFIG, $LOG; 
    $LOG->log ("Get subplaylist count.", PEAR_LOG_DEBUG);
    $Parts_T = $CONFIG['dayparts_table'];

    $WHERE = D_Build_Daypart_Where ($time);

    $request = "
SELECT $Parts_T.Excluded__
FROM   $Parts_T
WHERE  $Parts_T.What = 'SONG' AND $WHERE
";
    $songs = $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);

    $request = "
SELECT $Parts_T.Excluded__
FROM   $Parts_T
WHERE  $Parts_T.What = 'PLAYLIST' AND $WHERE
";
    $playlists = $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);
    $songs = array_merge ($songs,
                          D_SQL_Get_Playlist_Songs ($dbo,
                                                    $playlists,
                                                    $from_playlists));

    $request = "
SELECT $Parts_T.Excluded__
FROM   $Parts_T
WHERE  $Parts_T.What = 'ARTIST' AND $WHERE
";
    $artists = $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);

    return array ('songs' => $songs, 'artists' => $artists);
}

/*****************************************************************************/

function D_SQL_Is_Dayparted ($dbo, $what, $type, $time)
{
    global $CONFIG;
    $Parts_T = $CONFIG['dayparts_table'];

    $WHERE = D_Build_Daypart_Where ($time);

    $request = "
SELECT COUNT($Parts_T.Excluded__)
FROM   $Parts_T
WHERE  $Parts_T.What = '$type' AND $Parts_T.Excluded__ = $what AND $WHERE
";
    return $dbo->query($request)->fetchColumn ();
}

/*****************************************************************************/

function D_SQL_Load_Commands ($dbo)
{
    global $CONFIG;
    $C_T = $CONFIG['commands_table'];
    $request = "SELECT Command FROM $C_T";
    $commands = $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);

    if ($commands)
    {
        $dbo->query("DELETE FROM $C_T");
    }

    return $commands;
}

/*****************************************************************************/

function D_SQL_Get_Criteria_For_Songs ($dbo, $songs, $criteria)
{
    if (!$songs) return array ();
    
    global $CONFIG, $LOG;
    $LOG->log ("Get criteria for songs.", PEAR_LOG_DEBUG);
    $S_T = $CONFIG['song_table'];

    $request = "
SELECT Song__, `$criteria`
FROM   $S_T
WHERE  Song__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ")
";
    $c = $dbo->query($request)->fetchAll ();
    $rr = array ();
    foreach ($c as $r)
    {
        $rr[$r[0]] = $r[1];
    }

    $result = array ();
    foreach ($songs as $song)
    {
        $result[] = $rr[$song];
    }
    return $result;
}

/*****************************************************************************/

function D_SQL_Get_Other_Songs_For_Artists ($dbo, $artists)
{
    if (!$artists) return array ();
    global $CONFIG, $LOG;
    $LOG->log ("Get other songs for artists.", PEAR_LOG_DEBUG);
    $A_T = $CONFIG['artists_songs_table'];

    $request = "SELECT Song__ FROM $A_T WHERE Artist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $artists)) . ")";
    $songs = $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);

    $request = "
SELECT Song__ 
FROM   $A_T AS D_A_T
WHERE      D_A_T.Artist__ NOT IN (" . implode (',', D_SQL_Quote_Array ($dbo, $artists)) . ")
       AND Song__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $songs)) . ")
";
    return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);
}

/*****************************************************************************/

function D_SQL_Get_Songs_For_Artists ($dbo, $artists, $playlists)
{
    if (!$artists) return array ();
    global $CONFIG, $LOG;
    $LOG->log ("Get songs for artists.", PEAR_LOG_DEBUG);
    $A_T = $CONFIG['artists_songs_table'];

    $request = "
SELECT Song__
FROM   $A_T
WHERE  Artist__ in (". implode (',', D_SQL_Quote_Array ($dbo, $artists)) . ")
";
    if ($playlists)
    {
        $P_T = $CONFIG['playlist_songs_table'];
        $request .= 
" AND Song__ IN (SELECT Song__ FROM $P_T WHERE Playlist__ IN (" . implode (',', D_SQL_Quote_Array ($dbo, $playlists)) . ")";
    }

    return $dbo->query($request)->fetchAll (PDO::FETCH_COLUMN);
}

/*****************************************************************************/

?>
