<?php // $Id: schema_parser_meta.inc 44 2007-02-03 21:42:10Z mouke $

/**
 *  Parse a string with the following structure:
 *
 *    PROGRAMS n1,n2.. 
 *    PERCENTILES p1,p2..
 *
 *  In case of errors, programs will be set to default and percentiles
 *  will be set to 100 but if the sum of percentiles is < 100
 *  the percentile of the first program will be adjusted.
 */
function D_Schema_Parser_META ($schema_string, $prg, $dbo)
{
    global $CONFIG, $LOG, $MSGS;
    $schema = array ('type' => 'META');

    $LOG->log ("Schema prog $prg : \n$schema_string", PEAR_LOG_DEBUG);
    
    list ($programs, $percent) = explode ("\n", $schema_string);

    if (!preg_match ('/^PROGRAMS\s+([0-9]+,)*[0-9]+$/',
                     $programs)
        || ! preg_match ('/^PERCENTILES\s+([0-9]+,)*[0-9]+$/',
                         $percent))
    {
        $schema['programs'] = array ($CONFIG['default_program']);
        $schema['percent'] = array (1);
        $LOG->log (sprintf ($MSGS['K_PARSE_META_SYNTAX_ERROR'],
                            $prg, $schema_string),
                   PEAR_LOG_ERR);
        return $schema;
    }
    
    list ($keyword, $programs) = preg_split ('/\s+/', $programs);
    list ($keyword, $percent) = preg_split ('/\s+/', $percent);

    $programs = explode (',', $programs);
    $percent = explode (',', $percent);

    if (count($percent) != count($programs))
    {
        $LOG->log (sprintf ($MSGS['K_PARSE_META_MISMATCH_ERROR'],
                            $prg, $schema_string),
                   PEAR_LOG_ERR);
        $schema['programs'] = array ($CONFIG['default_program']);
        $schema['percent'] = array (1);
        return $schema;
    }

    $s = array_sum ($percent);
    if ($s != 100)
    {
        $LOG->log (sprintf ($MSGS['K_PARSE_META_NOT100_ERROR'],
                            $prg, $s, $schema_string),
                   PEAR_LOG_ERR);
        if (s < 100)
        {
            $percent[0] += (100 - $s);
        }
        else
        {
            $schema['programs'] = array ($CONFIG['default_program']);
            $schema['percent'] = array (1);
            return $schema;
        }
    }

    $schema['programs']   = $programs;
    $schema['percent']    = $percent;
    $schema['separation'] = '2 DAY';
    
    return $schema;
}

?>
