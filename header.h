#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <locale.h>

#define VUOTO           -1
#define ACAPO           -10
#define REGEX           5
#define REGEXLEVER      2
#define HASHSTATSIZE    181         // Occupy HASHSTATSIZE*2*sizeof(pointer) in RAM

#define FLAG_FINAL          1
#define FLAG_SILENT_FINAL   1 << 1

#define DEBUG_REGEX         1 << 1
#define DEBUG_MEMCOH        1 << 2
#define DEBUG_FAULT_DOT     1 << 3
#define DEBUG_MON           1 << 4

#define getCommand(com)             while (!isalpha(com=getchar()));
#define beginTimer                  timer = clock();
#define foreach(lnode, from)        for(lnode=from; lnode!=NULL; lnode = lnode->next)
#define foreachdecl(lnode, from)    struct ltrans * lnode; foreach(lnode, from)


#ifndef DEBUG_MODE
    #define DEBUG_MODE false
#endif
#if DEBUG_MODE
    #define printlog(...)           printf(__VA_ARGS__)
    #define debugif(mode, action)   if ((DEBUG_MODE & mode) == mode) action;
    #define INLINE(f)               f
    #define RESTRICT
#else
    #define printlog(...)
    #define debugif(...)
    #define INLINE(f)               inline f __attribute__((always_inline)); inline f
    #define RESTRICT                restrict
    #define NDEBUG
#endif
#include <assert.h>

// GENERAL
typedef struct {
    char * regex;
    int size;
    bool bracketed, concrete;
} Regex;

typedef struct {
    int length;
    struct sList {
        struct behstate *s;
        struct sList * next;
    } *sList [HASHSTATSIZE];
    struct tList {
        struct behtrans *t;
        struct faultspace *tempFault;
        struct tList * next;
    } *tList [HASHSTATSIZE+1];
} BehSpaceCatalog;

// DISCRETE EVENT SYSTEM
typedef struct {
    struct component *RESTRICT from, *RESTRICT to;
    int id, intId;
} Link;

typedef struct {
    int obs, fault, from, to, id;
    int idIncomingEvent, *RESTRICT idOutgoingEvents, nOutgoingEvents, sizeofOE;
    Link *RESTRICT linkIn, **RESTRICT linkOut;
} Trans;

typedef struct component {
    int nStates, id, intId, nTrans;
    Trans **RESTRICT transitions;
} Component;

// BEHAVIORAL SPACE
typedef struct behstate {
    int *RESTRICT componentStatus, *RESTRICT linkContent;
    int id, obsIndex;
    char flags;
    struct ltrans *RESTRICT transitions;
} BehState;

typedef struct behtrans {
    BehState *from, *to;
    int marker;
    Regex *regex;
    Trans * t;
} BehTrans;

struct ltrans {
    BehTrans *t;
    struct ltrans *RESTRICT next;
};

typedef struct {
    BehState **RESTRICT states;
    int nStates, nTrans, sizeofS;
} BehSpace;

// EXPLAINER AND MONITORNING
typedef struct faultspace {
    BehSpace *b;
    int *RESTRICT idMapToOrigin, * idMapFromOrigin, *RESTRICT exitStates;
    Regex ** diagnosis, *alternativeOfDiagnoses;
} FaultSpace;

typedef struct {
    FaultSpace * from, *to;
    int obs, fault, fromStateId, toStateId;
    Regex * regex;
} ExplTrans;

typedef struct {
    FaultSpace **RESTRICT faults;
    ExplTrans **RESTRICT trans;
    int nFaultSpaces, nTrans, sizeofTrans, sizeofFaults;
} Explainer;

typedef struct {
    FaultSpace * from, * to;
    Regex *l, *lp;
} MonitorTrans;

typedef struct {
    FaultSpace **RESTRICT expStates;
    MonitorTrans **RESTRICT arcs;
    int nExpStates, nArcs, sizeofArcs;
    Regex * lmu, ** lin, ** lout;
} MonitorState;

typedef struct {
    MonitorState ** mu;
    int length;
} Monitoring;


// Global variables: these will never change during execution
extern int nlink, ncomp;
extern Component **RESTRICT components;
extern Link **RESTRICT links;
extern char inputDES[100];
extern Regex* empty;
extern const unsigned int eps, mu;
extern BehSpaceCatalog catalog;

// DataStructures.c
Component * newComponent(void);
Link* linkById(int);
Component* compById(int);
void netAlloc(void);
void alloc1(void *, char);
int hashBehState(BehState *);
bool behTransCompareTo(BehTrans *, BehTrans *);
bool behStateCompareTo(BehState *, BehState *);
BehSpace * newBehSpace(void);
BehState * generateBehState(int *, int *);
void removeBehState(BehSpace *, BehState *);
void freeBehState(BehState *);
BehSpace * dup(BehSpace *, bool[], bool, int **);
void freeBehSpace(BehSpace *);
void freeMonitoring(Monitoring *);
void behCoherenceTest(BehSpace *);
void expCoherenceTest(Explainer *);
void monitoringCoherenceTest(Monitoring *);
// Parser.c
void parseDES(FILE*);
BehSpace * parseBehSpace(FILE *, bool, int*);
// Printer.c
void printDES(BehState *, bool);
char* printBehSpace(BehSpace *, bool, bool, int);
void printExplainer(Explainer *);
void printMonitoring(Monitoring *, Explainer *);
// SpaceMaker.c
bool enlargeBehavioralSpace(BehSpace * b, BehState *, BehState *, Trans *, int);
void generateBehavioralSpace(BehSpace *, BehState *, int *, int);
void prune(BehSpace *);
FaultSpace * faultSpace(BehSpace *, BehState *, BehTrans **);
FaultSpace ** faultSpaces(BehSpace *, int *, BehTrans ****);
FaultSpace * makeLazyFaultSpace(Explainer *, BehState *);
// Diagnoser.c
void freeRegex(Regex *);
Regex * emptyRegex(int);
Regex * regexCpy(Regex *);
void regexMake(Regex*, Regex*, Regex*, char, Regex *);
Regex** diagnostics(BehSpace *, bool);
// Explainer.c
Explainer * makeExplainer(BehSpace *);
Explainer * makeLazyExplainer(Explainer *, BehState *);
Monitoring* explanationEngine(Explainer *, Monitoring *, int *, int, bool);

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
    #define MSG_MENU_M "\tm: Avvia processo di monitoraggio\n"
    #define MSG_MENU_L "\tl: Avvia processo di monitoraggio (compilazione pigra del Diagnosticatore)\n"
    #define MSG_MENU_END "Scelta: "
    #define MSG_OBS "Fornire la sequenza di etichette. Per ogni numero, a capo o spazio, per terminare usa un carattere non numerico\n"
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
    #define MSG_MEMTEST13 "Monitoraggio: %d stati\n"
    #define MSG_MEMTEST14 "Stato di monitoraggio %d: %d chiusure e %d transizioni\n"
    #define MSG_MEMTEST15 "Rilevata nello stato di monitoraggio %d una chiusura senza archi uscenti\n"
    #define MSG_MEMTEST16 "Rilevata nello stato di monitoraggio %d una chiusura senza archi entranti\n"
    #define MSG_EXP_FAULT_NOT_FOUND "Non è stato possibile trovare una chiusura di destinazione ad una transizione\n"
    #define MSG_MONITORING_RESULT "\nTraccia delle diagnosi:\n"
    #define MSG_NEXT_OBS "Fornisca l'osservazione successiva: "
    #define MSG_IMPOSSIBLE_OBS "L'ultima osservazione fornita non è coerente con le strutture dati\n"
    #define MSG_BEFORE_EXIT "\nTerminare? "
    #define endTimer if (benchmark) printf("\tTempo: %fs\n", ((float)(clock() - timer))/CLOCKS_PER_SEC);
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
    #define MSG_MENU_FG "\tf: Load Behavioral Space from file\n\tg: Load Behavioral Space from file (states renamed)\n"
    #define MSG_MENU_M "\tm: Start monitoring procedure\n"
    #define MSG_MENU_L "\tl: Start monitoring procedure (lazy Explainer compilation)\n"
    #define MSG_MENU_END "Your choice: "
    #define MSG_OBS "Provide the observation sequence. A new line or space foreach number, non numeric to stop\n"
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
    #define MSG_MEMTEST13 "Monitoring: %d states\n"
    #define MSG_MEMTEST14 "Monitoring State %d: %d fault spaces and %d transitions\n"
    #define MSG_MEMTEST15 "Found that in Monitoring State %d there's a fault state non exited by any arc\n"
    #define MSG_MEMTEST16 "Found that in Monitoring State %d there's a fault state not reached by any arc\n"
    #define MSG_EXP_FAULT_NOT_FOUND "Unable to find a fault space destination for a transition\n"
    #define MSG_MONITORING_RESULT "\nExplanation Trace:\n"
    #define MSG_NEXT_OBS "Provide next observation: "
    #define MSG_IMPOSSIBLE_OBS "The last observation provided is not coherent with the actual data structures\n"
    #define MSG_BEFORE_EXIT "\nExit? "
    #define endTimer if (benchmark) printf("\tTime: %fs\n", ((float)(clock() - timer))/CLOCKS_PER_SEC);
#endif

/*
#define MSG_CHOOSE_OP "Generare spazio comportamentale (c), fornire un'osservazione lineare (o), o caricare uno spazio da file (f, se gli stati sono rinominati: g)? "
#define MSG_DIAG "Eseguire una diagnosi su questa osservazione (s/n)? "
#define MSG_CHOOSE_OP "Generate behavioral space (c), input a linear observation (o), or load a space from file (f, if states have been renamed: g)? "
#define MSG_DIAG "Calculate a diagnosis upon this observation (y/n)? "
*/