<?php // $Id: schema_parsers.inc 3 2006-07-13 21:23:43Z mouke $

$DECIDER_SCHEMA_PARSERS['RANDOM']             = 'D_Schema_Parser_RANDOM';
$DECIDER_SCHEMA_PARSERS['ORDERED']            = 'D_Schema_Parser_ORDERED';
$DECIDER_SCHEMA_PARSERS['META']               = 'D_Schema_Parser_META';
$DECIDER_SCHEMA_PARSERS['CYCLE']              = 'D_Schema_Parser_CYCLE';
$DECIDER_SCHEMA_PARSERS['STRUCTURED_RANDOM']  = 'D_Schema_Parser_STRUCTURED_RANDOM';

require_once ('schema_parser_random.inc');
require_once ('schema_parser_structured_random.inc');
require_once ('schema_parser_ordered.inc');
require_once ('schema_parser_meta.inc');
require_once ('schema_parser_cycle.inc');

?>
