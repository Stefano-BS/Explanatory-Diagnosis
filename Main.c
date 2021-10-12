#include "header.h"

const unsigned short eps = L'ε', mu = L'μ';

Regex* empty;
unsigned int strlenInputDES;
char * inputDES = "";
char * comBuf = "";
char dot = '\0';
bool benchmark = false;
struct timeval beginT, endT;

char getCommand(void) {
    char com;
    if (comBuf[0] == '\0') while (!isalpha(com=getchar()));
    else com = *comBuf++;
    return com;
}

static void beforeExit(int signo) {
    printf(MSG_BEFORE_EXIT);
    char close = getCommand();
    if (close == INPUT_Y) exit(0);
    signal(SIGINT, beforeExit);
}

bool loadInputDes(void) {
    FILE* file = fopen(inputDES, "r");
    if (file == NULL) printf(MSG_NO_FILE, inputDES);
    else {
        netAlloc(0, 0);
        beginTimer
        parseDES(file);
        fclose(file);
        endTimer
        printf(MSG_PARS_DONE);
        if (dot!='n') {
            BehState * tmp = generateBehState(NULL, NULL);
            printDES(tmp, dot != INPUT_Y);
            freeBehState(tmp);
        }
        return true;
    }
    return false;
}

int* impostaDatiOsservazione(unsigned short *loss) {
    int oss, sizeofBuf = 10;
    int *obs = calloc(10, sizeof(int));
    printf(MSG_OBS);
    while (true) {
        char digitazione[10];
        scanf("%9s", digitazione);
        oss = atoi(digitazione);
        if (oss <= 0) break;
        if (*loss+1 > sizeofBuf) {
            sizeofBuf += 10;
            obs = realloc(obs, sizeofBuf*sizeof(int));
        }
        obs[(*loss)++] = oss;
    }
    return obs;
}

void driveMonitoring(Explainer * explainer, Monitoring *monitor, bool lazy) {
    char digitazione[10];
    int oss, *obs=NULL, sizeofObs=0;
    unsigned short loss = 0;
    while (true) {
        interruptable({
            printf(MSG_MONITORING_RESULT);
            unsigned short i;
            for (i=0; i<=loss; i++)
                if (monitor->mu[i]->lmu->regex[0]=='\0') printf("%lc%d:\t%lc\n", mu, i, eps);
                else printf("%lc%d:\t%.1000s\n", mu, i, monitor->mu[i]->lmu->regex);
        })

        if (dot==INPUT_Y) printMonitoring(monitor, explainer);
        printf(MSG_NEXT_OBS);
        RETRY: scanf("%9s", digitazione);
        oss = atoi(digitazione);
        if (oss <= 0) goto RETRY;
        if (loss+1 > sizeofObs) {
            sizeofObs += 5;
            obs = realloc(obs, sizeofObs*sizeof(int));
        }
        obs[loss++] = oss;

        interruptable({
            beginTimer
            Monitoring * updatedMonitor = explanationEngine(explainer, monitor, obs, loss, lazy);
            if (updatedMonitor == NULL) break;
            monitor = updatedMonitor;
            endTimer
        })
    }
    printf(MSG_IMPOSSIBLE_OBS);
    freeMonitoring(monitor);
    free(obs);
}

void menu(void) {
    bool in = inputDES[0]!='\0', sc = false, exp = false, fixedObs = false;
    char op, doPrune, doRename;
    BehSpace *b = NULL;
    Explainer *explainer = NULL; 
    bool doneSomething = true;
    if (in) in = loadInputDes();
    while (true) {
        bool allow_c = in & !exp & !fixedObs,
            allow_o = in & !exp & !fixedObs,
            allow_fg = in & !exp & !fixedObs,
            allow_e = in & sc & !fixedObs & !exp,
            allow_m = in & sc & exp & !fixedObs,
            allow_d = in & sc & fixedObs,
            allow_l = in & !exp & !fixedObs,
            allow_i = !in;
        
        if (doneSomething) {
            printf(MSG_MENU_INTRO);
            if (allow_c) printf(MSG_MENU_C);
            if (allow_o) printf(MSG_MENU_O);
            if (allow_fg) printf(MSG_MENU_FG);
            if (allow_e) printf(MSG_MENU_E);
            if (allow_m) printf(MSG_MENU_M);
            if (allow_d) printf(MSG_MENU_D);
            if (allow_l) printf(MSG_MENU_L);
            if (allow_i) printf(MSG_MENU_I);
            if (allow_i) printf(MSG_MENU_K);
            printf(MSG_MENU_END);
        }
        op = getCommand();
        doneSomething = true;
        if (op == 'x') return;
        if (op == 's') {
            printf(MSG_DOT);
            dot = getCommand();
            printf(MSG_BENCH);
            benchmark = getCommand() == INPUT_Y;
        }
        else if (op == 'i' && allow_i) {
            printf(MSG_DEF_AUTOMA);
            inputDES = malloc(100);
            scanf("%99s", inputDES);
            strlenInputDES = strlen(inputDES);
            in = loadInputDes();
        }
        else if (op == 'k' && allow_i) {
            in = true;
            printf(MSG_NET_PARAMS);
            unsigned short nofComp, compSize, obsGamma, faultGamma, eventGamma;
            float connectionRatio, linkRatio, obsRatio, faultRatio, eventRatio;
            scanf(" %hu %hu %f %f %f %f %hu %hu %f %hu", &nofComp, &compSize, &connectionRatio, &linkRatio, &obsRatio, &faultRatio, &obsGamma, &faultGamma, &eventRatio,&eventGamma);
            int seed = netMake(nofComp, compSize, connectionRatio, linkRatio, obsRatio, faultRatio, obsGamma, faultGamma, eventRatio , eventGamma);
            if (dot!='n') {
                inputDES = malloc(20);
                sprintf(inputDES, "gen/Seed%d", seed);
                BehState * tmp = generateBehState(NULL, NULL);
                printDES(tmp, dot != INPUT_Y);
                freeBehState(tmp);
            }
        }
        else if (op == 'c' && allow_c) {
            printf(MSG_GEN_SC);
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            beginTimer
            interruptable(b = BehavioralSpace(NULL, NULL, 0);)
            endTimer
            printf(MSG_POTA);
            doPrune = getCommand();
            if (doPrune==INPUT_Y && b->nTrans>1) {
                unsigned int statiPrima = b->nStates, transPrima = b->nTrans;
                beginTimer
                interruptable(prune(b);)
                endTimer
                printf(MSG_POTA_RES, statiPrima-b->nStates, transPrima-b->nTrans);
            }
            printf(MSG_SC_RES, b->nStates, b->nTrans);
            if (dot==INPUT_Y) {
                printf(MSG_RENAME_STATES);
                doRename = getCommand();
                beginTimer
                printBehSpace(b, doRename==INPUT_Y, false, false);
                endTimer
            }
            sc = true;
        }
        else if (op == 'o' && allow_o) {
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            unsigned short loss = 0;
            int * obs = impostaDatiOsservazione(&loss);
            fixedObs = true;
            printf(MSG_GEN_SC);
            beginTimer
            interruptable(b = BehavioralSpace(NULL, obs, loss);)
            free(obs);
            sc = true;
            endTimer
            if (b->nTrans>1) {
                unsigned int statiPrima = b->nStates, transPrima = b->nTrans;
                beginTimer
                interruptable(prune(b);)
                endTimer
                printf(MSG_POTA_RES, statiPrima-b->nStates, transPrima-b->nTrans);
            }
            printf(MSG_SC_RES, b->nStates, b->nTrans);
            if (dot==INPUT_Y) {
                printf(MSG_RENAME_STATES);
                doRename = getCommand();
                beginTimer
                printBehSpace(b, doRename==INPUT_Y, true, false);
                endTimer
            }
        }
        else if (allow_fg && (op=='f' || op=='g')) {
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            printf(MSG_DOT_INPUT);
            char nomeFileSC[100];
            scanf("%99s", nomeFileSC);
            FILE* fileSC = fopen(nomeFileSC, "r");
            if (fileSC == NULL) {
                printf(MSG_NO_FILE, nomeFileSC);
                return;
            }
            beginTimer
            unsigned short loss=0;
            b = parseBehSpace(fileSC, op=='g', &loss);
            fclose(fileSC);
            endTimer
            if (loss==0 && op=='f') printf(MSG_INPUT_NOT_OBSERVATION);
            if (op=='g') printf(MSG_INPUT_UNKNOWN_TYPE);
            sc = true;
            fixedObs = true;
        }
        else if (op=='e' && allow_e) {
            beginTimer
            interruptable(
                explainer = makeExplainer(b);
                freeBehSpace(b);
                exp = true;
            )
            endTimer
            if (dot==INPUT_Y) printExplainer(explainer);
        }
        else if (op=='d' && allow_d) {
            printf(MSG_DIAG_EXEC);
            beginTimer
            interruptable(
                Regex * diagnosis = diagnostics(b, false)[0];
                if (diagnosis->regex[0] == '\0') printf("%lc\n", eps);
                else printf("%.10000s\n", diagnosis->regex);
                freeBehSpace(b);
                b = NULL;
                sc = false;
                fixedObs = false;
            )
            endTimer
        }
        else if (op=='m' && allow_m) {
            Monitoring * monitor = explanationEngine(explainer, NULL, NULL, 0, false);
            driveMonitoring(explainer, monitor, false);
        }
        else if (op=='l' && allow_l) {
            exp = sc = true;
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            explainer = makeLazyExplainer(NULL, generateBehState(NULL, NULL));
            Monitoring * monitor = explanationEngine(explainer, NULL, NULL, 0, true);
            driveMonitoring(explainer, monitor, true);
            if (dot==INPUT_Y) {
                printExplainer(explainer);
                printf(MSG_LAZY_EXPLAINER_DIFFERENCES);
            }
        }
        else {
            printf("\a");
            doneSomething = false;
        }
    }
}

int main(int argc, char *argv[]) {
    doC11(setbuf(stdout, NULL);)
    setlocale(LC_ALL, "");
    printf(LOGO);
    if (argc >1) {
        for (int optind = 1; optind < argc; optind++) {
            if (argv[optind][0] != '-') {
                inputDES = argv[optind];
                strlenInputDES = strlen(inputDES);
                continue;
            }
            switch (argv[optind][1]) {
                case 'd': dot = INPUT_Y; break;
                case 't': dot = 't'; break;
                case 'n': dot = 'n'; break;
                case 'b': benchmark = true; break;
                case 'c': 
                    if (optind < argc-1) comBuf = argv[++optind];
                    break;
            }   
        }
    }
    if (dot == '\0') {
        printf(MSG_DOT);
        dot = getCommand();
    }
    
    empty = emptyRegex(0);
    menu();
	return 0;
}