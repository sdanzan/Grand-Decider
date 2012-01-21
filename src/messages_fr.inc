<?php // $Id: messages_fr.inc 45 2007-02-28 12:54:46Z mouke $

$MSGS = array (
    'K_DB_FAILED' =>
    "Erreur de base de données (%s, ligne %d), le message est :\n\"%s\".",

    'K_COMMAND' =>
    "Commande : '%s'",

    'K_COMMIT_FAILED' =>
    "La mise à jour de la pile a echoué, le message de la base de donnée est :\n\"%s\".",

    'K_COMMIT_REQUEST_FAILED' =>
    "La mise à jour de la pile s'est mal passé, le message de la base de donnée est :\n\"%s\"\n et la requête était :\n\"%s\n\".",
    
    'K_PRG_UNKNOWN_TYPE' =>
    "Type de programme inconnu : \"%s\" (prg n°%d).",

    'K_PRG_LOAD_CONF' =>
    "Chargement de la configuration des programmes.",

    'K_RULE_SYNTAX_ERROR' =>
    "Erreur de syntaxe pour la règle :\n\"%s\"",

    'K_RULE_REMOVE' =>
    "Retrait de la règle : \"%s\"",

    'K_EVENT_SYNTAX_ERROR' =>
    "Erreur de syntaxe pour l'événement :\n\"%s\"",

    'K_LINK_SYNTAX_ERROR' =>
    "Erreur de syntaxe pour le lien :\n\"%s\"",

    'K_PARSE_STRUCTURED_RANDOM_PLAYLIST_ERROR' =>
    "Programme de type STRUCTURED_RANDOM (prg n°%d) :\nErreur de syntaxe pour la définition de la playlist (%s)",

    'K_PARSE_CYCLE_PLAYLIST_ERROR' =>
    "Programme de type CYCLE (prg n°%d) :\nErreur de syntaxe pour la définition de la playlist (%s)",

    'K_PARSE_CYCLE_CYCLE_ERROR' =>
    "Programme de type CYCLE (prg n°%d) :\nErreur de syntaxe pour la définition du cycle (%s)",
    
    'K_PARSE_CYCLE_BASE_ERROR' =>
    "Programme de type CYCLE (prg n°%d) :\nErreur de syntaxe pour la définitionde la durée de base (%s)",

    'K_PARSE_RANDOM_PLAYLIST_ERROR' =>
    "Programme de type RANDOM (prg n°%d) :\nErreur de syntaxe pour le champ 'PLAYLIST' (mot-clef : %s / valeur : %s)",

    'K_PARSE_RANDOM_RANDOM_ERROR' =>
    "Programme de type RANDOM (prg n°%d) :\nErreur de syntaxe pour le champ 'RANDOM' (mot-clef : %s / valeur : %s)",

    'K_PARSE_ORDERED_PLAYLIST_ERROR' =>
    "Programme de type ORDERED (prg n°%d) :\nErreur de syntaxe pour le champ 'PLAYLIST' (mot-clef : %s / valeur : %s)",

    'K_PARSE_ORDERED_ORDER_ERROR' =>
    "Programme de type ORDERED (prg n°%d) :\nErreur de syntaxe pour le champ d'ordre (%s)",

    'K_PARSE_META_SYNTAX_ERROR' =>
    "Programme de type META (prg n°%d) :\nErreur de syntaxe\n%s",

    'K_LINK_SCHEDULER_SONG' =>
    "Programmation d'un lien de chanson",

    'K_LINK_SCHEDULER_ARTIST' =>
    "Programmation d'un lien d'artiste",

    'K_LINK_SCHEDULER_PLAYLIST' =>
    "Programmation d'un lien de liste",

    'K_LINK_SCHEDULER_PROGRAM' =>
    "Programmation d'un lien de programme",

    'K_LINK_SCHEDULER_PROGRAM_BEGIN' =>
    "Programmation d'un lien de début de programme",

    'K_LINK_SCHEDULER_PROGRAM_END' =>
    "Programmation d'un lien de fin de programme",

    'K_LINK_SCHEDULER_EVENT' =>
    "Programmation d'un lien d'événement",
    
    'K_TIMED_EVENT_SCHEDULER_NEW_SONG' =>
    "Programmation d'un événement daté (%s)",

    'K_ROTATED_EVENT_SCHEDULER_NEW_SONG' =>
    "Programmation d'un événement en rotation",

    'K_REQUEST_SCHEDULER_NEW_SONG' =>
    "Programmation d'une requête (n° %d)",

    'K_SCHEDULER_ORDERED_NEW_SONG' =>
    "Programme de type ORDERED, choix d'un nouveau morceau (position suivante de %d)",

    'K_SCHEDULER_STRUCTURED_RANDOM_NEW_SONG' =>
    "Programme de type STRUCTURED_RANDOM, choix d'un nouveau morceau",

    'K_SCHEDULER_CYCLE_NEW_SONG' =>
    "Programme de type CYCLE, choix d'un nouveau morceau",

    'K_PARSE_META_MISMATCH_ERROR' =>
    "Programme de type META (prg n°%d) :\nLe nombre de programmes et de pourcentages diffèrent\n%s",

    'K_PARSE_META_NOT100_ERROR' =>
    "Programme de type META (prg n°%d) :\nLa somme des pourcentages (%d) ne fait pas 100, on va faire ce qu'on peut pour corriger\n%s",

    'K_PARSE_ROTATION_ERROR' =>
    "Programme n°%d : Erreur dans la description des rotations. Ligne incriminée : %s",

    'K_SCHEDULER_META' =>
    "Un programme aléatoire démarre, choix du programme n°%s",

    'K_SCHEDULER_RANDOM_NEW_SONG' =>
    "Programme de type RANDOM, choix d'un nouveau morceau",

    'K_RESTART_LONG_TIME_AFTER' =>
    "Les données courantes sont anciennes, on repart de zéro.",

    'K_NO_SONG_FOUND' =>
    "Aucun morceau trouvé, basculement sur les morceaux de secours.",
      
    );
?>
