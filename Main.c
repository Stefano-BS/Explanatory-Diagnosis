#include "header.h"
#include <locale.h>

const unsigned int eps = L'ε';

char nomeFile[100] = "";
char sceltaDot='\0';
int *osservazione, loss=0;
bool benchmark = false;
clock_t inizio;
StatoRete * iniziale;

void generaStatoIniziale(void) {
    int *statiAttivi = calloc(ncomp, sizeof(int));
    int statoLink[nlink], i;
    for (i=0; i<nlink; i++)
        statoLink[i] = VUOTO;
    iniziale = generaStato(statoLink, statiAttivi);
    iniziale->indiceOsservazione = 0;
}

void impostaDatiOsservazione(void) {
    int oss, sizeofBuf = 10;
    osservazione = calloc(10, sizeof(int));
    printf(MSG_OBS);
    while (true) {
        char digitazione[10];
        scanf("%9s", digitazione);
        oss = atoi(digitazione);
        if (oss <= 0) break;
        if (loss+1 > sizeofBuf) {
            sizeofBuf += 10;
            osservazione = realloc(osservazione, sizeofBuf*sizeof(int));
        }
        osservazione[loss++] = oss;
    }
}

void freeOss(void) {
    free(osservazione);
    osservazione = NULL;
    loss = 0;
}

void menu(void) {
    bool sc = false, exp = false, fixedObs = false;
    char op, pota, sceltaRinomina;
    BehSpace *b = NULL;
    Explainer *explainer = NULL; 

    while (true) {
        printf(MSG_MENU_INTRO);
        if (!fixedObs) printf(MSG_MENU_C);
        if (!fixedObs) printf(MSG_MENU_O);
        if (!fixedObs) printf(MSG_MENU_FG);
        if (sc & !fixedObs & !exp) printf(MSG_MENU_E);
        if (sc & fixedObs) printf(MSG_MENU_D);
        printf(MSG_MENU_END);
        ottieniComando(&op);

        if (op == 'x') return;
        else if (op == 'c' && !fixedObs) {
            printf(MSG_GEN_SC);
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            if (osservazione != NULL) {freeOss(); fixedObs = false; osservazione=NULL;}
            inizioTimer
            generaStatoIniziale();
            b = newBehSpace();
            ampliaSpazioComportamentale(b, NULL, iniziale, NULL);
            generaSpazioComportamentale(b, iniziale);
            fineTimer
            printf(MSG_POTA);
            ottieniComando(&pota);
            if (pota==INPUT_Y && b->nTrans>1) {
                int statiPrima = b->nStates, transPrima = b->nTrans;
                inizioTimer
                potatura(b);
                fineTimer
                printf(MSG_POTA_RES, statiPrima-b->nStates, transPrima-b->nTrans);
            }
            printf(MSG_SC_RES, b->nStates, b->nTrans);
            if (sceltaDot==INPUT_Y) {
                printf(MSG_RENAME_STATES);
                ottieniComando(&sceltaRinomina);
                inizioTimer
                stampaSpazioComportamentale(b, sceltaRinomina==INPUT_Y, false);
                fineTimer
            }
            sc = true;
        }
        else if (op == 'o' && !fixedObs) {
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            if (osservazione != NULL) {freeOss(); fixedObs = false; osservazione=NULL;}
            impostaDatiOsservazione();
            fixedObs = true;
            printf(MSG_GEN_SC);
            inizioTimer
            generaStatoIniziale();
            b = newBehSpace();
            ampliaSpazioComportamentale(b, NULL, iniziale, NULL);
            generaSpazioComportamentale(b, iniziale);
            sc = true;
            fineTimer
            printf(MSG_POTA);
            ottieniComando(&pota);
            if (pota==INPUT_Y && b->nTrans>1) {
                int statiPrima = b->nStates, transPrima = b->nTrans;
                inizioTimer
                potatura(b);
                fineTimer
                printf(MSG_POTA_RES, statiPrima-b->nStates, transPrima-b->nTrans);
            }
            printf(MSG_SC_RES, b->nStates, b->nTrans);
            if (sceltaDot==INPUT_Y) {
                printf(MSG_RENAME_STATES);
                ottieniComando(&sceltaRinomina);
                inizioTimer
                stampaSpazioComportamentale(b, sceltaRinomina==INPUT_Y, false);
                fineTimer
            }
        }
        else if (!fixedObs && (op=='f' || op=='g')) {
            if (b != NULL) {freeBehSpace(b); b=NULL;}
            if (osservazione != NULL) {freeOss(); fixedObs = false; osservazione=NULL;}
            printf(MSG_DOT_INPUT);
            fflush(stdout);
            fflush(stdin);
            char nomeFileSC[100];
            scanf("%99s", nomeFileSC);
            fflush(stdin);
            FILE* fileSC = fopen(nomeFileSC, "rb+");
            if (fileSC == NULL) {
                printf(MSG_NO_FILE, nomeFile);
                return;
            }
            inizioTimer
            b = parseDot(fileSC, op=='g');
            fclose(fileSC);
            fineTimer
            if (loss==0 && op=='f') printf(MSG_INPUT_NOT_OBSERVATION);
            if (op=='g') printf(MSG_INPUT_UNKNOWN_TYPE);
            sc = true;
            fixedObs = true;
        }
        else if (op=='e' && !exp && sc && !fixedObs) {
            inizioTimer
            explainer = makeExplainer(b);
            freeBehSpace(b);
            exp = true;
            fineTimer
            if (sceltaDot==INPUT_Y) printExplainer(explainer);
        }
        else if (fixedObs && sc && op=='d') {
            printf(MSG_DIAG_EXEC);
            inizioTimer
            char * diagnosis = diagnostica(b, false)[0];
            if (diagnosis[0] == '\0') printf("%lc\n", eps);
            else printf("%.10000s\n", diagnosis);
            freeBehSpace(b);
            b = NULL;
            sc = false;
            freeOss();
            fixedObs = false;
            fineTimer
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
                strcpy(nomeFile, argv[optind]);
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
    
    if (nomeFile[0]=='\0') {
        printf(MSG_DEF_AUTOMA);
        fflush(stdout);
        scanf("%99s", nomeFile);
    }
	FILE* file = fopen(nomeFile, "rb+");
	if (file == NULL) {
		printf(MSG_NO_FILE, nomeFile);
		return -1;
	}
    netAlloc();
    inizioTimer
	parse(file);
	fclose(file);
    fineTimer
	printf(MSG_PARS_DONE);
    if (sceltaDot == '\0') {
        printf(MSG_DOT);
        ottieniComando(&sceltaDot);
    }
    generaStatoIniziale();
    if (sceltaDot!='n') stampaStruttureAttuali(iniziale, sceltaDot != INPUT_Y);

    menu();
	return(0);
}