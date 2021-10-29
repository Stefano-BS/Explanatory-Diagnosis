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

void parseDES(FILE* file) {
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
                Component *nuovo = newComponent();
                fscanf(file, "%hu", &(nuovo->nStates));
                match(',', file);
                fscanf(file, "%hd", &(nuovo->id));
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
                Trans *nuovo = calloc(1, sizeof(Trans));
                unsigned short componentePadrone;
                short temp;
                nuovo->idOutgoingEvents = calloc(2, sizeof(short));
                nuovo->linkOut = calloc(2, sizeof(Link*));
                nuovo->sizeofOE = 2;
                fscanf(file, "%hd", &(nuovo->id));
                match(',', file);
                fscanf(file, "%hu", &componentePadrone);
                match(',', file);
                fscanf(file, "%hu", &(nuovo->from));
                match(',', file);
                fscanf(file, "%hu", &(nuovo->to));
                match(',', file);
                fscanf(file, "%hu", &(nuovo->obs));
                match(',', file);
                fscanf(file, "%hu", &(nuovo->fault));
                match(',', file);
                if (isdigit(c = fgetc(file))) {                     // Se dopo la virgola c'è un numero, allora c'è un evento in ingresso
                    ungetc(c, file);
                    fscanf(file, "%hd", &(nuovo->idIncomingEvent));
                    match(':', file);                               // idEvento:idLink
                    fscanf(file, "%hd", &temp);
                    nuovo->linkIn = linkById(temp);
                } else {
                    nuovo->idIncomingEvent = VUOTO;
                    nuovo->linkIn = NULL;
                    ungetc(c, file);
                }
                match('|', file);                                   // Altrimenti se c'è | allora l'evento in ingresso è nullo
                
                while (!feof(file) && (c=fgetc(file)) != '\n') {    // Creazione lista di eventi in uscita
                    if (feof(file)) break;
                    ungetc(c, file);
                    
                    fscanf(file, "%hd", &temp);
                    if (nuovo->nOutgoingEvents +1 > nuovo->sizeofOE) {    // Allocazione bufferizzata della memoria anche in questo caso
                        nuovo->sizeofOE += 5;
                        nuovo->idOutgoingEvents = realloc(nuovo->idOutgoingEvents, nuovo->sizeofOE*sizeof(short));
                        nuovo->linkOut = realloc(nuovo->linkOut, nuovo->sizeofOE*sizeof(Link*));
                    }
                    nuovo->idOutgoingEvents[nuovo->nOutgoingEvents] = temp;

                    match(':', file); //idEvento:idLink
                    fscanf(file, "%hd", &temp);
                    Link *linkNuovo = linkById(temp);
                    nuovo->linkOut[nuovo->nOutgoingEvents] = linkNuovo;
                    nuovo->nOutgoingEvents++;
                    match(',', file);
                }
                if (!feof(file)) ungetc(c, file);
                
                Component *padrone = compById(componentePadrone);  // Aggiunta della neonata transizione al relativo componente
                alloc1(padrone, 't');
                padrone->transitions[padrone->nTrans] = nuovo;
                padrone->nTrans++;

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
                unsigned short idComp1, idComp2;
                fscanf(file, "%hd", &idComp1);
                match(',', file);
                fscanf(file, "%hd", &idComp2);
                match(',', file);
                fscanf(file, "%hd", &(nuovo->id));
                nuovo->intId = nlink;
                nuovo->from = compById(idComp1);
                nuovo->to = compById(idComp2);
                alloc1(NULL, 'l');
                links[nlink++] = nuovo;

                if (!feof(file)) match(ACAPO, file); 
            }
        } else if (!feof(file))
            printf(MSG_PARSERR_LINE, sezione);
    }
}

BehSpace * parseBehSpace(FILE * file, bool semplificata, unsigned short* loss) {
    BehSpace * b = newBehSpace();
    unsigned int i;
    char buffer[50];
    fgets(buffer, 49, file);   // Intestazione
    if (!semplificata) {
        char nomeStatiTrovati[1000][50];        // Attenzione, limite arbitrario
        while (true) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "node") != 0) break;
            fscanf(file, "%s", buffer);
            bool dbc = strcmp(buffer, "[style=filled")==0;
            if (dbc) fscanf(file, "%s", buffer); // fillcolor="#FFEEEE"];
            fscanf(file, "%s", buffer);
            strcpy(nomeStatiTrovati[b->nStates], buffer);
            unsigned short strl = strlen(buffer), sez=0, j=1;
            short oss=-1, attivi[ncomp], clink[nlink];
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
                    *loss = (*loss>oss? *loss : oss);
                }
            }
            BehState * nuovo = generateBehState(clink, attivi);
            nuovo->id = b->nStates;
            nuovo->flags = dbc;
            nuovo->obsIndex = oss;
            catalogInsertState(b, nuovo, false);
            b->nStates++;
            fscanf(file, "%s", buffer); // Simbolo ;
        }
        while (true) {
            BehState *statoDa, *statoA;
            unsigned int j;
            for (i=0; i<b->nStates; i++)
                if (strcmp(nomeStatiTrovati[i], buffer) == 0) {
                    statoDa = stateById(b, i); 
                    break;
                }
            fscanf(file, "%s", buffer); // Simbolo ->
            fscanf(file, "%s", buffer);
            for (i=0; i<b->nStates; i++)
                if (strcmp(nomeStatiTrovati[i], buffer) == 0) {
                    statoA = stateById(b, i); 
                    break;
                }
            
            fscanf(file, "%s", buffer); // Label
            short idTransizione = atoi(buffer+8);
            Trans *t = NULL;
            for (i=0; i<ncomp; i++) {
                for (j=0; j<components[i]->nTrans; j++) {
                    if (components[i]->transitions[j]->id == idTransizione) {
                        t = components[i]->transitions[j];
                        break;
                    }
                }
                if (t != NULL) break;
            }

            BehTrans * nuovaTransRete = calloc(1, sizeof(BehTrans));
            nuovaTransRete->to = statoA;
            nuovaTransRete->from = statoDa;
            nuovaTransRete->t = t;
            nuovaTransRete->regex = NULL;
            struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->next = statoDa->transitions;
            statoDa->transitions = nuovaLTr;
            if (statoA != statoDa) {
                nuovaLTr = calloc(1, sizeof(struct ltrans));
                nuovaLTr->t = nuovaTransRete;
                nuovaLTr->next = statoA->transitions;
                statoA->transitions = nuovaLTr;
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
            bool dbc = strcmp(buffer, "[style=filled")==0;
            if (dbc) fscanf(file, "%s", buffer); // fillcolor="#FFEEEE"];
            fscanf(file, "%s", buffer); // Trattandosi di una sequenza S0 S1 ... Sn il contenuto è prevedibile
            
            BehState * nuovo = generateBehState(NULL, NULL);
            nuovo->id = b->nStates;
            nuovo->flags = dbc;
            catalogInsertState(b, nuovo, false);
            b->nStates++;
            fscanf(file, "%s", buffer); // Simbolo ;
        }
        while (true) {
            short idDa = atoi(buffer+1), j;
            fscanf(file, "%s", buffer); // Simbolo ->
            fscanf(file, "%s", buffer);
            short idA = atoi(buffer+1);
            fscanf(file, "%s", buffer); // Label
            short idTransizione = atoi(buffer+8);
            Trans *t = NULL;
            for (i=0; i<ncomp; i++) {
                for (j=0; j<components[i]->nTrans; j++) {
                    if (components[i]->transitions[j]->id == idTransizione) {
                        t = components[i]->transitions[j];
                        break;
                    }
                }
                if (t != NULL) break;
            }
            if (t==NULL) printf(MSG_PARSERR_TRANS_NOT_FOUND, idTransizione);
            
            BehTrans * nuovaTransRete = calloc(1, sizeof(BehTrans));
            nuovaTransRete->regex = NULL;
            nuovaTransRete->to = stateById(b, idA);
            nuovaTransRete->from = stateById(b, idDa);
            nuovaTransRete->t = t;
            struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->next = nuovaTransRete->from->transitions;
            nuovaTransRete->from->transitions = nuovaLTr;
            if (idA != idDa) {
                nuovaLTr = calloc(1, sizeof(struct ltrans));
                nuovaLTr->t = nuovaTransRete;
                nuovaLTr->next = nuovaTransRete->to->transitions;
                nuovaTransRete->to->transitions = nuovaLTr;
            }
            b->nTrans++;

            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "}") == 0) break;
        }
    }
    debugif(DEBUG_MEMCOH, behCoherenceTest(b))
    return b;
}

double gauss(double mu, double sigma) {
    double u1, u2;
    do {
        u1 = (double)rand()/RAND_MAX;
        u2 = (double)rand()/RAND_MAX;
    }
    while (u1 < __DBL_EPSILON__);
    return sigma * sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2) + mu; //double z1  = sigma * sqrt(-2.0 * log(u1)) * sin(2.0 * M_PI * u2) + mu;
}

INLINE(bool componentContainsTr(Component * c, Trans *tr)) {
    unsigned short i;
    if (c->transitions == NULL || c->nTrans == 0) return false;
    for (Trans *t = c->transitions[i=0]; i<c->nTrans; t = c->transitions[++i])
        if (t->from == tr->from && t->to == tr->to) return true;
    return false;
}

INLINE(bool linkExists(Link *lk)) {
    unsigned short i;
    for (Link *l = links[i=0]; i<nlink; l = links[++i])
        if (l->from == lk->from && l->to == lk->to) return true;
    return false;
}

INLINE(void setTransAttributes(Trans * nt, float obsRatio, float faultRatio, unsigned short obsGamma, unsigned short faultGamma)) {
    if (faultRatio>0 && faultGamma>0) nt->fault = ((float)rand())/RAND_MAX < faultRatio ? rand() % faultGamma+1 : 0; else nt->fault = 0;
    if (obsRatio>0 && obsGamma>0) nt->obs = ((float)rand())/RAND_MAX < obsRatio ? rand() % obsGamma+1 : 0; else nt->obs = 0;
    nt->idIncomingEvent = VUOTO;
    nt->linkIn = NULL;
    nt->sizeofOE = 0;
    nt->nOutgoingEvents = 0;
    nt->idOutgoingEvents = NULL;
    nt->linkOut = NULL;
}

void netMake(unsigned short nofComp, unsigned short compSize, float connectionRatio, float linkRatio, float obsRatio, float faultRatio, unsigned short obsGamma, unsigned short faultGamma, float eventProb, unsigned short eventGamma) {
    srand((unsigned int) seed);
    unsigned short desiredNlink = round(nofComp*(nofComp-1)*linkRatio);
    netAlloc(nofComp, desiredNlink);
    for(unsigned short i=0; i<nofComp; i++) {
        Component * c = newComponent();
        c->intId = c->id = i;
        unsigned short ns = round(gauss(compSize, 1));
        ns = ns == 0 ? 1: ns;
        c->nStates = ns;
        unsigned short desiderTrs = round(ns*(ns-1)*connectionRatio);
        bool reachable[ns];
        memset(reachable, false, ns*sizeof(bool));
        reachable[0] = true;
        for (unsigned short j=0; j<ns-1; j++) {  // Make the component's states reachable
            unsigned short extractedSrc, extractedDest;
            do extractedSrc = rand() % ns;
            while (!reachable[extractedSrc]);
            do extractedDest = rand() % ns;
            while (reachable[extractedDest]);
            reachable[extractedDest] = true;
            Trans * nt = calloc(1, sizeof(Trans));
            nt->from = extractedSrc;
            nt->to = extractedDest;
            setTransAttributes(nt, obsRatio, faultRatio, obsGamma, faultGamma);
            alloc1(c, 't');
            c->transitions[c->nTrans] = nt;
            c->nTrans++;
        }
        for (unsigned short j=ns-1; j<desiderTrs; j++) {
            Trans * nt = calloc(1, sizeof(Trans));
            nt->id = j;
            do {
                nt->from = rand()%ns;
                nt->to = rand()%ns;
            } while(componentContainsTr(c, nt));
            setTransAttributes(nt, obsRatio, faultRatio, obsGamma, faultGamma);
            alloc1(c, 't');
            c->transitions[c->nTrans] = nt;
            c->nTrans++;
        }
    }
    for (unsigned short i=0; i<desiredNlink; i++) {
        Link * l = malloc(sizeof(Link));
        l->id = l->intId = i;
        links[i] = l;
        do {
            l->from = components[rand()%ncomp];
            l->to = components[rand()%ncomp];
        } while (linkExists(l));
        nlink++;
    }
    if (eventProb>0 && eventGamma>0) {
        unsigned short i, j;
        float oEvProb = eventProb*(1-eventProb);
        for (Component * c=components[i=0]; i<ncomp; c=components[++i])
            for (Trans * t=c->transitions[j=0]; j<c->nTrans; t=c->transitions[++j]) {
                t->idIncomingEvent = ((float)rand())/RAND_MAX < eventProb ? rand() % eventGamma+1 : VUOTO;
                if (t->idIncomingEvent != VUOTO) t->linkIn = links[rand() % nlink];
                short genEvent = VUOTO;
                while ((genEvent = ((float)rand())/RAND_MAX < oEvProb ? rand() % eventGamma+1 : VUOTO) != VUOTO) {
                    t->sizeofOE++;
                    if (t->idOutgoingEvents == NULL) t->idOutgoingEvents = malloc(sizeof(short));
                    else t->idOutgoingEvents = realloc(t->idOutgoingEvents, t->sizeofOE*sizeof(short));
                    t->idOutgoingEvents[t->nOutgoingEvents] = genEvent;
                    if (t->linkOut == NULL) t->linkOut = malloc(sizeof(Link*));
                    else t->linkOut = realloc(t->linkOut, t->sizeofOE*sizeof(Link*));
                    t->linkOut[t->nOutgoingEvents] = links[rand() % nlink];
                    t->nOutgoingEvents++;
                }
            }
    }
}