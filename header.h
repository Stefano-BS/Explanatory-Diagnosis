#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <locale.h>
#include <stdarg.h>
#if __STDC_VERSION__ >= 201112L
    #include <threads.h>
    #define doC11(...) __VA_ARGS__
    #define doC99(...)
#else
    #define doC11(...)
    #define doC99(...) __VA_ARGS__
#endif

#define VUOTO           -1
#define ACAPO           -10
#define REGEX           4U
#define REGEXLEVER      1.6
#define HASH_NO         1U
#define HASH_XT         11U
#define HASH_TN         43U
#define HASH_XS         89U
#define HASH_SM         181U
#define HASH_MD         773U
#define HASH_LG         2971U
#define HASH_XL         7043U
#define HASH_HG         25889U
#define HASH_XH         95063U

#define FLAG_FINAL          1
#define FLAG_SILENT_FINAL   1 << 1

#define DEBUG_REGEX         1 << 1
#define DEBUG_MEMCOH        1 << 2
#define DEBUG_FAULT_DOT     1 << 3
#define DEBUG_MON           1 << 4
#define DEBUG_DIAG          1 << 5

#define beginTimer                  gettimeofday(&beginT, NULL); beginC = clock();
#define foreachst(b, code)          for(struct sList*sl=b->sMap[bucketId=0];bucketId<b->hashLen;sl=b->sMap[++bucketId]){while (sl) {code;sl=sl->next;}}
#define foreachstb(b)               for(struct sList*sl=b->sMap[bucketId=0];bucketId<b->hashLen;sl=b->sMap[++bucketId]){while (sl) {
#define foreachstc                  ;sl=sl->next;}}
#define foreachtr(lnode, from)      for(lnode=from; lnode!=NULL; lnode = lnode->next)
#define foreachdecl(lnode, from)    for(struct ltrans *lnode=from; lnode!=NULL; lnode = lnode->next)
#define interruptable(code)         signal(SIGINT, beforeExit); code signal(SIGINT, SIG_DFL);
#ifndef M_PI
    #define M_PI                    m_pi
#endif
#define normal(mu, sigma, x)        (1.0 / (sigma * sqrt(M_PI+M_PI))) * exp(-(x-mu)*(x-mu)/(2.0*sigma*sigma))

#ifndef DEBUG_MODE
    #define DEBUG_MODE false
#endif
#if DEBUG_MODE
    #define printlog(...)           printf(__VA_ARGS__)
    #define debugif(mode, action)   if ((DEBUG_MODE & mode) == mode) action;
    #define ndebugif(...)
    #define INLINE(f)               f
    #define RESTRICT
#else
    #define printlog(...)
    #define debugif(...)
    #define ndebugif(mode, action)  if ((DEBUG_MODE & mode) == 0) action;
    #define INLINE(f)               inline f __attribute__((always_inline)); inline f
    #define RESTRICT                restrict
    #define NDEBUG
#endif
#include <assert.h>

// GENERAL
typedef struct {
    char * regex;
    unsigned int size, strlen;
    bool bracketed,         // Doesn't need do put brackets around (doesn't necessarily mean it has brackets wrap)
        concrete,           // Can't be epsilon
        brkBrk4Alt;         // Can disassemble brackets to get a|b|c instead of (a|b)|c
    char altDecomposable;   // 0: not containing alternatives; 1: is simple alternatives; 2: is recursive mixture of type 1 and 2 
} Regex;

typedef struct {
    struct tList {
        struct behtrans *t;
        struct faultspace *tempFault;
        struct tList * next;
    } **tList;
    unsigned int length;
} BehTransCatalog;

// DISCRETE EVENT SYSTEM
typedef struct {
    struct component *RESTRICT from, *RESTRICT to;
    short id, intId;
} Link;

typedef struct {
    Link *RESTRICT linkIn, **RESTRICT linkOut;
    short *RESTRICT idOutgoingEvents;
    short id, idIncomingEvent;
    unsigned short obs, fault, from, to, nOutgoingEvents, sizeofOE;
} Trans;

typedef struct component {
    Trans **RESTRICT transitions;
    short id, intId;
    unsigned short nStates, nTrans;
} Component;

// BEHAVIORAL SPACE
typedef struct behstate {
    short *RESTRICT componentStatus, *RESTRICT linkContent;
    struct ltrans *RESTRICT transitions;
    int id;
    unsigned short obsIndex;
    char flags;
} BehState;

typedef struct behtrans {
    BehState *from, *to;
    Regex *regex;
    Trans * t;
    int marker;
} BehTrans;

struct ltrans {
    BehTrans *t;
    struct ltrans *RESTRICT next;
};

typedef struct {
    struct sList {
        struct behstate *s;
        struct sList * next;
    } **sMap;
    unsigned int nStates, nTrans, hashLen;
    bool containsFinalStates;
} BehSpace;

// EXPLAINER AND DIAGNOSER
typedef struct {
    int *RESTRICT idMapToOrigin, * idMapFromOrigin, *RESTRICT exitStates;
} FaultSpaceMaps;

typedef struct faultspace {
    BehSpace *b;
    Regex ** diagnosis, *alternativeOfDiagnoses;
} FaultSpace;

typedef struct {
    FaultSpace * from, *to;
    Regex * regex;
    int fromStateId, toStateId;
    unsigned short obs, fault;
} ExplTrans;

typedef struct {
    FaultSpace **RESTRICT faults;
    ExplTrans **RESTRICT trans;
    FaultSpaceMaps **maps;
    unsigned int nFaultSpaces, nTrans, sizeofTrans, sizeofFaults;
} Explainer;

// MONITORING
typedef struct {
    FaultSpace * from, * to;
    Regex *l, *lp;
} MonitorTrans;

typedef struct {
    FaultSpace **RESTRICT expStates;
    MonitorTrans **RESTRICT arcs;
    Regex * lmu, ** lin, ** lout;
    unsigned short nExpStates, nArcs, sizeofArcs;
} MonitorState;

typedef struct {
    MonitorState ** mu;
    unsigned short length;
} Monitoring;

// MULTITHREADING
struct FaultSpaceParams {
    FaultSpace **RESTRICT ret;
    BehSpace *RESTRICT b;
    FaultSpaceMaps *RESTRICT map;
    BehState *RESTRICT s;
    BehTrans **RESTRICT obsTrs;
    bool decorateOnlyFinals;
};

// Global variables
extern double m_pi;
extern unsigned long long seed;
extern char outGraphType[6];
extern char * inputDES;
extern unsigned int strlenInputDES;
extern unsigned int bucketId;               // Ovverride to local scope when in use inside threads
extern unsigned short nlink, ncomp;
extern Component **RESTRICT components;
extern Link **RESTRICT links;
extern Regex* empty;
extern const unsigned short eps, mu;
extern BehTransCatalog catalog;

// DataStructures.c
Component * newComponent(void);
Link* linkById(short);
Component* compById(short);
unsigned int BehSpaceSizeEsteem(void);
void netAlloc(unsigned short, unsigned short);
void alloc1(void *, char);
unsigned int hashBehState(unsigned int, BehState *);
bool behTransCompareTo(BehTrans *, BehTrans *, bool, bool);
bool behStateCompareTo(BehState *, BehState *, bool, bool);
void initCatalogue(void);
BehState * catalogInsertState(BehSpace *, BehState *, bool);
BehState * stateById(BehSpace *, int);
BehSpace * newBehSpace(void);
BehState * generateBehState(short *, short *);
void removeBehState(BehSpace *, BehState *, bool);
void freeBehState(BehState *);
void freeCatalogue(void);
BehSpace * dup(BehSpace *, bool[], bool, int **);
void freeBehSpace(BehSpace *);
void freeMonitoring(Monitoring *);
void behCoherenceTest(BehSpace *);
void expCoherenceTest(Explainer *);
void monitoringCoherenceTest(Monitoring *);
// Parser.c
void parseDES(FILE*);
void netMake(unsigned short, unsigned short, float, float, float, float, unsigned short, unsigned short, float, unsigned short);
// Printer.c
void printDES(BehState *, bool);
char* printBehSpace(BehSpace *, bool, bool, int);
void printExplainer(Explainer *);
void printMonitoring(Monitoring *, Explainer *, bool);
// SpaceMaker.c
BehSpace * BehavioralSpace(BehState *, int *, unsigned short);
void prune(BehSpace *);
FaultSpace * faultSpace(FaultSpaceMaps *, BehSpace *, BehState *, BehTrans **, bool);
FaultSpace ** faultSpaces(FaultSpaceMaps ***, BehSpace *, unsigned int *, BehTrans ****, bool);
FaultSpace * makeLazyFaultSpace(Explainer *, BehState *, bool);
BehSpace * uncompiledMonitoring(BehSpace *, int *, unsigned short);
// Regex.c
void freeRegex(Regex *);
Regex * emptyRegex(unsigned int);
Regex * regexCpy(Regex *);
void regexCompile(Regex *, unsigned short);
void regexMake(Regex*, Regex*, Regex*, char, Regex *);
// Candidates.c
Regex** diagnostics(BehSpace *, char);
// Explainer.c
Explainer * makeExplainer(BehSpace *, bool);
Explainer * makeLazyExplainer(Explainer *, BehState *, bool);
Monitoring* explanationEngine(Explainer *, Monitoring *, int *, unsigned short, bool, bool);

#ifndef LANG
    #define LANG 'e'
#endif

#ifdef ABBR
    #define ABBR_BEH "χ"
    #define ABBR_EXP "𝓔"
    #define ABBR_DIAG "𝓓"
    #define ABBR_MON "𝜧"
#endif

#if LANG == 'i'
    #ifndef ABBR
        #define ABBR_BEH "Spazio Comportamentale"
        #define ABBR_EXP "Esplicatore"
        #define ABBR_DIAG "Diagnosticatore"
        #define ABBR_MON "Monitoraggio"
    #endif
    #define INPUT_Y 's'
    #define MSG_YES "si"
    #define MSG_NO "no"
    #define LOGO "    ______                     __                     ___         __                  _\n   / ____/_______  _______  __/ /_____  ________     /   | __  __/ /_____  ____ ___  (_)\n  / __/ / ___/ _ \\/ ___/ / / / __/ __ \\/ ___/ _ \\   / /| |/ / / / __/ __ \\/ __ `__ \\/ /\n / /___(__  )  __/ /__/ /_/ / /_/ /_/ / /  /  __/  / ___ / /_/ / /_/ /_/ / / / / / / /\n/_____/____/\\___/\\___/\\____/\\__/\\____/_/   \\___/  /_/  |_\\____/\\__/\\____/_/ /_/ /_/_/\n"
    #define MSG_MENU_INTRO "\nMenu\tx: Esci\n\ts: Impostazioni\n"
    #define MSG_MENU_C "\tc: Genera " ABBR_BEH "\n"
    #define MSG_MENU_O "\to: Calcola una diagnosi a posteriori relativa ad un'osservazione\n"
    #define MSG_MENU_D "\td: Genera un "ABBR_DIAG"\n"
    #define MSG_MENU_E "\te: Genera un "ABBR_EXP"\n"
    #define MSG_MENU_M "\tm: Avvia processo di monitoraggio\n"
    #define MSG_MENU_N "\tn: Avvia processo di monitoraggio (senza conoscenza compilata)\n"
    #define MSG_MENU_L "\tl: Avvia processo di monitoraggio (compilazione pigra dell'"ABBR_EXP"/"ABBR_DIAG")\n"
    #define MSG_MENU_I "\ti: Fornire la descrizione del Sistema a Eventi Discreti in ingresso\n"
    #define MSG_MENU_K "\tk: Generare automaticamente un Sistema a Eventi Discreti\n"
    #define MSG_MENU_END "Scelta: "
    #define MSG_OBS "Fornire la sequenza di etichette. Per ogni numero, a capo o spazio, per terminare usa un carattere non numerico\n"
    #define MSG_DEF_AUTOMA "Indicare il file che contiene la definizione dell'automa: "
    #define MSG_NO_FILE "File \"%s\" inesistente!\n"
    #define MSG_PARS_DONE "Scansione effettuata...\n"
    #define MSG_NET_SEED "Inserisci il seme per la generazione (0 per utilizzare il tempo di sistema): "
    #define MSG_NET_PARAMS "Fornire i parametri che il generatore dovrebbe seguire: inserire una lista intervallata da spazi. I rapporti sono frazionari, gli altri sono interi brevi senza segno.\nNumero di componenti, Media stati per componente, Rapporto di connessione interna, Rapporto di connessione esterna (Links), Rapporto di osservabilità, Rapporto di rilevanza, Gamma osservabilità, Gamma rilevanza, Rapporto eventi, Gamma eventi\n"
    #define MSG_DOT "Salvare i grafi come .dot (s), stampare testo (t) o nessun'uscita (n)? "
    #define MSG_GRAPH_FORMAT "Scegliere il formato di generazione di Graphviz (attuale: %s): "
    #define MSG_BENCH "Cronometrare le esecuzioni? (s/n)? "
    #define MSG_DOT_INPUT "Indicare il file dot generato contenete lo "ABBR_BEH": "
    #define MSG_INPUT_NOT_OBSERVATION "Lo spazio non corrisponde ad un'osservazione lineare, pertanto non si consiglia un suo utilizzo per diagnosi\n"
    #define MSG_INPUT_UNKNOWN_TYPE "Non e' possibile stabilire se lo spazio importato sia derivante da un'osservazione lineare: eseguire una diagnosi solo in caso affermativo\n"
    #define MSG_GEN_SC "Generazione "ABBR_BEH"...\n"
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
    #define MSG_MEMTEST1 ABBR_BEH ": %d stati e %d transizioni\n"
    #define MSG_MEMTEST2 "Lo stato %d non è coerente col proprio id %d\n"
    #define MSG_MEMTEST3 "Lo stato %d ha tr dall'id %d all'id %d\n"
    #define MSG_MEMTEST4 "Lo stato id %d, presso %p ha transizione che non punta a sé: "
    #define MSG_MEMTEST5 "da %p (id %d) a %p (id %d)\n"
    #define MSG_MEMTEST6 "\tmemcmp stato a: %d\n"
    #define MSG_MEMTEST7 "\tmemcmp stato da: %d\n"
    #define MSG_MEMTEST8 "Trovata TransExpl da/per chiusura inesistente\n"
    #define MSG_MEMTEST9 ABBR_EXP": %d chiusure e %d transizioni\n"
    #define MSG_MEMTEST10 "La transizione %d non è osservabile\n"
    #define MSG_MEMTEST11 "La chiusura %d ha mapToOrigin -1 in posizione %d\n"
    #define MSG_MEMTEST12 "Chiusura %d: trovato che mapFrom[mapTo[%d]] != %d\n"
    #define MSG_MEMTEST13 ABBR_MON": %u stati\n"
    #define MSG_MEMTEST14 "Stato di monitoraggio %d: %d chiusure e %d transizioni\n"
    #define MSG_MEMTEST15 "Rilevata nello stato di monitoraggio %d una chiusura senza archi uscenti\n"
    #define MSG_MEMTEST16 "Rilevata nello stato di monitoraggio %d una chiusura senza archi entranti\n"
    #define MSG_EXP_FAULT_NOT_FOUND "Non è stato possibile trovare una chiusura di destinazione ad una transizione\n"
    #define MSG_MONITORING_RESULT "\n"ABBR_MON" (traccia delle diagnosi):\n"
    #define MSG_NEXT_OBS "Fornisca l'osservazione successiva: "
    #define MSG_IMPOSSIBLE_OBS "L'ultima osservazione fornita non è coerente con le strutture dati\n"
    #define MSG_LAZY_DIAG_EXP ABBR_EXP" pigro (s) o "ABBR_DIAG" pigro (n)? "
    #define MSG_LAZY_EXPLAINER_DIFFERENCES ABBR_EXP"/"ABBR_DIAG" parziale stampato. Potrebbe mostrare, rispetto al "ABBR_EXP"/"ABBR_DIAG" completo, più stati e transizioni (interni alle chiusure) a causa dell'impossibile potatura.\n"
    #define MSG_BEFORE_EXIT "\a\nTerminare? "
    #define endTimer if (benchmark) {gettimeofday(&endT, NULL); printf("\tTempo: %lfs, Tempo CPU: %fs\n", (double)(endT.tv_sec-beginT.tv_sec)+((double)(endT.tv_usec-beginT.tv_usec))/1000000, ((float)(clock() - beginC))/CLOCKS_PER_SEC);}
#elif LANG=='e'
    #ifndef ABBR
        #define ABBR_BEH "Behavioral Space"
        #define ABBR_EXP "Explainer"
        #define ABBR_DIAG "Diagnoser"
        #define ABBR_MON "Monitoring"
    #endif
    #define INPUT_Y 'y'
    #define MSG_YES "yes"
    #define MSG_NO "no"
    #define LOGO "  ___        _                        _          _____                    _             \n / _ \\      | |                      | |        |  ___|                  | |            \n/ /_\\ \\_   _| |_ ___  _ __ ___   __ _| |_ __ _  | |____  _____  ___ _   _| |_ ___  _ __ \n|  _  | | | | __/ _ \\| '_ ` _ \\ / _` | __/ _` | |  __\\ \\/ / _ \\/ __| | | | __/ _ \\| '__|\n| | | | |_| | || (_) | | | | | | (_| | || (_| | | |___>  <  __/ (__| |_| | || (_) | |   \n\\_| |_/\\__,_|\\__\\___/|_| |_| |_|\\__,_|\\__\\__,_| \\____/_/\\_\\___|\\___|\\__,_|\\__\\___/|_|\n"
    #define MSG_MENU_INTRO "\nMenu\tx: Exit\n\ts: Settings\n"
    #define MSG_MENU_C "\tc: Generate "ABBR_BEH"\n"
    #define MSG_MENU_O "\to: Calculate a posteriori diagnosis relative to an observation\n"
    #define MSG_MENU_D "\td: Generate "ABBR_DIAG"\n"
    #define MSG_MENU_E "\te: Generate "ABBR_EXP"\n"
    #define MSG_MENU_M "\tm: Start monitoring procedure\n"
    #define MSG_MENU_N "\tn: Start monitoring procedure (without compiled knowledge)\n"
    #define MSG_MENU_L "\tl: Start monitoring procedure (lazy "ABBR_EXP"/"ABBR_DIAG" compilation)\n"
    #define MSG_MENU_I "\ti: Provide DES input description\n"
    #define MSG_MENU_K "\tk: Generate automatically a DES\n"
    #define MSG_MENU_END "Your choice: "
    #define MSG_OBS "Provide the observation sequence. A new line or space foreach number, non numeric to stop\n"
    #define MSG_DEF_AUTOMA "Where does automata's definition file locate: "
    #define MSG_NO_FILE "File \"%s\" not found!\n"
    #define MSG_PARS_DONE "Parsing done...\n"
    #define MSG_NET_SEED "Insert the generator seed (0 to use system time): "
    #define MSG_NET_PARAMS "Provide the parameter the DES generator should follow: insert a list with numbers separated by spaces. Ratios are floats, the rest are unsigned short integers.\nNumber of components, Average component states number, Connection ratio inside components, Connection ratio between components (Links), Observability ratio, Faulty ratio, Observability gamma, Faulty gamma, Event ratio, Event gamma\n"
    #define MSG_DOT "Save graphs as .dot (y), print as text (t), or no output (n)? "
    #define MSG_GRAPH_FORMAT "Choose Graphviz printing format (current: %s): "
    #define MSG_BENCH "Measure execution time? (y/n)? "
    #define MSG_DOT_INPUT "Input .dot file describing the "ABBR_BEH": "
    #define MSG_INPUT_NOT_OBSERVATION "This space is not the result of a linear observation, thus it is not recommended a diagnosis on that\n"
    #define MSG_INPUT_UNKNOWN_TYPE "It is not possible to establish if this space is the result of linear observation: execute a diagnosis just in that case\n"
    #define MSG_GEN_SC "Calculating "ABBR_BEH"...\n"
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
    #define MSG_MEMTEST1 ABBR_BEH": %d states and %d transitions\n"
    #define MSG_MEMTEST2 "State %d is not coherent with its id %d\n"
    #define MSG_MEMTEST3 "State %d has transition from id %d to id %d\n"
    #define MSG_MEMTEST4 "State id %d, located at %p has transition not pointing to itself: "
    #define MSG_MEMTEST5 "from %p (id %d) to %p (id %d)\n"
    #define MSG_MEMTEST6 "\tmemcmp state to: %d\n"
    #define MSG_MEMTEST7 "\tmemcmp state from: %d\n"
    #define MSG_MEMTEST8 "Found TransExpl from/to non existent fault space\n"
    #define MSG_MEMTEST9 ABBR_EXP": %d fault spaces and %d transitions\n"
    #define MSG_MEMTEST10 "Transition %d is not observable\n"
    #define MSG_MEMTEST11 "Fault %d has mapToOrigin -1 in position %d\n"
    #define MSG_MEMTEST12 "Fault %d: found mapFrom[mapTo[%d]] != %d\n"
    #define MSG_MEMTEST13 ABBR_MON": %u states\n"
    #define MSG_MEMTEST14 ABBR_MON" State %d: %d fault spaces and %d transitions\n"
    #define MSG_MEMTEST15 "Found that in "ABBR_MON" State %d there's a fault state non exited by any arc\n"
    #define MSG_MEMTEST16 "Found that in "ABBR_MON" State %d there's a fault state not reached by any arc\n"
    #define MSG_EXP_FAULT_NOT_FOUND "Unable to find a fault space destination for a transition\n"
    #define MSG_MONITORING_RESULT "\nExplanation Trace:\n"
    #define MSG_NEXT_OBS "Provide next observation: "
    #define MSG_IMPOSSIBLE_OBS "The last observation provided is not coherent with the actual data structures\n"
    #define MSG_LAZY_DIAG_EXP "Lazy "ABBR_EXP" (y) or lazy "ABBR_DIAG" (n)? "
    #define MSG_LAZY_EXPLAINER_DIFFERENCES "Lazy "ABBR_EXP"/"ABBR_DIAG" printed. Note that it may contain (in comparison to the full "ABBR_EXP"/"ABBR_DIAG") more states and transitions (within fault spaces) due to unfeasible pruning.\n"
    #define MSG_BEFORE_EXIT "\a\nExit? "
    #define endTimer if (benchmark) {gettimeofday(&endT, NULL); printf("\tTime: %lfs, CPU time: %fs\n", (double)(endT.tv_sec-beginT.tv_sec)+((double)(endT.tv_usec-beginT.tv_usec))/1000000, ((float)(clock() - beginC))/CLOCKS_PER_SEC);}
#elif LANG == 's'
    #ifndef ABBR
        #define ABBR_BEH "Espacio de Comportamiento"
        #define ABBR_EXP "Explicador"
        #define ABBR_DIAG "Diagnosticador"
        #define ABBR_MON "Monitorización"
    #endif
    #define INPUT_Y 's'
    #define MSG_YES "si"
    #define MSG_NO "no"
    #define LOGO " _____ _                 _              _             _         ___        _                   _\n|  ___(_)               | |            | |           | |       / _ \\      | |                 | |\n| |__  _  ___  ___ _   _| |_ __ _ _ __ | |_ ___    __| | ___  / /_\\ \\_   _| |_ _ __ ___   __ _| |_ __ _ ___ \n|  __|| |/ _ \\/ __| | | | __/ _` | '_ \\| __/ _ \\  / _` |/ _ \\ |  _  | | | | __| '_ ` _ \\ / _` | __/ _` / __|\n| |___| |  __/ (__| |_| | || (_| | | | | ||  __/ | (_| |  __/ | | | | |_| | |_| | | | | | (_| | || (_| \\__ \\\n\\____/| |\\___|\\___|\\__,_|\\__\\__,_|_| |_|\\__\\___|  \\__,_|\\___| \\_| |_/\\__,_|\\__|_| |_| |_|\\__,_|\\__\\__,_|___/\n     _/ |\n    |__/\n"
    #define MSG_MENU_INTRO "\nMenù\tx: Salida\n\ts: Configuración\n"
    #define MSG_MENU_C "\tc: Generar "ABBR_BEH"\n"
    #define MSG_MENU_O "\to: Calcula un diagnóstico a posteriori de una observación\n"
    #define MSG_MENU_D "\td: Generar un "ABBR_DIAG"\n"
    #define MSG_MENU_E "\te: Generar un "ABBR_EXP"\n"
    #define MSG_MENU_M "\tm: Iniciar el proceso de monitoreo\n"
    #define MSG_MENU_N "\tn: Iniciar el proceso de monitoreo (sin conoscimiento compilado)\n"
    #define MSG_MENU_L "\tl: Iniciar el proceso de monitoreo (compilación perezosa del "ABBR_EXP" o "ABBR_DIAG")\n"
    #define MSG_MENU_I "\ti: Proporcionar una descripción del sistema de eventos discretos entrante\n"
    #define MSG_MENU_K "\tk: Generar automáticamente un sistema de eventos discretos\n"
    #define MSG_MENU_END "Elección: "
    #define MSG_OBS "Proporcione la secuencia de etiquetas. Para cada número, retorno de carro o espacio, utilice un carácter no numérico para finalizar\n"
    #define MSG_DEF_AUTOMA "Indique el archivo que contiene la definición del autómata: "
    #define MSG_NO_FILE "Archivo \"%s\" inexistente!\n"
    #define MSG_PARS_DONE "Escaneo realizado...\n"
    #define MSG_NET_SEED "Introducir la semilla para la generación (0 para utilizar el tiempo del sistema): "
    #define MSG_NET_PARAMS "Proporcione los parámetros que debe seguir el generador: ingrese una lista intercalada con espacios. Las proporciones son fraccionarias, las otras son enteros cortos sin signo.\nNúmero de componentes, Estados medios por componente, Fracción de conexión interna, Fracción de conexión externa (enlaces), Fracción de observabilidad, Fracción de relevancia, Rango de observabilidad, Rango de relevancia, Fracción de eventos, Rango de eventos\n"
    #define MSG_DOT "Guardar gráficos como .dot (s), imprimir texto (t) o sin salida (n)? "
    #define MSG_GRAPH_FORMAT "Elige el formato de impresión de Graphviz (actual: %s): "
    #define MSG_BENCH "Cronometrar ejecuciones? (s/n)? "
    #define MSG_DOT_INPUT "Indicar el archivo dot generado que contiene el "ABBR_BEH": "
    #define MSG_INPUT_NOT_OBSERVATION "El espacio no corresponde a una observación lineal, por lo tanto no se recomienda utilizarlo para el diagnóstico.\n"
    #define MSG_INPUT_UNKNOWN_TYPE "No es posible determinar si el espacio importado se deriva de una observación lineal: hacer un diagnóstico solo si es así.\n"
    #define MSG_GEN_SC "Generación "ABBR_BEH"...\n"
    #define MSG_POTA "Realizar podas (s/n)? "
    #define MSG_POTA_RES "Podadas %d estados y %d transiciones\n"
    #define MSG_SC_RES "Espacio generado: contiene %d estados y %d transiciones\n"
    #define MSG_RENAME_STATES "Cambiar el nombre de los estados con su identificador (s/n)? "
    #define MSG_DIAG_EXEC "Realizo diagnósticos... \n"
    #define MSG_ACTUAL_STRUCTURES "\nEstructuras de datos actuales:\n"
    #define MSG_COMP_DESCRIPTION "Componente id:%d, tienes %d estados (activo: %d) y %d transiciones:\n"
    #define MSG_TRANS_DESCRIPTION "\tTransicione id:%d va desde %d a %d, observable: %d, defectuosa: %d,"
    #define MSG_TRANS_DESCRIPTION2 " no hay eventos entrantes,"
    #define MSG_TRANS_DESCRIPTION3 " evento entrante: %d en el enlace %d,"
    #define MSG_TRANS_DESCRIPTION4 " sin eventos salientes.\n"
    #define MSG_TRANS_DESCRIPTION5 " eventos salientes:\n"
    #define MSG_TRANS_DESCRIPTION6 "\t\tEvento %d en el enlace id:%d\n"
    #define MSG_LINK_DESCRIPTION1 "Enlace id:%d, vacío, conecta %d a %d\n"
    #define MSG_LINK_DESCRIPTION2 "Enlace id:%d, contiene el evento %d, conecta %d a %d\n"
    #define MSG_END_STATE_DESC "Fin del contenido. Estado final: %s\n"
    #define MSG_SOBSTITUTION_LIST "Lista de sustituciones de nombres de estado:\n"
    #define MSG_PARSERR_NOENDLINE "Línea %d - Terminador de línea esperado\n"
    #define MSG_PARSERR_TOKEN "Línea %d - Carácter inesperado: %c (esperado: %c)\n"
    #define MSG_PARSERR_LINE "Línea incorrecta: %s\n"
    #define MSG_PARSERR_TRANS_NOT_FOUND "Error: transición %d no encontrado\n"
    #define MSG_COMP_NOT_FOUND "Error: componente con id %d no encontrado\n"
    #define MSG_LINK_NOT_FOUND "Error: enlace con id %d no encontrado\n"
    #define MSG_MEMERR "Error de asignación de memoria!\n"
    #define MSG_STATE_ANOMALY "Anomalía en el estado %d\n"
    #define MSG_MEMTEST1 ABBR_BEH": %d estados y %d transiciones\n"
    #define MSG_MEMTEST2 "El estado %d no es coherente con su identificador %d\n"
    #define MSG_MEMTEST3 "El estado %d tiene tr de id %d a id %d\n"
    #define MSG_MEMTEST4 "El estado id %d, cerca %p tiene una transición que no apunta a sí misma: "
    #define MSG_MEMTEST5 "de %p (id %d) a %p (id %d)\n"
    #define MSG_MEMTEST6 "\tmemcmp estado a: %d\n"
    #define MSG_MEMTEST7 "\tmemcmp estado de: %d\n"
    #define MSG_MEMTEST8 "Encontró una TransExpl da/a cierre inexistente\n"
    #define MSG_MEMTEST9 ABBR_EXP": %d cierres y %d transiciones\n"
    #define MSG_MEMTEST10 "La transición %d no es observable\n"
    #define MSG_MEMTEST11 "El cierre %d tiene mapToOrigin -1 in en lugar %d\n"
    #define MSG_MEMTEST12 "Cierre %d: encontró que mapFrom[mapTo[%d]] != %d\n"
    #define MSG_MEMTEST13 ABBR_MON": %u estados\n"
    #define MSG_MEMTEST14 "Estado de monitoreo %d: %d cierres y %d transiciones\n"
    #define MSG_MEMTEST15 "Detectado en el estado de monitoreo %d un cierre sin arcos salientes\n"
    #define MSG_MEMTEST16 "Detectado en el estado de monitoreo %d un cierre sin arcos entrantes\n"
    #define MSG_EXP_FAULT_NOT_FOUND "No se pudo encontrar un cierre objetivo de una transición\n"
    #define MSG_MONITORING_RESULT "\nTraza de diagnósticos:\n"
    #define MSG_NEXT_OBS "Proporcione la siguiente observación: "
    #define MSG_LAZY_DIAG_EXP ABBR_EXP" perezoso (s) o "ABBR_DIAG" perezoso (n)? "
    #define MSG_IMPOSSIBLE_OBS "La última observación dada no es consistente con las estructuras de datos\n"
    #define MSG_LAZY_EXPLAINER_DIFFERENCES ABBR_EXP"/"ABBR_DIAG" parcial impreso. Podría mostrar, en comparación con el "ABBR_EXP"/"ABBR_DIAG" completo, más estados y transiciones (internas a los cierres) debido a la poda imposible.\n"
    #define MSG_BEFORE_EXIT "\a\nInterrumpir? "
    #define endTimer if (benchmark) {gettimeofday(&endT, NULL); printf("\tTiempo: %lfs, Tiempo de CPU: %fs\n", (double)(endT.tv_sec-beginT.tv_sec)+((double)(endT.tv_usec-beginT.tv_usec))/1000000, ((float)(clock() - beginC))/CLOCKS_PER_SEC);}
#endif