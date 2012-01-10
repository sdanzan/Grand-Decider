<?php // $Id: config.inc 47 2007-03-16 16:47:34Z mouke $

// Fall back defaults
$CONFIG['default_playlist']          = 0;
$CONFIG['default_program']           = 1; // For meta
$CONFIG['default_grid_prog']         = 1;
$CONFIG['default_queue_size']        = 20;
$CONFIG['default_crash_margin']      = 5; // minutes
$CONFIG['default_prog_margin']       = 90; // seconds
$CONFIG['default_event_margin']      = 121; // seconds
$CONFIG['default_date_push_margin']  = 60; // seconds
$CONFIG['default_sleep_time']        = 31; // seconds
$CONFIG['default_iteration_reload']  = 10;
$CONFIG['default_policy']            = 0;
$CONFIG['panic_playlist']            = 96;
$CONFIG['default_rotation_block']    = 100;
$CONFIG['default_length_correction'] = 2;
$CONFIG['default_use_requests']      = 1;
$CONFIG['enforce_rules_on_change']   = true;
$CONFIG['event_no_iteration']        = false;
$CONFIG['meta_default_separation']   = '2 DAY';
//$CONFIG['log_file']                  = '/var/log/decider.log';

// Database access
$CONFIG['driver']   = '';
$CONFIG['user']     = 'user';
$CONFIG['password'] = 'password';
require_once ('dbconfig.inc');

// Database tables and columns
$CONFIG['programs_table']       = 'Program';
$CONFIG['queue_table']          = 'Queue';
$CONFIG['now_table']            = 'Now';
$CONFIG['grid_table']           = 'Grid';
$CONFIG['rules_table']          = 'D_Rules';
$CONFIG['marks_table']          = 'D_Playlist_Marks';
$CONFIG['full_library_table']   = 'Song';
$CONFIG['playlist_songs_table'] = 'D_Playlist_Songs';
$CONFIG['song_table']           = 'D_Songs';
$CONFIG['jingle_table']         = 'D_Jingles';
$CONFIG['links_table']          = 'D_Links';
//$CONFIG['artists_songs_table']  = 'D_Songs';
$CONFIG['artists_songs_table']  = 'D_Artists_Songs';
$CONFIG['played_songs_table']   = 'Played';
$CONFIG['played_jingles_table'] = 'Played';
$CONFIG['full_played_table']    = 'Played';
$CONFIG['requests_table']       = 'Request';
$CONFIG['dayparts_table']       = 'D_Dayparts';
$CONFIG['commands_table']       = 'D_Commands';

// Default rules
$CONFIG['default_rules'] = array (
    'SEPARATION ARTIST 3 HOUR',
    'SEPARATION SONG 6 HOUR',
    );

// Default links
$CONFIG['default_links'] = "
ALL LINKS ON
SPECIFIC LINKS OFF
FREQUENCY [5-7 SONG]
ITERATION STEP 1
STRICT LINKS
FORCE BEFORE EVENTS
ROTATION MARGIN [45 MINUTE]
";

?>
