<?php // $Id: main.php 48 2007-03-26 13:53:50Z mouke $

/*****************************************************************************/

$VERSION = '1.0';

/*****************************************************************************/

set_time_limit(0);

/*****************************************************************************/

require_once ('Log.php');
require_once ('messages.inc');
require_once ('config.inc');

if ($CONFIG['log_file'])
{
    $LOG = &Log::singleton ('file', $CONFIG['log_file']);
}
else
{
    $LOG = &Log::singleton ('console');
}

if ($argv[1] == 'debug')
{
    $MASK = PEAR_LOG_DEBUG;
}
else
{
    $MASK = PEAR_LOG_INFO;
}
$LOG->setMask (Log::UPTO ($MASK));

/*****************************************************************************/

define (D_TAG_Song,        1);
define (D_TAG_Event,       2);
define (D_TAG_Rotated,     3);
define (D_TAG_Link,        4);
define (D_TAG_RequestLink, 5);
define (D_TAG_Panic_Song,  6);

define (D_TYPE_Song,   1);
define (D_TYPE_Jingle, 2);
define (D_TYPE_Liner,  3);
define (D_TYPE_Promo,  4);
define (D_TYPE_Live,   5);

/*****************************************************************************/

require_once ('utils.inc');
require_once ('sql.inc');
require_once ('decider_config_management.inc');
require_once ('main_loop.inc');

/*****************************************************************************/

$DECIDER_STATE = array ();
$DECIDER_STATE['iteration'] = 0;
$DECIDER_STATE['song_iteration'] = 0;
$DECIDER_STATE['running'] = true;

while ($DECIDER_STATE['running'])
{
    $LOG->log ('', PEAR_LOG_DEBUG);
    $LOG->log ("Iteration {$DECIDER_STATE['iteration']}", PEAR_LOG_DEBUG);
    try
    {
        mysql_connect ('localhost', $CONFIG['user'], $CONFIG['password']);
        mysql_select_db ($CONFIG['base']);
        $pdo = new PDO ($CONFIG['driver'],
                        $CONFIG['user'],
                        $CONFIG['password']);
        $pdo->setAttribute (PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

        $DECIDER_STATE['db'] = $pdo;

        D_Check_Config ($DECIDER_STATE);

        D_Main_Loop ($DECIDER_STATE);

        if ($DECIDER_STATE['config']['commands']['stop'] == true)
        {
            $LOG->log (sprintf ($MSGS['K_COMMAND'], 'stop'));
            $DECIDER_STATE['running'] = false;
            continue;
        }

	mysql_close ();
        D_Wait_Before_Iteration ($DECIDER_STATE);
        
        ++$DECIDER_STATE['iteration'];

        $DECIDER_STATE['db'] = false;
        $pdo = false;
    }
    catch (PDOException $e)
    {
        $trace = $e->getTrace();
        print_r($trace[2]);
        print_r($trace[1]);
        print_r($trace[0]);
        
        printf ($MSGS['K_DB_FAILED'],
                $e->getFile(),
                $e->getLine(),
                $e->getMessage());
        die ();
        D_Wait_Before_Iteration ($DECIDER_STATE);
    }
}

D_Close_Decider ();

/*****************************************************************************/

/**
 * D_Close_Decider 
 * 
 * @access public
 * @return void
 */
function D_Close_Decider ()
{
    // Nothing special to do
}

/*****************************************************************************/

/**
 * D_Wait_Before_Iteration 
 * 
 * @param mixed $decider 
 * @access public
 * @return void
 */
function D_Wait_Before_Iteration ($decider)
{
    //printf ('DECIDER_STATE: ');
    //print_r ($decider);
    
    sleep ($decider['config']['sleep_time']);
}

/*****************************************************************************/

?>
