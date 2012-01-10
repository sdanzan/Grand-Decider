<?php // $Id: rules_parser.inc 56 2007-05-03 14:40:55Z mouke $

/*****************************************************************************/

$RULE_DEFS = array (
    'D_Rule_Parse_Separation' =>
    '/^\s*SEPARATION\s+(ARTIST|SONG|PLAYLISTS\s+MARKED\s+\[([A-Za-z0-9_]+)\]|PLAYLIST\s+\[([0-9]+)\]|CRITERIA\s+\[([A-Za-z0-9_]+)\]|CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+\[([A-Za-z0-9_]+)\])\s+([0-9]+)\s+(HOUR|MINUTE|DAY|SONG)\s*$/',

    'D_Rule_Parse_Sequence_Control_Criteria_Delta' =>
    '/^\s*SEQUENCE\s+CONTROL\s+DELTA\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+(<|>|>=|<=|=)\s+([0-9]+)\s*$/',

    'D_Rule_Parse_Sequence_Control_Criteria_Difference' =>
    '/^\s*SEQUENCE\s+CONTROL\s+DIFFERENCE\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+(<|>|>=|<=|=)\s+([0-9\-]+)\s*$/',

    'D_Rule_Parse_Sequence_Control_Criteria_Sum' =>
    '/^\s*SEQUENCE\s+CONTROL\s+SUM\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+(<|>|>=|<=)\s+([0-9]+)\s+OVER\s+([0-9]+)\s+SONG\s*$/',

    'D_Rule_Parse_Sequence_Control_Criteria_Number' =>
    '/^\s*SEQUENCE\s+CONTROL\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+(< |> |<> |= |)\s*\[([A-Za-z0-9_]+)\]\s+([0-9]+)\s+OVER\s+([0-9]+)\s+(SONG|HOUR|MINUTE|DAY)\s*$/',

    'D_Rule_Parse_Sequence_Control_Criteria_Multi_Number' =>
    '/^\s*SEQUENCE\s+CONTROL\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+([0-9]+)\s+OVER\s+([0-9]+)\s+(SONG|HOUR|MINUTE|DAY)\s*$/',

    'D_Rule_Parse_Sequence_Control_Criteria_Sawtooth' =>
    '/^\s*SEQUENCE\s+CONTROL\s+SAWTOOTH\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+THRESHOLD\s+([0-9]+)\s*$/',
    
    'D_Rule_Parse_Sequence_Control_Criteria_Peak' =>
    '/^\s*SEQUENCE\s+CONTROL\s+PEAK\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+THRESHOLD\s+([0-9]+)\s*$/',

    'D_Rule_Parse_Sequence_Control_Playlist_Number' =>
    '/^\s*SEQUENCE\s+CONTROL\s+PLAYLIST\s+(NOT\s+|)\[([0-9]+)\]\s+([0-9]+)\s+OVER\s+([0-9]+)\s+(SONG|HOUR|MINUTE|DAY)\s*$/',

    'D_Rule_Parse_Sequence_Control_Playlists_Number' =>
    '/^\s*SEQUENCE\s+CONTROL\s+PLAYLISTS\s+(NOT\s+|)MARKED\s+\[([A-Za-z0-9_]+)\]\s+([0-9]+)\s+OVER\s+([0-9]+)\s+(SONG|HOUR|MINUTE|DAY)\s*$/',

    'D_Rule_Parse_Sequence_Operator_Criteria' =>
    '/^\s*SEQUENCE\s+(FORBID|FORCE)\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+(< |> |<= |>= |<> |= |)\s*\[([A-Za-z0-9_]+)\]\s+AFTER\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+VALUE\s+(< |> |<> |= |)\s*\[([A-Za-z0-9_]+)\](\s+([0-9]+)%){0,1}\s*$/',

    'D_Rule_Parse_Sequence_Operator_Playlist' =>
    '/^\s*SEQUENCE\s+(FORBID|FORCE)\s+PLAYLIST\s+\[([0-9]+)\]\s+AFTER\s+PLAYLIST\s+\[([0-9]+)\](\s+([0-9]+)%){0,1}\s*$/',
    
    'D_Rule_Parse_Sequence_Playlist_Series' =>
    '/^\s*SEQUENCE\s+(FORBID|FORCE)\s+PLAYLIST\s+\[([0-9]+)\]\s+SERIES\s*$/',
    
    'D_Rule_Parse_Sequence_Playlist_Series_Limit' =>
    '/^\s*SEQUENCE\s+FORCE\s+PLAYLIST\s+\[([0-9]+)\]\s+SERIES\s+LIMIT\s+([0-9]+)\s*$/',

    'D_Rule_Parse_Control_Criteria' =>
    '/^\s*CONTROL\s+CRITERIA\s+\[([A-Za-z0-9_]+)\]\s+(>|<|>=|<=|=|<>)\s+([A-Za-z0-9_]+)\s*$/',
    
    'D_Rule_Parse_Dayparts' =>
    '/^\s*(SMART\s*([0-9]+)|SONG)\s+DAYPARTS(\s+OVER\s*([0-9]+)\s+DAYS|)\s*$/',

    'D_Rule_Parse_Control_Length' =>
    '/^\s*SMART\s+LENGTH\s+CONTROL\s*$/',

    'D_Rule_Parse_Ignore' =>
    '/^\s*IGNORE\s+DEFAULTS\s*$/',
    
    );

/*****************************************************************************/

function D_Parse_Rule ($rule_string)
{
    if (!preg_match ('/^(\s*|\s*#.*)$/', $rule_string))
    {
        global $LOG, $MSGS, $RULE_DEFS;
    
        $tokens = array ();

        foreach ($RULE_DEFS as $parser => $rule_def)
        {    
            if (preg_match ($rule_def, $rule_string, $tokens))
            {
                $rule = $parser ($tokens);
                if (array_key_exists (0, $rule))
                {
                    // Multi rule
                    foreach ($rule as &$r)
                    {
                        $r['rule_def'] = $tokens[0];
                    }
                }
                else
                {
                    $rule['rule_def'] = $tokens[0];
                }
                return $rule;
            }
        }

        if (!preg_match ('/^\s*#/', $rule_string))
        {
            $LOG->log (sprintf ($MSGS['K_RULE_SYNTAX_ERROR'], $rule_string),
                       PEAR_LOG_ERR);
        }
    }
    
    return false;
}

/*****************************************************************************/

function D_Get_Time_Or_Song (& $rule, $type, $token)
{
    switch ($type)
    {
        case 'HOUR':
            $rule['time'] = $token * 60 * 60;
            break;

        case 'MINUTE':
            $rule['time'] = $token * 60;
            break;

        case 'DAY':
            $rule['time'] = $token * 24 * 60 * 60;
            break;

        case 'SONG':
            $rule['song'] = $token - 1;
            break;
    }
}

/*****************************************************************************/

function D_Rule_Parse_Separation ($tokens)
{
    $rule = array ();

    if ($tokens[2])
    {
        $rule['type'] = 'separation_playlists';
        $rule['mark'] = $tokens[2];
    }
    if ($tokens[3])
    {
        $rule['type'] = 'separation_playlist';
        $rule['playlist'] = $tokens[3];
    }
    if ($tokens[4])
    {
        // separation_criteria_multi
        $newtokens[0] = $tokens[0];
        $newtokens[1] = $tokens[4];
        $newtokens[2] = 1;
        $newtokens[3] = ($tokens[8] == 'SONG') ? ($tokens[7] + 1) : $tokens[7];
        $newtokens[4] = $tokens[8];
        return D_Rule_Parse_Sequence_Control_Criteria_Multi_Number ($newtokens);
    }
    if ($tokens[5])
    {
        // separation_criteria
        $newtokens[0] = $tokens[0];
        $newtokens[1] = $tokens[5];
        $newtokens[2] = '= ';
        $newtokens[3] = $tokens[6];
        $newtokens[4] = 1;
        $newtokens[5] = ($tokens[8] == 'SONG') ? ($tokens[7] + 1) : $tokens[7];
        $newtokens[6] = $tokens[8];
        return D_Rule_Parse_Sequence_Control_Criteria_Number ($newtokens);
    }
    
    switch ($tokens[1])
    {
        case 'SONG':
            $rule['type'] = 'separation_song';
            break;

        case 'ARTIST':
            $rule['type'] = 'separation_artist';
            break;
    }

    D_Get_Time_Or_Song ($rule, $tokens[8], $tokens[7]);
    
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Criteria_Difference ($tokens)
{
    return D_Rule_Parse_Sequence_Control_Criteria_Delta ($tokens, true);
}

function D_Rule_Parse_Sequence_Control_Criteria_Delta ($tokens, $signed = false)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_criteria_delta';
    $rule['criteria_1'] = $tokens[1];
    $rule['criteria_2'] = $tokens[1];
    $rule['operator']   = $tokens[2];
    $rule['value']      = $tokens[3];
    $rule['signed']     = $signed;
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Criteria_Sawtooth ($tokens)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_criteria_sawtooth';
    $rule['criteria'] = $tokens[1];
    $rule['threshold'] = $tokens[2];
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Criteria_Peak ($tokens)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_criteria_peak';
    $rule['criteria'] = $tokens[1];
    $rule['threshold'] = $tokens[2];
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Criteria_Sum ($tokens)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_criteria_sum';
    $rule['criteria'] = $tokens[1];
    $rule['operator'] = $tokens[2];
    $rule['sum']      = $tokens[3];
    $rule['song']     = $tokens[4];
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Criteria_Number ($tokens)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_criteria_number';
    $rule['criteria'] = $tokens[1];
    $rule['operator'] = $tokens[2] ? trim ($tokens[2]) : '=';
    $rule['value']    = $tokens[3];
    $rule['number']   = $tokens[4];
    D_Get_Time_Or_Song ($rule, $tokens[6], $tokens[5]);
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Criteria_Multi_Number ($tokens)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_criteria_multi_number';
    $rule['criteria'] = $tokens[1];
    $rule['number']   = $tokens[2];
    D_Get_Time_Or_Song ($rule, $tokens[4], $tokens[3]);
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Playlist_Number ($tokens)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_playlist_number';
    $rule['not'] = $tokens[1] ? true : false;
    $rule['playlist'] = $tokens[2];
    $rule['number']   = $tokens[3];
    D_Get_Time_Or_Song ($rule, $tokens[5], $tokens[4]);
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Control_Playlists_Number ($tokens)
{
    $rule = array ();
    $rule['type'] = 'sequence_control_playlists_number';
    $rule['not'] = $tokens[1] ? true : false;
    $rule['mark']   = $tokens[2];
    $rule['number'] = $tokens[3];
    D_Get_Time_Or_Song ($rule, $tokens[5], $tokens[4]);
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Operator_Criteria ($tokens)
{
    $rule = array ();
    $rule['type'] = ($tokens[1] == 'FORBID') ?
        'sequence_forbid_criteria' :
        'sequence_force_criteria';
    $rule['criteria_1'] = $tokens[2];
    $rule['operator_1'] = $tokens[3] ? trim ($tokens[3]) : '=';
    $rule['value_1']    = $tokens[4];
    $rule['criteria_2'] = $tokens[5];
    $rule['operator_2'] = $tokens[6] ? trim ($tokens[6]) : '=';
    $rule['value_2']    = $tokens[7];
    $rule['percent']    = $tokens[8] ? $tokens[7] : 100;
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Playlist_Series ($tokens)
{
    // Cette règle est un raccourci pour la règle 
    // D_Rule_Parse_Sequence_Operator_Playlist avec les deux playlists
    // identiques
    $tokens[3] = $tokens[2];
    return D_Rule_Parse_Sequence_Operator_Playlist ($tokens);
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Playlist_Series_Limit ($tokens)
{
    // Cette règle est un raccourci pour les règles 
    // D_Rule_Parse_Sequence_Control_Playlist_Number
    // et D_Rule_Parse_Sequence_Playlist_Series

    $t1[0] = $t2[0] = $tokens[0];
    $t1[1] = false; // NOT should be false
    $t1[2] = $tokens[1]; // Playlist
    $t1[3] = $tokens[2]; // Max
    $t1[4] = $t1[3] + 1; // Song window
    $t1[5] = 'SONG';
    $t2[1] = 'FORCE';
    $t2[2] = $tokens[1];

    $r[] = D_Rule_Parse_Sequence_Control_Playlist_Number ($t1);
    $r[] = D_Rule_Parse_Sequence_Playlist_Series ($t2);
    return $r;
}

/*****************************************************************************/

function D_Rule_Parse_Sequence_Operator_Playlist ($tokens)
{
    $rule = array ();
    $rule['type'] = ($tokens[1] == 'FORBID') ?
        'sequence_forbid_playlist' :
        'sequence_force_playlist';
    $rule['playlist_1'] = $tokens[2];
    $rule['playlist_2'] = $tokens[3];
    $rule['percent']    = $tokens[5] ? $tokens[5] : 100;
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Control_Criteria ($tokens)
{
    $rule = array ();
    $rule['type']     = 'control_criteria';    
    $rule['criteria'] = $tokens[1];
    $rule['operator'] = $tokens[2];
    $rule['value']    = $tokens[3];
    return $rule;
}


/*****************************************************************************/

function D_Rule_Parse_Dayparts ($tokens)
{
    $rule['type'] = ($tokens[1] == 'SONG') ? 'song_dayparts' : 'smart_dayparts';
    if (isset ($tokens[2]))
    {
        $rule['parts'] = $tokens[2] ? $tokens[2] : 6;
        $rule['days'] = $tokens[4] ? $tokens[4] : 0;
    }
    return $rule;
}

/*****************************************************************************/

function D_Rule_Parse_Ignore ($tokens)
{
    return array ('type' => 'ignore');
}

/*****************************************************************************/

?>
