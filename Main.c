#include "header.h"

const unsigned short eps = L'ε', mu = L'μ';

Regex* empty;
char inputDES[100] = "";
char sceltaDot='\0';
bool benchmark = false;
clock_t timer;
BehState * iniziale;

static void beforeExit(int signo) {
    printf(MSG_BEFORE_EXIT);
    char close;
    getCommand(close)
    if (close == INPUT_Y) exit(0);
    signal(SIGINT, beforeExit);
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
                else printf("%lc%d:\t%s\n", mu, i, monitor->mu[i]->lmu->regex);
            if (sceltaDot==INPUT_Y) printMonitoring(monitor, explainer);
            printf(MSG_NEXT_OBS);
        })

        RETRY: scanf("%9s", digitazione);
        oss = atoi(digitazione);
        if (oss <= 0) goto RETRY;

        interruptable({
            if (loss+1 > sizeofObs) {
                sizeofObs += 5;
                obs = realloc(obs, sizeofObs*sizeof(int));
            }
            obs[loss++] = oss;
            Monitoring * updatedMonitor = explanationEngine(explainer, monitor, obs, loss, lazy);
            if (updatedMonitor == NULL) break;
            monitor = updatedMonitor;
        })
    }
    printf(MSG_IMPOSSIBLE_OBS);
    freeMonitoring(monitor);
    free(obs);
}

void menu(void) {
    bool sc = false, exp = false, fixedObs = false;
    char op, pota, sceltaRinomina;
    BehSpace *b = NULL;
    Explainer *explainer = NULL; 
    bool doneSomething = true;
    while (true) {
        bool allow_c = !exp & !fixedObs,
            allow_o = !exp & !fixedObs,
            allow_fg = !exp & !fixedObs,
            allow_e = sc & !fixedObs & !exp,
            allow_m = sc & exp & !fixedObs,
            allow_d = sc & fixedObs,
            allow_l = !fixedObs & !exp;
        
        if (doneSomething) {
            printf(MSG_MENU_INTRO);
            if (allow_c) printf(MSG_MENU_C);
            if (allow_o) printf(MSG_MENU_O);
            if (allow_fg) printf(MSG_MENU_FG);
            if (allow_e) printf(MSG_MENU_E);
            if (allow_m) printf(MSG_MENU_M);
            if (allow_d) printf(MSG_MENU_D);
            if (allow_l) printf(MSG_MENU_L);
            printf(MSG_MENU_END);
        }
        getCommand(op);
        doneSomething = true;
        if (op == 'x') return;
        else if (op == 'c' && allow_c) {
            printf(MSG_GEN_SC);
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            beginTimer
            interruptable(b = BehavioralSpace(NULL, NULL, 0);)
            endTimer
            printf(MSG_POTA);
            getCommand(pota);
            if (pota==INPUT_Y && b->nTrans>1) {
                int statiPrima = b->nStates, transPrima = b->nTrans;
                beginTimer
                interruptable(prune(b);)
                endTimer
                printf(MSG_POTA_RES, statiPrima-b->nStates, transPrima-b->nTrans);
            }
            printf(MSG_SC_RES, b->nStates, b->nTrans);
            if (sceltaDot==INPUT_Y) {
                printf(MSG_RENAME_STATES);
                getCommand(sceltaRinomina);
                beginTimer
                printBehSpace(b, sceltaRinomina==INPUT_Y, false, false);
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
            printf(MSG_POTA);
            getCommand(pota);
            if (pota==INPUT_Y && b->nTrans>1) {
                int statiPrima = b->nStates, transPrima = b->nTrans;
                beginTimer
                interruptable(prune(b);)
                endTimer
                printf(MSG_POTA_RES, statiPrima-b->nStates, transPrima-b->nTrans);
            }
            printf(MSG_SC_RES, b->nStates, b->nTrans);
            if (sceltaDot==INPUT_Y) {
                printf(MSG_RENAME_STATES);
                getCommand(sceltaRinomina);
                beginTimer
                printBehSpace(b, sceltaRinomina==INPUT_Y, true, false);
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
            int loss=0;
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
            interruptable({
                explainer = makeExplainer(b);
                freeBehSpace(b);
                exp = true;
            })
            endTimer
            if (sceltaDot==INPUT_Y) printExplainer(explainer);
        }
        else if (op=='d' && allow_d) {
            printf(MSG_DIAG_EXEC);
            beginTimer
            interruptable({
                Regex * diagnosis = diagnostics(b, false)[0];
                if (diagnosis->regex[0] == '\0') printf("%lc\n", eps);
                else printf("%.10000s\n", diagnosis->regex);
                freeBehSpace(b);
                b = NULL;
                sc = false;
                fixedObs = false;
            })
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
        size_t optind;
        for (optind = 1; optind < argc; optind++) {
            if (argv[optind][0] != '-') {
                strcpy(inputDES, argv[optind]);
                continue;
            }
            switch (argv[optind][1]) {
                case 'd': sceltaDot = INPUT_Y; break;
                case 't': sceltaDot = 't'; break;
                case 'n': sceltaDot = 'n'; break;
                case 'b': benchmark = true; break;
            }   
        }
    }
    
    empty = emptyRegex(0);
    if (inputDES[0]=='\0') {
        printf(MSG_DEF_AUTOMA);
        scanf("%99s", inputDES);
    }
	FILE* file = fopen(inputDES, "r");
	if (file == NULL) {
		printf(MSG_NO_FILE, inputDES);
		return -1;
	}
    netAlloc();
    beginTimer
	parseDES(file);
	fclose(file);
    endTimer
	printf(MSG_PARS_DONE);
    if (sceltaDot == '\0') {
        printf(MSG_DOT);
        getCommand(sceltaDot);
    }
    
    if (sceltaDot!='n') {
        BehState * tmp = generateBehState(NULL, NULL);
        printDES(tmp, sceltaDot != INPUT_Y);
        freeBehState(tmp);
    }
    menu();
	return(0);
}