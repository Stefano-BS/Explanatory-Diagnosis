#include "Lang.h"
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
#define HASH_XS         181U
#define HASH_SM         773U
#define HASH_MD         2971U
#define HASH_LG         7043U
#define HASH_XL         25889U
#define HASH_HG         95063U
#define HASH_XH         500057U

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
#define hashlen(ns)                 ns<5? HASH_NO : ns<21 ? HASH_XT : ns<80? HASH_TN : ns<350? HASH_XS : ns<1450? HASH_SM : ns<5000? HASH_MD : ns<12000? HASH_LG : ns<50000? HASH_XL : ns<180000? HASH_HG : HASH_XH
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
    #define O3(f)                   f
    #define INLINEO3(f)             f
    #define RESTRICT
#else
    #define printlog(...)
    #define debugif(...)
    #define ndebugif(mode, action)  if ((DEBUG_MODE & mode) == 0) action;
    #define INLINE(f)               f //inline f __attribute__((always_inline)); inline f
    #define O3(f)                   f __attribute__((optimize("O3"))); f
    #define INLINEO3(f)             f __attribute__((optimize("O3"))); f //inline f __attribute__((always_inline)) __attribute__((optimize("O3"))); inline f
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
//extern unsigned int bucketId;               // Ovverride to local scope when in use inside threads or twice in a nested foreachst
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
BehState * insertState(BehSpace *, BehState *, bool) __attribute__((optimize("O3")));
BehState * stateById(BehSpace *, int) __attribute__((optimize("O3")));
BehSpace * newBehSpace(void);
BehState * generateBehState(short *, short *, char) __attribute__((optimize("O3")));
void removeBehState(BehSpace *, BehState *, bool);
void freeBehState(BehState *);
void freeCatalogue(void);
BehSpace * dup(BehSpace *, bool[], bool, int **, bool);
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
BehSpace * BehavioralSpace(BehState *, int *, unsigned short, char);
void prune(BehSpace *);
FaultSpace * faultSpace(FaultSpaceMaps *, BehSpace *, BehState *, BehTrans **, bool) __attribute__((optimize("O3")));
FaultSpace ** faultSpaces(FaultSpaceMaps ***, BehSpace *, unsigned int *, BehTrans ****, bool) __attribute__((optimize("O3")));
FaultSpace * makeLazyFaultSpace(Explainer *, BehState *, bool) __attribute__((optimize("O3")));
BehSpace * uncompiledMonitoring(BehSpace *, int *, unsigned short, bool) __attribute__((optimize("O3")));

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
void buildFaultsReachedWithObs(Explainer *, FaultSpace *, int, bool);

// Monitoring.c
Monitoring* explanationEngine(Explainer *, Monitoring *, int *, unsigned short, bool, bool, bool);