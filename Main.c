#include "header.h"

char nomeFile[100] = "";
int *osservazione, loss=0;

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

int main(int argc, char *argv[]) {
    printf(LOGO);
    BehSpace *b = calloc(1, sizeof(BehSpace));
    char sceltaDot='\0', sceltaOperazione, pota, nomeFileSC[100], sceltaDiag, sceltaRinomina;
    bool benchmark = false;
    clock_t inizio;
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
    allocamentoIniziale(b);
    inizioTimer
	parse(file);
	fclose(file);
    fineTimer
	printf(MSG_PARS_DONE);
    int *statiAttivi = calloc(ncomp, sizeof(int));
    int statoLink[nlink], i;
    for (i=0; i<nlink; i++)
        statoLink[i] = VUOTO;
    StatoRete * iniziale = generaStato(statoLink, statiAttivi);
    iniziale->indiceOsservazione = 0;
    
    if (sceltaDot == '\0') {
        printf(MSG_DOT);
        ottieniComando(&sceltaDot);
    }
    if (sceltaDot!='n') stampaStruttureAttuali(iniziale, sceltaDot != INPUT_Y);
    printf(MSG_CHOOSE_OP);
    ottieniComando(&sceltaOperazione);
    if (sceltaOperazione=='o') {
        impostaDatiOsservazione();
        iniziale->finale = false;
    } else if (sceltaOperazione=='f' || sceltaOperazione=='g') {
        printf(MSG_DOT_INPUT);
        fflush(stdout);
        fflush(stdin);
        scanf("%99s", nomeFileSC);
        fflush(stdin);
        FILE* fileSC = fopen(nomeFileSC, "rb+");
        if (fileSC == NULL) {
            printf(MSG_NO_FILE, nomeFile);
            return -1;
        }
        inizioTimer
        parseDot(b, fileSC, sceltaOperazione=='g');
        fclose(fileSC);
        fineTimer
        if (loss==0 && sceltaOperazione=='f') printf(MSG_INPUT_NOT_OBSERVATION);
        if (sceltaOperazione=='g') printf(MSG_INPUT_UNKNOWN_TYPE);
    }
    if (sceltaOperazione != 'f' && sceltaOperazione!='g') {
        printf(MSG_GEN_SC);
        inizioTimer
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
            stampaSpazioComportamentale(b, sceltaRinomina==INPUT_Y);
            fineTimer
        }
    }
    if (sceltaOperazione=='f' || sceltaOperazione=='g' || (sceltaOperazione=='o' && pota==INPUT_Y)) {
        printf(MSG_DIAG);
        fflush(stdout);
        ottieniComando(&sceltaDiag);
        if (sceltaDiag==INPUT_Y) {
            printf(MSG_DIAG_EXEC);
            inizioTimer
            printf("%.10000s\n", diagnostica(b));
            fineTimer
        }
    }
	return(0);
}