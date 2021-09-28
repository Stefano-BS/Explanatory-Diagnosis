#include "header.h"
#include <locale.h>

const unsigned int eps = L'ε', mu = L'μ';

char inputDES[100] = "";
char sceltaDot='\0';
bool benchmark = false;
clock_t timer;
BehState * iniziale;

void generaStatoIniziale(void) {
    int *statiAttivi = calloc(ncomp, sizeof(int));
    int statoLink[nlink], i;
    for (i=0; i<nlink; i++)
        statoLink[i] = VUOTO;
    iniziale = generateBehState(statoLink, statiAttivi);
    iniziale->obsIndex = 0;
}

int* impostaDatiOsservazione(int *loss) {
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

void menu(void) {
    bool sc = false, exp = false, fixedObs = false;
    char op, pota, sceltaRinomina;
    BehSpace *b = NULL;
    Explainer *explainer = NULL; 

    while (true) {
        bool allow_c = !exp & !fixedObs,
            allow_o = !exp & !fixedObs,
            allow_fg = !exp & !fixedObs,
            allow_e = sc & !fixedObs & !exp,
            allow_m = sc & exp & !fixedObs,
            allow_d = sc & fixedObs;
        
        printf(MSG_MENU_INTRO);
        if (allow_c) printf(MSG_MENU_C);
        if (allow_o) printf(MSG_MENU_O);
        if (allow_fg) printf(MSG_MENU_FG);
        if (allow_e) printf(MSG_MENU_E);
        if (allow_m) printf(MSG_MENU_M);
        if (allow_d) printf(MSG_MENU_D);
        printf(MSG_MENU_END);
        getCommand(op);

        if (op == 'x') return;
        else if (op == 'c' && allow_c) {
            printf(MSG_GEN_SC);
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            beginTimer
            generaStatoIniziale();
            b = newBehSpace();
            enlargeBehavioralSpace(b, NULL, iniziale, NULL, 0);
            generateBehavioralSpace(b, iniziale, NULL, 0);
            endTimer
            printf(MSG_POTA);
            getCommand(pota);
            if (pota==INPUT_Y && b->nTrans>1) {
                int statiPrima = b->nStates, transPrima = b->nTrans;
                beginTimer
                prune(b);
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
            int loss = 0;
            int * obs = impostaDatiOsservazione(&loss);
            fixedObs = true;
            printf(MSG_GEN_SC);
            beginTimer
            generaStatoIniziale();
            b = newBehSpace();
            enlargeBehavioralSpace(b, NULL, iniziale, NULL, loss);
            generateBehavioralSpace(b, iniziale, obs, loss);
            free(obs);
            sc = true;
            endTimer
            printf(MSG_POTA);
            getCommand(pota);
            if (pota==INPUT_Y && b->nTrans>1) {
                int statiPrima = b->nStates, transPrima = b->nTrans;
                beginTimer
                prune(b);
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
            fflush(stdout);
            fflush(stdin);
            char nomeFileSC[100];
            scanf("%99s", nomeFileSC);
            fflush(stdin);
            FILE* fileSC = fopen(nomeFileSC, "rb+");
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
            explainer = makeExplainer(b);
            freeBehSpace(b);
            exp = true;
            endTimer
            if (sceltaDot==INPUT_Y) printExplainer(explainer);
        }
        else if (op=='d' && allow_d) {
            printf(MSG_DIAG_EXEC);
            beginTimer
            Regex * diagnosis = diagnostics(b, false)[0];
            if (diagnosis->regex[0] == '\0') printf("%lc\n", eps);
            else printf("%.10000s\n", diagnosis->regex);
            freeBehSpace(b);
            b = NULL;
            sc = false;
            fixedObs = false;
            endTimer
        }
        else if (op=='m' && allow_m) {
            Monitoring * monitor = explanationEngine(explainer, NULL, NULL, 0), *updatedMonitor;
            char digitazione[10];
            int oss, loss=0, *obs=NULL, sizeofObs=0;
            while (true) {
                printf(MSG_MONITORING_RESULT);
                int i;
                for (i=0; i<=loss; i++)
                    printf("%lc%d:\t%s\n", mu, i, monitor->mu[i]->lmu->regex);
                printf(MSG_NEXT_OBS);

                RETRY: scanf("%9s", digitazione);
                oss = atoi(digitazione);
                if (oss <= 0) goto RETRY;

                if (loss+1 > sizeofObs) {
                    sizeofObs += 5;
                    obs = realloc(obs, sizeofObs*sizeof(int));
                }
                obs[loss++] = oss;
                updatedMonitor = explanationEngine(explainer, monitor, obs, loss);
                if (updatedMonitor == NULL) break;
                monitor = updatedMonitor;
            }
            printf(MSG_IMPOSSIBLE_OBS);
            freeMonitoring(monitor);
            free(obs);
        }
    }
}

int main(int argc, char *argv[]) {
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
    
    if (inputDES[0]=='\0') {
        printf(MSG_DEF_AUTOMA);
        fflush(stdout);
        scanf("%99s", inputDES);
    }
	FILE* file = fopen(inputDES, "rb+");
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
    generaStatoIniziale();
    if (sceltaDot!='n') printDES(iniziale, sceltaDot != INPUT_Y);

    menu();
	return(0);
}