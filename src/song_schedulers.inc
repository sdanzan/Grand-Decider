<?php // $Id: song_schedulers.inc 3 2006-07-13 21:23:43Z mouke $

$DECIDER_SONG_SCHEDULERS['RANDOM']            = "D_Song_Scheduler_RANDOM";
$DECIDER_SONG_SCHEDULERS['STRUCTURED_RANDOM'] = "D_Song_Scheduler_STRUCTURED_RANDOM";
$DECIDER_SONG_SCHEDULERS['ORDERED']           = "D_Song_Scheduler_ORDERED";
$DECIDER_SONG_SCHEDULERS['CYCLE']             = "D_Song_Scheduler_CYCLE";
$DECIDER_SONG_SCHEDULERS['META']              = "D_Song_Scheduler_META";

require_once ('song_scheduler_random.inc');
require_once ('song_scheduler_structured_random.inc');
require_once ('song_scheduler_ordered.inc');
require_once ('song_scheduler_cycle.inc');
require_once ('song_scheduler_meta.inc');

?>
