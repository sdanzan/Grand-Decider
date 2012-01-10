<?php // $Id: song_scheduler_meta.inc 5 2006-07-14 22:33:25Z mouke $

function D_Song_Scheduler_META ($grid_slot, $iteration, $queue, $config, $dbo)
{
    global $CONFIG, $MSGS, $LOG;
    $program = $grid_slot['program'];
    $programs = $config['programs'][$program]['prog_schema']['programs'];
    $percent = $config['programs'][$program]['prog_schema']['percent'];

    $random = mt_rand (1, 100);
    $i = 0;
    $c = 0;
    foreach ($percent as $per)
    {
        $c += $per;
        if ($random <= $c)
        {
           break;
        }
        ++$i;
    }

    $p = $programs[$i];
    D_SQL_Update_Grid ($dbo, $grid_slot, $p);
    $LOG->log (sprintf ($MSGS['K_SCHEDULER_META'], $p));
    
    return array ('grid_change' => true);
}

?>
