#include "header.h"

// ANALISI LESSICALE FILE IN INGRESSO
int linea = 0;

void match(int c, FILE* input) {
    if (c==ACAPO) {
        char c1 = fgetc(input);
        if (c1 == '\n') return;
        else if (c1 == '\r') 
            if (fgetc(input) != '\n') printf(MSG_PARSERR_NOENDLINE, linea);
    } else {
        char cl = fgetc(input);
        if (cl != c) printf(MSG_PARSERR_TOKEN, linea, cl, c);
    }
}

void parse(FILE* file) {
    // NOTA: prima definizione componenti
    // Segue la definizione dei link
    // Infine, la definizione delle transizioni
    char sezione[16], c;
    while (fgets(sezione, 15, file) != NULL) {
        linea++;
        if (strcmp(sezione,"//COMPONENTI\n")==0 || strcmp(sezione,"//COMPONENTI\r\n")==0) {
            while (!feof(file)) { //Per ogni componente
                if  ((c=fgetc(file)) == '/') { //Vuol dire che siamo sforati nella sezione dopo
                    ungetc(c, file);
                    break;
                }
                ungetc(c, file); //Rimettiamo dentro il pezzo di intero
                
                linea++;
                Componente *nuovo = nuovoComponente();
                fscanf(file, "%d", &(nuovo->nStati));
                match(',', file);
                fscanf(file, "%d", &(nuovo->id));
                if (!feof(file)) match(ACAPO, file);
            }
        } else if (strcmp(sezione,"//TRANSIZIONI\n")==0 || strcmp(sezione,"//TRANSIZIONI\r\n")==0) {
            while (!feof(file)) {
                if  ((c=fgetc(file)) == '/') {
                    ungetc(c, file);
                    break;
                }
                ungetc(c, file);

                linea++;
                Transizione *nuovo = calloc(1, sizeof(Transizione));
                int temp, componentePadrone;
                nuovo->idEventiU = calloc(2, sizeof(int));
                nuovo->linkU = calloc(2, sizeof(Link*));
                nuovo->sizeofEvU = 2;
                fscanf(file, "%d", &(nuovo->id));
                match(',', file);
                fscanf(file, "%d", &componentePadrone);
                match(',', file);
                fscanf(file, "%d", &(nuovo->da));
                match(',', file);
                fscanf(file, "%d", &(nuovo->a));
                match(',', file);
                fscanf(file, "%d", &(nuovo->oss));
                match(',', file);
                fscanf(file, "%d", &(nuovo->ril));
                match(',', file);
                if (isdigit(c = fgetc(file))) {                     // Se dopo la virgola c'è un numero, allora c'è un evento in ingresso
                    ungetc(c, file);
                    fscanf(file, "%d", &(nuovo->idEventoIn));
                    match(':', file);                               // idEvento:idLink
                    fscanf(file, "%d", &temp);
                    nuovo->linkIn = linkDaId(temp);
                } else {
                    nuovo->idEventoIn = VUOTO;
                    nuovo->linkIn = NULL;
                    ungetc(c, file);
                }
                match('|', file);                                   // Altrimenti se c'è | allora l'evento in ingresso è nullo
                
                while (!feof(file) && (c=fgetc(file)) != '\n') {    // Creazione lista di eventi in uscita
                    if (feof(file)) break;
                    ungetc(c, file);
                    
                    fscanf(file, "%d", &temp);
                    if (nuovo->nEventiU +1 > nuovo->sizeofEvU) {    // Allocazione bufferizzata della memoria anche in questo caso
                        nuovo->sizeofEvU += 5;
                        nuovo->idEventiU = realloc(nuovo->idEventiU, nuovo->sizeofEvU*sizeof(int));
                        nuovo->idEventiU = realloc(nuovo->idEventiU, nuovo->sizeofEvU*sizeof(int));
                    }
                    nuovo->idEventiU[nuovo->nEventiU] = temp;

                    match(':', file); //idEvento:idLink
                    fscanf(file, "%d", &temp);
                    Link *linkNuovo = linkDaId(temp);
                    nuovo->linkU[nuovo->nEventiU] = linkNuovo;
                    nuovo->nEventiU++;
                    match(',', file);
                }
                if (!feof(file)) ungetc(c, file);
                
                Componente *padrone = compDaId(componentePadrone);  // Aggiunta della neonata transizione al relativo componente
                alloc1trC(padrone);
                padrone->transizioni[padrone->nTransizioni] = nuovo;
                padrone->nTransizioni++;

                if (!feof(file)) match(ACAPO, file);
            }
        } else if (strcmp(sezione,"//LINK\n")==0 || strcmp(sezione,"//LINK\r\n")==0) {
            while (!feof(file)) {
                if  ((c=fgetc(file)) == '/') {
                    ungetc(c, file);
                    break;
                }
                ungetc(c, file);
                
                linea++;
                Link *nuovo = calloc(1, sizeof(Link));
                int idComp1, idComp2;
                fscanf(file, "%d", &idComp1);
                match(',', file);
                fscanf(file, "%d", &idComp2);
                match(',', file);
                fscanf(file, "%d", &(nuovo->id));
                nuovo->intId = nlink;
                nuovo->da = compDaId(idComp1);
                nuovo->a = compDaId(idComp2);
                alloc1(NULL, 'l');
                links[nlink++] = nuovo;

                if (!feof(file)) match(ACAPO, file); 
            }
        } else if (!feof(file))
            printf(MSG_PARSERR_LINE, sezione);
    }
}

BehSpace * parseDot(FILE * file, bool semplificata) {
    BehSpace * b = newBehSpace();
    int i;
    char buffer[50];
    fgets(buffer, 49, file);   // Intestazione
    if (!semplificata) {
        char nomeStatiTrovati[1000][50];        // Attenzione, limite arbitrario
        while (true) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "node") != 0) break;
            fscanf(file, "%s", buffer);
            bool dbc = strcmp(buffer, "[shape=doublecircle];")==0;
            fscanf(file, "%s", buffer);
            strcpy(nomeStatiTrovati[b->nStates], buffer);
            int strl = strlen(buffer), attivi[ncomp], clink[nlink], oss=-1, sez=0, j=1;
            for (i=1; i<strl; i++) {
                if (buffer[i] == '_') {
                    sez++;
                    j=i+2;
                    continue;
                }
                if (buffer[i] == 'L') continue;
                if (buffer[i] == 'O') continue;
                if (sez==0) attivi[i-j] = buffer[i] - '0';
                if (sez==1) clink[i-j] = buffer[i] == 'e' ? VUOTO : buffer[i] - '0';
                if (sez==2) {
                    oss = buffer[i] - '0';
                    loss = (loss>oss? loss : oss);
                }
            }
            StatoRete * nuovo = generaStato(clink, attivi);
            nuovo->id = b->nStates;
            nuovo->flags = dbc;
            nuovo->indiceOsservazione = oss;
            alloc1(b, 's');
            b->states[b->nStates++] = nuovo;
            fscanf(file, "%s", buffer); // Simbolo ;
        }
        while (true) {
            StatoRete *statoDa, *statoA;
            int j;
            for (i=0; i<b->nStates; i++)
                if (strcmp(nomeStatiTrovati[i], buffer) == 0) {
                    statoDa = b->states[i]; 
                    break;
                }
            fscanf(file, "%s", buffer); // Simbolo ->
            fscanf(file, "%s", buffer);
            for (i=0; i<b->nStates; i++)
                if (strcmp(nomeStatiTrovati[i], buffer) == 0) {
                    statoA = b->states[i]; 
                    break;
                }
            
            fscanf(file, "%s", buffer); // Label
            int idTransizione = atoi(buffer+8);
            Transizione *t = NULL;
            for (i=0; i<ncomp; i++) {
                for (j=0; j<componenti[i]->nTransizioni; j++) {
                    if (componenti[i]->transizioni[j]->id == idTransizione) {
                        t = componenti[i]->transizioni[j];
                        break;
                    }
                }
                if (t != NULL) break;
            }

            TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
            nuovaTransRete->a = statoA;
            nuovaTransRete->da = statoDa;
            nuovaTransRete->t = t;
            nuovaTransRete->regex = NULL;
            struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->prossima = statoDa->transizioni;
            statoDa->transizioni = nuovaLTr;
            if (statoA != statoDa) {
                nuovaLTr = calloc(1, sizeof(struct ltrans));
                nuovaLTr->t = nuovaTransRete;
                nuovaLTr->prossima = statoA->transizioni;
                statoA->transizioni = nuovaLTr;
            }
            b->nTrans++;

            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "}") == 0) break;
        }
    } else {
        while (true) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "node") != 0) break;
            fscanf(file, "%s", buffer);
            bool dbc = strcmp(buffer, "[shape=doublecircle];")==0;
            fscanf(file, "%s", buffer); // Trattandosi di una sequenza S0 S1 ... Sn il contenuto è prevedibile
            
            StatoRete * nuovo = generaStato(NULL, NULL);
            nuovo->id = b->nStates;
            nuovo->flags = dbc;
            alloc1(b, 's');
            b->states[b->nStates++] = nuovo;
            fscanf(file, "%s", buffer); // Simbolo ;
        }
        while (true) {
            int idDa = atoi(buffer+1), j;
            fscanf(file, "%s", buffer); // Simbolo ->
            fscanf(file, "%s", buffer);
            int idA = atoi(buffer+1);
            fscanf(file, "%s", buffer); // Label
            int idTransizione = atoi(buffer+8);
            Transizione *t = NULL;
            for (i=0; i<ncomp; i++) {
                for (j=0; j<componenti[i]->nTransizioni; j++) {
                    if (componenti[i]->transizioni[j]->id == idTransizione) {
                        t = componenti[i]->transizioni[j];
                        break;
                    }
                }
                if (t != NULL) break;
            }
            if (t==NULL) printf(MSG_PARSERR_TRANS_NOT_FOUND, idTransizione);
            
            TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
            nuovaTransRete->regex = NULL;
            nuovaTransRete->a = b->states[idA];
            nuovaTransRete->da = b->states[idDa];
            nuovaTransRete->t = t;
            struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->prossima = b->states[idDa]->transizioni;
            b->states[idDa]->transizioni = nuovaLTr;
            if (idA != idDa) {
                nuovaLTr = calloc(1, sizeof(struct ltrans));
                nuovaLTr->t = nuovaTransRete;
                nuovaLTr->prossima = b->states[idA]->transizioni;
                b->states[idA]->transizioni = nuovaLTr;
            }
            b->nTrans++;

            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "}") == 0) break;
        }
    }
    if (DEBUG_MODE) behCoherenceTest(b);
    return b;
}