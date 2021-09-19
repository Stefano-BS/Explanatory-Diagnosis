#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#define VUOTO -1
#define ACAPO -10
#define REGEX 10
#define REGEXLEVER 2

#define ottieniComando(com) while (!isalpha(*com=getchar()));
#define inizioTimer inizio = clock();

// MODEL
typedef struct link {
    struct componente *da, *a;
    int id, intId;
} Link;

typedef struct transizione {
    int oss, ril, da, a, id;
    int idEventoIn, *idEventiU, nEventiU, sizeofEvU;
    struct link *linkIn, **linkU;
} Transizione;

typedef struct componente {
    int nStati, id, intId, nTransizioni;
    struct transizione **transizioni;
} Componente;

// BEHAVIORAL SPACE
typedef struct statorete {
    int *statoComponenti, *contenutoLink;
    int id, indiceOsservazione;
    bool finale;
    struct ltrans *transizioni;
} StatoRete;

typedef struct {
    StatoRete *da, *a;
    int dimRegex;
    struct transizione * t;
    char *regex;
    bool parentesizzata, concreta;
} TransizioneRete;

struct ltrans {
    TransizioneRete *t;
    struct ltrans * prossima;
};

typedef struct behspace {
    StatoRete ** states;
    int nStates, nTrans, sizeofS;
} BehSpace;


extern int nlink, ncomp, *osservazione, loss;
extern Componente **componenti;
extern Link **links;
extern char nomeFile[100];

// DataStructures.c
Componente * nuovoComponente(void);
Link* linkDaId(int);
Componente* compDaId(int);
void alloc1trC(Componente *);
void allocamentoIniziale(BehSpace *);
void alloc1(BehSpace *, char);
StatoRete * generaStato(int *, int *);
void eliminaStato(BehSpace *, int);
void freeStatoRete(StatoRete *);
void memCoherenceTest(BehSpace *);
BehSpace * dup(BehSpace *, bool[], bool);
// Parser.c
void parse(FILE*);
void parseDot(BehSpace *, FILE *, bool);
// Printer.c
void stampaStruttureAttuali(StatoRete *, bool);
void stampaSpazioComportamentale(BehSpace *, bool);
// SpaceMaker.c
bool ampliaSpazioComportamentale(BehSpace * b, StatoRete *, StatoRete *, Transizione *);
void potatura(BehSpace *);
void generaSpazioComportamentale(BehSpace *, StatoRete *);
BehSpace ** faultSpaces(BehSpace *, int *);
// Diagnoser.c
char* diagnostica(BehSpace *);

#define LANG_ENG

#if defined LANG_ITA
    #define INPUT_Y 's'
    #define MSG_YES "si"
    #define MSG_NO "no"
    #define LOGO "    ______                     __                     ___         __                  _\n   / ____/_______  _______  __/ /_____  ________     /   | __  __/ /_____  ____ ___  (_)\n  / __/ / ___/ _ \\/ ___/ / / / __/ __ \\/ ___/ _ \\   / /| |/ / / / __/ __ \\/ __ `__ \\/ /\n / /___(__  )  __/ /__/ /_/ / /_/ /_/ / /  /  __/  / ___ / /_/ / /_/ /_/ / / / / / / /\n/_____/____/\\___/\\___/\\____/\\__/\\____/_/   \\___/  /_/  |_\\____/\\__/\\____/_/ /_/ /_/_/\n"
    #define MSG_OBS "Fornire la sequenza di etichette. Ogni numero, a capo, per terminare usa un carattere non numerico\n"
    #define MSG_DEF_AUTOMA "\nIndicare il file che contiene la definizione dell'automa: "
    #define MSG_NO_FILE "File \"%s\" inesistente!\n"
    #define MSG_PARS_DONE "Parsing effettuato...\n"
    #define MSG_DOT "Salvare i grafi come .dot (s), stampare testo (t) o nessun'uscita (n)? "
    #define MSG_CHOOSE_OP "Generare spazio comportamentale (c), fornire un'osservazione lineare (o), o caricare uno spazio da file (f, se gli stati sono rinominati: g)? "
    #define MSG_DOT_INPUT "Indicare il file dot generato contenete lo spazio comportamentale: "
    #define MSG_INPUT_NOT_OBSERVATION "Lo spazio non corrisponde ad un'osservazione lineare, pertanto non si consiglia un suo utilizzo per diagnosi\n"
    #define MSG_INPUT_UNKNOWN_TYPE "Non e' possibile stabilire se lo spazio importato sia derivante da un'osservazione lineare: eseguire una diagnosi solo in caso affermativo\n"
    #define MSG_GEN_SC "Generazione spazio comportamentale...\n"
    #define MSG_POTA "Effettuare potatura (s/n)? "
    #define MSG_POTA_RES "Potati %d stati e %d transizioni\n"
    #define MSG_SC_RES "Generato lo spazio: conta %d stati e %d transizioni\n"
    #define MSG_RENAME_STATES "Rinominare gli stati col loro id (s/n)? "
    #define MSG_DIAG "Eseguire una diagnosi su questa osservazione (s/n)? "
    #define MSG_DIAG_EXEC "Eseguo diagnostica... \n"
    #define MSG_ACTUAL_STRUCTURES "\nStrutture dati attuali:\n"
    #define MSG_COMP_DESCRIPTION "Componente id:%d, ha %d stati (attivo: %d) e %d transizioni:\n"
    #define MSG_TRANS_DESCRIPTION "\tTransizione id:%d va da %d a %d, osservabile: %d, rilevante: %d,"
    #define MSG_TRANS_DESCRIPTION2 " nessun evento entrante,"
    #define MSG_TRANS_DESCRIPTION3 " evento in ingresso: %d sul link %d,"
    #define MSG_TRANS_DESCRIPTION4 " nessun evento in uscita.\n"
    #define MSG_TRANS_DESCRIPTION5 " eventi in uscita:\n"
    #define MSG_TRANS_DESCRIPTION6 "\t\tEvento %d sul link id:%d\n"
    #define MSG_LINK_DESCRIPTION1 "Link id:%d, vuoto, collega %d a %d\n"
    #define MSG_LINK_DESCRIPTION2 "Link id:%d, contiene evento %d, collega %d a %d\n"
    #define MSG_END_STATE_DESC "Fine contenuto. Stato finale: %s\n"
    #define MSG_SOBSTITUTION_LIST "Elenco sostituzioni nomi degli stati:\n"
    #define MSG_PARSERR_NOENDLINE "Linea %d - Atteso fine linea\n"
    #define MSG_PARSERR_TOKEN "Linea %d - Carattere inaspettato: %c (atteso: %c)\n"
    #define MSG_PARSERR_LINE "Riga non corretta: %s\n"
    #define MSG_PARSERR_TRANS_NOT_FOUND "Errore: transizione %d non trovata\n"
    #define MSG_COMP_NOT_FOUND "Errore: componente con id %d non trovato\n"
    #define MSG_LINK_NOT_FOUND "Errore: link con id %d non trovato\n"
    #define MSG_MEMERR "Errore di allocazione memoria!\n"
    #define MSG_STATE_ANOMALY "Anomalia nello stato %d\n"
    #define MSG_MEMTEST1 "Spazio: %d stati e %d transizioni\n"
    #define MSG_MEMTEST2 "Lo stato %d non è coerente col proprio id %d\n"
    #define MSG_MEMTEST3 "Lo stato %d ha tr dall'id %d all'id %d\n"
    #define MSG_MEMTEST4 "Lo stato id %d, presso %p ha transizione che non punta a sé: "
    #define MSG_MEMTEST5 "da %p (id %d) a %p (id %d)\n"
    #define MSG_MEMTEST6 "\tmemcmp stato a: %d\n"
    #define MSG_MEMTEST7 "\tmemcmp stato da: %d\n"
    #define fineTimer if (benchmark) printf("\tTempo: %fs\n", ((float)(clock() - inizio))/CLOCKS_PER_SEC);
#elif defined LANG_ENG
    #define INPUT_Y 'y'
    #define MSG_YES "yes"
    #define MSG_NO "no"
    #define LOGO "  ___        _                        _          _____                    _             \n / _ \\      | |                      | |        |  ___|                  | |            \n/ /_\\ \\_   _| |_ ___  _ __ ___   __ _| |_ __ _  | |____  _____  ___ _   _| |_ ___  _ __ \n|  _  | | | | __/ _ \\| '_ ` _ \\ / _` | __/ _` | |  __\\ \\/ / _ \\/ __| | | | __/ _ \\| '__|\n| | | | |_| | || (_) | | | | | | (_| | || (_| | | |___>  <  __/ (__| |_| | || (_) | |   \n\\_| |_/\\__,_|\\__\\___/|_| |_| |_|\\__,_|\\__\\__,_| \\____/_/\\_\\___|\\___|\\__,_|\\__\\___/|_|\n"
    #define MSG_OBS "Provide the observation sequence. A new line foreach number, non numeric to stop\n"
    #define MSG_DEF_AUTOMA "\nWhere does automata'definition file locate: "
    #define MSG_NO_FILE "File \"%s\" not found!\n"
    #define MSG_PARS_DONE "Parsing done...\n"
    #define MSG_DOT "Save graphs as .dot (y), print as text (t), or no output (n)? "
    #define MSG_CHOOSE_OP "Generate behavioral space (c), input a linear observation (o), or load a space from file (f, if states have been renamed: g)? "
    #define MSG_DOT_INPUT "Input .dot file describing the behavioral space: "
    #define MSG_INPUT_NOT_OBSERVATION "This space is not the result of a linear observation, thus it is not recommended a diagnosis on that\n"
    #define MSG_INPUT_UNKNOWN_TYPE "It is not possible to establish if this space is the result of linear observation: execute a diagnosis just in that case\n"
    #define MSG_GEN_SC "Calculating space...\n"
    #define MSG_POTA "Prune (y/n)? "
    #define MSG_POTA_RES "%d states and %d transitions pruned\n"
    #define MSG_SC_RES "Space generated: total %d states, %d transitions\n"
    #define MSG_RENAME_STATES "Rename states with their id (y/n)? "
    #define MSG_DIAG "Calculate a diagnosis upon this observation (y/n)? "
    #define MSG_DIAG_EXEC "Calculating diagnosis... \n"
    #define MSG_ACTUAL_STRUCTURES "\nData structures:\n"
    #define MSG_COMP_DESCRIPTION "Component id:%d, has %d states (active: %d) and %d transitions:\n"
    #define MSG_TRANS_DESCRIPTION "\tTransition id:%d goes from %d to %d, observability: %d, relevance: %d,"
    #define MSG_TRANS_DESCRIPTION2 " no incoming event,"
    #define MSG_TRANS_DESCRIPTION3 " incoming event: %d on link %d,"
    #define MSG_TRANS_DESCRIPTION4 " no outgoing event.\n"
    #define MSG_TRANS_DESCRIPTION5 " outgoing events:\n"
    #define MSG_TRANS_DESCRIPTION6 "\t\tEvent %d on link id:%d\n"
    #define MSG_LINK_DESCRIPTION1 "Link id:%d, empty, from %d to %d\n"
    #define MSG_LINK_DESCRIPTION2 "Link id:%d, containing event %d, from %d to %d\n"
    #define MSG_END_STATE_DESC "End. Final state: %s\n"
    #define MSG_SOBSTITUTION_LIST "List of sobstitutions:\n"
    #define MSG_PARSERR_NOENDLINE "Line %d - end of line expected\n"
    #define MSG_PARSERR_TOKEN "Line %d - unexpected char: %c (expected: %c)\n"
    #define MSG_PARSERR_LINE "Malformed row: %s\n"
    #define MSG_PARSERR_TRANS_NOT_FOUND "Error: transition %d not found\n"
    #define MSG_COMP_NOT_FOUND "Error: component with id %d not found\n"
    #define MSG_LINK_NOT_FOUND "Error: link with id %d not found\n"
    #define MSG_MEMERR "Unable to alloc memory!\n"
    #define MSG_STATE_ANOMALY "Anomaly in state %d\n"
    #define MSG_MEMTEST1 "Status: %d states and %d transizioni\n"
    #define MSG_MEMTEST2 "State %d is not coherent with its id %d\n"
    #define MSG_MEMTEST3 "State %d has transition from id %d to id %d\n"
    #define MSG_MEMTEST4 "State id %d, located at %p has transition not pointing to itself: "
    #define MSG_MEMTEST5 "from %p (id %d) to %p (id %d)\n"
    #define MSG_MEMTEST6 "\tmemcmp state to: %d\n"
    #define MSG_MEMTEST7 "\tmemcmp state from: %d\n"
    #define fineTimer if (benchmark) printf("\tTime: %fs\n", ((float)(clock() - inizio))/CLOCKS_PER_SEC);
#endif