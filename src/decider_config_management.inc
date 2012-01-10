<?php // $Id: decider_config_management.inc 57 2007-05-04 08:48:54Z mouke $

/*****************************************************************************/

function D_Check_Config (& $decider)
{
    if (!isset ($decider['config']))
    {
        $decider['config'] = array ();
    }

    D_Load_Config ($decider['config'], $decider['db']);
    
    D_Load_Commands ($decider['config'], $decider['db']);

    if ($decider['config']['commands']['debug'] == true)
    {
        global $LOG;
        $LOG->setMask (Log::UPTO (PEAR_LOG_DEBUG));
    }
    if ($decider['config']['commands']['nodebug'] == true)
    {
        global $LOG;
        $LOG->setMask (Log::UPTO (PEAR_LOG_INFO));
    }

    if ($decider['config']['commands']['reload'] == true
        || $decider['iteration'] % $decider['config']['iteration_reload'] == 0)
    {
        if ($decider['config']['commands']['reload'])
        {
	        global $LOG, $MSGS;
            $LOG->log (sprintf ($MSGS['K_COMMAND'], 'reload'));
        }
        $decider['config']['rules'] = array ();
        D_Load_Rules_Configuration($decider['config']['rules'],
                                   $decider['db']);

        $decider['config']['programs'] = array ();
        D_Load_Programs_Configuration ($decider['config']['programs'],
                                       $decider['config']['default_links'],
                                       $decider['db']);
    }
}

/*****************************************************************************/

function D_Load_Config (& $config, $dbo)
{
    global $CONFIG;
    $config['queue_size']              = $CONFIG['default_queue_size'];
    $config['prog_margin']             = $CONFIG['default_prog_margin'];
    $config['crash_margin']            = $CONFIG['default_crash_margin'];
    $config['event_margin']            = $CONFIG['default_event_margin'];
    $config['date_margin']             = $CONFIG['default_date_push_margin'];
    $config['grid_default']            = $CONFIG['default_grid_prog'];
    $config['sleep_time']              = $CONFIG['default_sleep_time'];
    $config['iteration_reload']        = $CONFIG['default_iteration_reload'];
    $config['length_correction']       = $CONFIG['default_length_correction'];
    $config['use_requests']            = $CONFIG['default_use_requests'];
    $config['default_links']           = $CONFIG['default_links'];
    $config['enforce_rules_on_change'] = $CONFIG['enforce_rules_on_change'];
    $config['event_no_iteration']      = $CONFIG['event_no_iteration'];

    $config['change_hook']             = $CONFIG['change_hook'];
}

/*****************************************************************************/

function D_Load_Commands (& $config, $dbo)
{
    $config['commands'] = array ();
    $commands = D_SQL_Load_Commands ($dbo);
    foreach ($commands as $command)
    {
        $config['commands'][$command] = true;
    }
}

/*****************************************************************************/

require_once ('rules_parser.inc');

function D_Load_Rules_Configuration (& $rules, $dbo)
{
    global $CONFIG;
    foreach ($CONFIG['default_rules'] as $rule)
    {
        $r = D_Parse_Rule ($rule);
        if ($r)
        {
            $rules[0][] = $r;
        }
    }

    foreach (D_SQL_Load_Rules ($dbo) as $rule)
    {
        $r = D_Parse_Rule ($rule['rule']);
        if ($r)
        {
            if (array_key_exists (0, $r))
            {
                // Une "règle" en a généré plusieurs
                foreach ($r as $ru)
                {
                    $rules[$rule['policy']][] = $ru;
                }
            }
            else
            {
                $rules[$rule['policy']][] = $r;
            }
        }
    }
}

/*****************************************************************************/

require_once ('schema_parsers.inc');
require_once ('events_parser.inc');
require_once ('links_parser.inc');

/**
 *  Loads all the program definitions and parse their schema.
 */
function D_Load_Programs_Configuration (& $programs, $default_links, $dbo)
{
    global $MSGS, $DECIDER_SCHEMA_PARSERS, $LOG;

    $LOG->log ($MSGS['K_PRG_LOAD_CONF']);
    
    $programs = D_SQL_Load_Programs ($dbo);

    foreach ($programs as $prg => &$program)
    {
        // 1. Get schema
        if (isset ($DECIDER_SCHEMA_PARSERS[$program['type']]))
        {
            // 1. Get schema
            $parser = $DECIDER_SCHEMA_PARSERS[$program['type']];
            $program['prog_schema'] = $parser (str_replace("\r", "", $program['prog_schema']), $prg, $dbo);
        }
        else
        {
            $LOG->log (sprintf ($MSGS['K_PRG_UNKNOWN_TYPE'],
                                $program['type'], 
                                $prg),
                       PEAR_LOG_ERR);
            $program['prog_schema'] = false;
        }

        // 2. Events
        $program['events'] = D_Parse_Events (str_replace("\r", "", $program['events']));

        // 3. Links
        if (!$program['links_schema'])
        {
            $program['links_schema'] = $default_links;
        }
        $program['links_schema'] = D_Parse_Links (str_replace("\r", "", $program['links_schema']), $prg, $dbo);
    }
}

/*****************************************************************************/

?>
