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

#define FLAG_FINAL 1
#define FLAG_SILENT_FINAL 1 << 1

#define ottieniComando(com) while (!isalpha(*com=getchar()));
#define inizioTimer inizio = clock();
#define foreach(lnode, from) for(lnode=from; lnode!=NULL; lnode = lnode->prossima)
#define foreachdecl(lnode, from) struct ltrans * lnode; foreach(lnode, from)

#ifndef DEBUG_MODE
    #define DEBUG_MODE false
#endif
#if DEBUG_MODE
    #define printlog(...) printf(__VA_ARGS__)
#else
    #define printlog(...)
#endif

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
    char flags;
    struct ltrans *transizioni;
} StatoRete;

typedef struct {
    StatoRete *da, *a;
    int dimRegex, marker;
    char *regex;
    bool parentesizzata, concreta;
    struct transizione * t;
} TransizioneRete;

struct ltrans {
    TransizioneRete *t;
    struct ltrans * prossima;
};

typedef struct behspace {
    StatoRete ** states;
    int nStates, nTrans, sizeofS;
} BehSpace;

// EXPLAINER
typedef struct faultspace {
    BehSpace *b;
    int *idMapToOrigin, *idMapFromOrigin, *exitStates;
} FaultSpace;

typedef struct texp {
    FaultSpace * from, *to;
    int obs, ril, fromStateId, toStateId;
    char * regex;
} TransExpl;

typedef struct explainer {
    FaultSpace ** faults;
    TransExpl ** trans;
    int nFaultSpaces, nTrans, sizeofTrans;
} Explainer;

extern int nlink, ncomp, *osservazione, loss;
extern Componente **componenti;
extern Link **links;
extern char nomeFile[100];
extern const unsigned int eps;

// DataStructures.c
Componente * nuovoComponente(void);
Link* linkDaId(int);
Componente* compDaId(int);
void netAlloc(void);
void alloc1(BehSpace *, char);
void alloc1trC(Componente *);
void alloc1trExp(Explainer *);
BehSpace * newBehSpace(void);
StatoRete * generaStato(int *, int *);
void removeState(BehSpace *, StatoRete *);
void freeStatoRete(StatoRete *);
BehSpace * dup(BehSpace *, bool[], bool, int **);
void freeBehSpace(BehSpace *);
void behCoherenceTest(BehSpace *);
void expCoherenceTest(Explainer *);
// Parser.c
void parse(FILE*);
BehSpace * parseDot(FILE *, bool);
// Printer.c
void stampaStruttureAttuali(StatoRete *, bool);
char* stampaSpazioComportamentale(BehSpace *, bool, int);
void printExplainer(Explainer *);
// SpaceMaker.c
bool ampliaSpazioComportamentale(BehSpace * b, StatoRete *, StatoRete *, Transizione *);
void potatura(BehSpace *);
void generaSpazioComportamentale(BehSpace *, StatoRete *);
FaultSpace ** faultSpaces(BehSpace *, int *, TransizioneRete ****);
// Diagnoser.c
void regexMake(TransizioneRete*, TransizioneRete*, TransizioneRete*, char, TransizioneRete *);
char** diagnostica(BehSpace *, bool);
// Explainer.c
Explainer * makeExplainer(BehSpace *);

#ifndef LANG
    #define LANG 'e'
#endif

#if LANG == 'i'
    #define INPUT_Y 's'
    #define MSG_YES "si"
    #define MSG_NO "no"
    #define LOGO "    ______                     __                     ___         __                  _\n   / ____/_______  _______  __/ /_____  ________     /   | __  __/ /_____  ____ ___  (_)\n  / __/ / ___/ _ \\/ ___/ / / / __/ __ \\/ ___/ _ \\   / /| |/ / / / __/ __ \\/ __ `__ \\/ /\n / /___(__  )  __/ /__/ /_/ / /_/ /_/ / /  /  __/  / ___ / /_/ / /_/ /_/ / / / / / / /\n/_____/____/\\___/\\___/\\____/\\__/\\____/_/   \\___/  /_/  |_\\____/\\__/\\____/_/ /_/ /_/_/\n"
    #define MSG_MENU_INTRO "\nMenu\tx: Esci\n"
    #define MSG_MENU_C "\tc: Genera Spazio Comportamentale\n"
    #define MSG_MENU_O "\to: Genera Spazio Comportamentale relativo ad osservazione completa\n"
    #define MSG_MENU_D "\td: Calcola una diagnosi su questa osservazione completa\n"
    #define MSG_MENU_E "\te: Genera un Diagnosticatore\n"
    #define MSG_MENU_FG "\tf: Carica Spazio Comportamentale da file\n\tg: Carica Spazio Comportamentale da file (stati rinominati)\n"
    #define MSG_MENU_END "Scelta: "
    #define MSG_OBS "Fornire la sequenza di etichette. Ogni numero, a capo, per terminare usa un carattere non numerico\n"
    #define MSG_DEF_AUTOMA "\nIndicare il file che contiene la definizione dell'automa: "
    #define MSG_NO_FILE "File \"%s\" inesistente!\n"
    #define MSG_PARS_DONE "Parsing effettuato...\n"
    #define MSG_DOT "Salvare i grafi come .dot (s), stampare testo (t) o nessun'uscita (n)? "
    #define MSG_DOT_INPUT "Indicare il file dot generato contenete lo spazio comportamentale: "
    #define MSG_INPUT_NOT_OBSERVATION "Lo spazio non corrisponde ad un'osservazione lineare, pertanto non si consiglia un suo utilizzo per diagnosi\n"
    #define MSG_INPUT_UNKNOWN_TYPE "Non e' possibile stabilire se lo spazio importato sia derivante da un'osservazione lineare: eseguire una diagnosi solo in caso affermativo\n"
    #define MSG_GEN_SC "Generazione spazio comportamentale...\n"
    #define MSG_POTA "Effettuare potatura (s/n)? "
    #define MSG_POTA_RES "Potati %d stati e %d transizioni\n"
    #define MSG_SC_RES "Generato lo spazio: conta %d stati e %d transizioni\n"
    #define MSG_RENAME_STATES "Rinominare gli stati col loro id (s/n)? "
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
    #define MSG_MEMTEST1 "Spazio Comportamentale: %d stati e %d transizioni\n"
    #define MSG_MEMTEST2 "Lo stato %d non è coerente col proprio id %d\n"
    #define MSG_MEMTEST3 "Lo stato %d ha tr dall'id %d all'id %d\n"
    #define MSG_MEMTEST4 "Lo stato id %d, presso %p ha transizione che non punta a sé: "
    #define MSG_MEMTEST5 "da %p (id %d) a %p (id %d)\n"
    #define MSG_MEMTEST6 "\tmemcmp stato a: %d\n"
    #define MSG_MEMTEST7 "\tmemcmp stato da: %d\n"
    #define MSG_MEMTEST8 "Trovata TransExpl da/per chiusura inesistente\n"
    #define MSG_MEMTEST9 "Diagnosticatore: %d chiusure e %d transizioni\n"
    #define MSG_MEMTEST10 "La transizione %d non è osservabile\n"
    #define MSG_MEMTEST11 "La chiusura %d ha mapToOrigin -1 in posizione %d\n"
    #define MSG_MEMTEST12 "Chiusura %d: trovato che mapFrom[mapTo[%d]] != %d\n"
    #define MSG_EXP_FAULT_NOT_FOUND "Non è stato possibile trovare una chiusura di destinazione ad una transizione\n"
    #define fineTimer if (benchmark) printf("\tTempo: %fs\n", ((float)(clock() - inizio))/CLOCKS_PER_SEC);
#elif LANG=='e'
    #define INPUT_Y 'y'
    #define MSG_YES "yes"
    #define MSG_NO "no"
    #define LOGO "  ___        _                        _          _____                    _             \n / _ \\      | |                      | |        |  ___|                  | |            \n/ /_\\ \\_   _| |_ ___  _ __ ___   __ _| |_ __ _  | |____  _____  ___ _   _| |_ ___  _ __ \n|  _  | | | | __/ _ \\| '_ ` _ \\ / _` | __/ _` | |  __\\ \\/ / _ \\/ __| | | | __/ _ \\| '__|\n| | | | |_| | || (_) | | | | | | (_| | || (_| | | |___>  <  __/ (__| |_| | || (_) | |   \n\\_| |_/\\__,_|\\__\\___/|_| |_| |_|\\__,_|\\__\\__,_| \\____/_/\\_\\___|\\___|\\__,_|\\__\\___/|_|\n"
    #define MSG_MENU_INTRO "\nMenu\tx: Exit\n"
    #define MSG_MENU_C "\tc: Generate Behavioral Space\n"
    #define MSG_MENU_O "\to: Generate Behavioral Space relative to a full observation\n"
    #define MSG_MENU_D "\td: Calculate diagnosis over this full observation\n"
    #define MSG_MENU_E "\te: Generate Explainer\n"
    #define MSG_MENU_FG "\tf: Load Behavioral Space from file\n\tg: Load Behavioral space from file (states renamed)\n"
    #define MSG_MENU_END "Your choice: "
    #define MSG_OBS "Provide the observation sequence. A new line foreach number, non numeric to stop\n"
    #define MSG_DEF_AUTOMA "\nWhere does automata's definition file locate: "
    #define MSG_NO_FILE "File \"%s\" not found!\n"
    #define MSG_PARS_DONE "Parsing done...\n"
    #define MSG_DOT "Save graphs as .dot (y), print as text (t), or no output (n)? "
    #define MSG_DOT_INPUT "Input .dot file describing the behavioral space: "
    #define MSG_INPUT_NOT_OBSERVATION "This space is not the result of a linear observation, thus it is not recommended a diagnosis on that\n"
    #define MSG_INPUT_UNKNOWN_TYPE "It is not possible to establish if this space is the result of linear observation: execute a diagnosis just in that case\n"
    #define MSG_GEN_SC "Calculating space...\n"
    #define MSG_POTA "Prune (y/n)? "
    #define MSG_POTA_RES "%d states and %d transitions pruned\n"
    #define MSG_SC_RES "Space generated: total %d states, %d transitions\n"
    #define MSG_RENAME_STATES "Rename states with their id (y/n)? "
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
    #define MSG_MEMTEST1 "Behavioral Space: %d states and %d transitions\n"
    #define MSG_MEMTEST2 "State %d is not coherent with its id %d\n"
    #define MSG_MEMTEST3 "State %d has transition from id %d to id %d\n"
    #define MSG_MEMTEST4 "State id %d, located at %p has transition not pointing to itself: "
    #define MSG_MEMTEST5 "from %p (id %d) to %p (id %d)\n"
    #define MSG_MEMTEST6 "\tmemcmp state to: %d\n"
    #define MSG_MEMTEST7 "\tmemcmp state from: %d\n"
    #define MSG_MEMTEST8 "Found TransExpl from/to non existent fault space\n"
    #define MSG_MEMTEST9 "Explainer: %d fault spaces and %d transitions\n"
    #define MSG_MEMTEST10 "Transition %d is not observable\n"
    #define MSG_MEMTEST11 "Fault %d has mapToOrigin -1 in position %d\n"
    #define MSG_MEMTEST12 "Fault %d: found mapFrom[mapTo[%d]] != %d\n"
    #define MSG_EXP_FAULT_NOT_FOUND "Unable to find a fault space destination for a transition\n"
    #define fineTimer if (benchmark) printf("\tTime: %fs\n", ((float)(clock() - inizio))/CLOCKS_PER_SEC);
#endif

/*
#define MSG_CHOOSE_OP "Generare spazio comportamentale (c), fornire un'osservazione lineare (o), o caricare uno spazio da file (f, se gli stati sono rinominati: g)? "
#define MSG_DIAG "Eseguire una diagnosi su questa osservazione (s/n)? "
#define MSG_CHOOSE_OP "Generate behavioral space (c), input a linear observation (o), or load a space from file (f, if states have been renamed: g)? "
#define MSG_DIAG "Calculate a diagnosis upon this observation (y/n)? "
*/