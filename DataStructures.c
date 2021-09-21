#include "header.h"

int nlink=0, ncomp=0;
Componente **componenti;
Link **links;

// Buffered memory allocation (reduces realloc frequency)
int *sizeofTrComp;
int sizeofCOMP=5, sizeofLINK=5;                                 // Initial sizes

void netAlloc(void){
    componenti = malloc(sizeofCOMP* sizeof(Componente*));
    sizeofTrComp = malloc(sizeofCOMP*sizeof(int));              // Array of pointers to transitions inside components' sizes
    links = malloc(sizeofLINK* sizeof(Link*));
    if (componenti==NULL || sizeofTrComp==NULL || links==NULL) printf(MSG_MEMERR);
}

BehSpace * newBehSpace(void) {
    BehSpace* b = calloc(1, sizeof(BehSpace));
    b->sizeofS=5;
    b->states = malloc(b->sizeofS* sizeof(StatoRete*));
    if (b->states==NULL) printf(MSG_MEMERR);
    return b;
}

void alloc1(BehSpace *b, char o) {   // Call before inserting anything new in any of these structures
    if (o=='s') {
        if (b->nStates+1 > b->sizeofS) {
            b->sizeofS *= 1.25;
            b->sizeofS += 5;
            StatoRete ** spazio = realloc(b->states, b->sizeofS*sizeof(StatoRete*));
            if (spazio == NULL) printf(MSG_MEMERR);
            else b->states = spazio;
        }
    } else if (o=='c') {
        if (ncomp+1 > sizeofCOMP) {
            sizeofCOMP += 5;
            Componente ** spazio = realloc(componenti, sizeofCOMP*sizeof(Componente*));
            int* sottoAlloc = realloc(sizeofTrComp, sizeofCOMP*sizeof(int));
            if ((spazio == NULL) | (sottoAlloc == NULL)) printf(MSG_MEMERR);
            else {componenti = spazio; sizeofTrComp = sottoAlloc;}
        }
    } else if (o=='l') {
        if (nlink+1 > sizeofLINK) {
            sizeofLINK += 5;
            Link ** spazio = realloc(links, sizeofLINK*sizeof(Link*));
            if (spazio == NULL) printf(MSG_MEMERR);
            else links = spazio;
        }
    }
}

void alloc1trC(Componente * comp) {
    if (comp->nTransizioni+1 > sizeofTrComp[comp->intId]) {
        sizeofTrComp[comp->intId] += 10;
        Transizione ** spazio = realloc(comp->transizioni, sizeofTrComp[comp->intId]*sizeof(Transizione*));
        if (spazio == NULL) printf(MSG_MEMERR);
        else comp->transizioni = spazio;
    }
}

Componente * nuovoComponente(void) {
    alloc1(NULL, 'c');
    Componente *nuovo = calloc(1, sizeof(Componente));
    nuovo->transizioni = calloc(5, sizeof(Transizione*));
    nuovo->intId = ncomp;
    sizeofTrComp[ncomp] = 5;
    componenti[ncomp++] = nuovo;
    return nuovo;
}

Componente* compDaId(int id) {
    int i=0;
    for (; i<ncomp; i++)
        if (componenti[i]->id == id) return componenti[i];
    printf(MSG_COMP_NOT_FOUND, id);
    return NULL;
}

Link* linkDaId(int id) {
    int i=0;
    for (; i<nlink; i++)
        if (links[i]->id == id) return links[i];
    printf(MSG_LINK_NOT_FOUND, id);
    return NULL;
}

StatoRete * generaStato(int *contenutoLink, int *statiAttivi) {
    StatoRete *s = calloc(1, sizeof(StatoRete));
    s->id = VUOTO;
    if (contenutoLink != NULL) {
        s->contenutoLink = malloc(nlink*sizeof(int));
        memcpy(s->contenutoLink, contenutoLink, nlink*sizeof(int));
        s->finale = true;
        int i;
        for (i=0; i<nlink; i++)
            s->finale &= (s->contenutoLink[i] == VUOTO);
    }
    else s->contenutoLink = NULL;
    if (statiAttivi != NULL) {
        s->statoComponenti = malloc(ncomp*sizeof(int));
        memcpy(s->statoComponenti, statiAttivi, ncomp*sizeof(int));
    }
    else s->statoComponenti = NULL;
    return s;
}

void removeState(BehSpace *b, StatoRete * delete) {
    // Paragoni tra stati per puntatori a locazione di memoria, non per id
    int deleteId = delete->id, i;
    struct ltrans * temp, *trans, *transPrima, *temp2;
    temp = delete->transizioni;
    trans = transPrima = temp2 = NULL;
    
    while (temp != NULL) { // Ciclo di rimozione di doppioni
        trans = temp->prossima;
        transPrima = trans;
        while (trans != NULL) {
            if (trans->t == temp->t) {
                temp2 = transPrima;
                transPrima->prossima = trans->prossima;
                free(trans);
                trans = temp2;
            }
            transPrima = trans;
            trans = trans->prossima;
        }
        temp = temp->prossima;
    }

    temp = delete->transizioni;
    while (temp != NULL) {
        TransizioneRete * tr = temp->t;
        StatoRete  *eliminaAncheDa = NULL;

        if (tr->da != delete && tr->a != delete)
            printf(MSG_STATE_ANOMALY, deleteId); // Situazione anomala, ma non importa, qui
        else {
            eliminaAncheDa = NULL;
            if (tr->da == delete && tr->a != delete) eliminaAncheDa = tr->a;
            else if (tr->da != delete && tr->a == delete) eliminaAncheDa = tr->da;

            if (eliminaAncheDa != NULL) {
                transPrima = temp2 = NULL;
                for (trans = eliminaAncheDa->transizioni; trans != NULL; trans = trans->prossima) {
                    if (trans->t == tr) {
                        temp2 = trans->prossima;
                        free(trans);
                        if (transPrima == NULL) eliminaAncheDa->transizioni = temp2;
                        else transPrima->prossima = temp2;
                        break;
                    }
                    else transPrima = trans;
                }
            }
            b->nTrans--;
            free(tr);
            temp->t = NULL;
        }
        temp = temp->prossima;
        free(delete->transizioni);
        delete->transizioni = temp;
    }
    freeStatoRete(delete);
    
    b->nStates--;
    memcpy(b->states+deleteId, b->states+deleteId+1, (b->nStates - deleteId)*sizeof(StatoRete*));
    for (i=0; i<b->nStates; i++)                                         // Abbasso l'id degli stati successivi
        if (b->states[i]->id>=deleteId) b->states[i]->id--;
}

void freeStatoRete(StatoRete *s) {
    if (s->contenutoLink != NULL) free(s->contenutoLink);
    if (s->statoComponenti != NULL) free(s->statoComponenti);
    if (s->transizioni != NULL) free(s->transizioni); // Struttura ricorsiva il cui contenuto deve essere già stato liberato
    free(s);
}

/* Call like:
    bool mask[b->nStates];
    memset(mask, true, b->nStates);
    BehSpace * duplicated = dup(b, mask, false); */
BehSpace * dup(BehSpace *b, bool mask[], bool silence) {
    int i, ns = 0, map[b->nStates];
    for (i=0; i<b->nStates; i++) {
        map[i] = mask[i] ? ns : -1;     // Map: id->id from old to new space
        ns = mask[i] ? ns+1 : ns;
    }
    
    BehSpace *dup = calloc(1, sizeof(BehSpace));    // Not using newBehSpace since knowing in advance exact target size
    dup->nStates = ns;                              // lets avoid realloc process and avoid wasting space
    dup->sizeofS = ns;
    dup->states = calloc(ns, sizeof(StatoRete *));

    StatoRete * s, * new;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {    // Preallocate states to have valid pointers
        if (mask[i]) dup->states[map[i]] = calloc(1, sizeof(StatoRete));
    }
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (mask[i]) {
            new = dup->states[map[i]];  // State information copy...
            new->contenutoLink = malloc(nlink*sizeof(int));
            new->statoComponenti = malloc(ncomp*sizeof(int));
            memcpy(new->contenutoLink, s->contenutoLink, nlink*sizeof(int));
            memcpy(new->statoComponenti, s->statoComponenti, ncomp*sizeof(int));
            new->finale = s->finale;
            new->indiceOsservazione = s->indiceOsservazione;
            new->id = map[i];
            struct ltrans *trans = s->transizioni, *temp;
            while (trans != NULL) {     // Transition list copy...
                TransizioneRete *t = trans->t;
                if (silence && t->t->oss != 0) new->finale = true; // A fault space state is final if final or if having observale outgoings
                else {
                    int mapA = map[t->a->id], mapDa = map[t->da->id]; 
                    if (mapA != -1 && mapDa != -1) {
                        struct ltrans *newList = calloc(1, sizeof(struct ltrans));
                        temp = new->transizioni;
                        new->transizioni = newList;
                        newList->prossima = temp;
                        if (new->id <= mapA && new->id <= mapDa) { // Alloc nt only once. Dislikes double autotransitions
                            TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                            dup->nTrans++;
                            newList->t = nt;
                            nt->a = dup->states[mapA];
                            nt->da = dup->states[mapDa];
                            nt->t = t->t;
                            nt->concreta = t->concreta;
                            nt->parentesizzata = t->parentesizzata;
                            nt->dimRegex = t->dimRegex;
                            nt->marker = t->marker;
                            if (nt->dimRegex>0) {
                                nt->regex = malloc(nt->dimRegex);
                                strcpy(nt->regex, t->regex);
                            }
                        }
                        else {  // If execution goes here, it means the TransizioneRete has alredy been created, so we search for its pointer
                            int idSt = mapA < mapDa ? mapA : mapDa;
                            temp = dup->states[idSt]->transizioni;
                            while (temp != NULL) {
                                if (temp->t->a->id == mapA && temp->t->da->id == mapDa
                                && temp->t->t == t->t) {
                                    newList->t = temp->t;
                                    break;
                                }
                                temp = temp->prossima;
                            }
                        }
                    }
                }
                trans = trans->prossima;
            }
        }
    }
    return dup;
}

void freeBehSpace(BehSpace *b) {
    int i;
    for (i=0; i<b->nStates; i++)
        removeState(b, b->states[i]);
    free(b);
}

void memCoherenceTest(BehSpace *b){
    int i;
    StatoRete *s;
    printf(MSG_MEMTEST1, b->nStates, b->nTrans);
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (s->id != i) printf(MSG_MEMTEST2, i, s->id);
        struct ltrans * lt = s->transizioni;
        while (lt != NULL) {
            if (lt->t->a->id != i && lt->t->da->id != i)
                printf(MSG_MEMTEST3, i, lt->t->da->id, lt->t->a->id);
            if (lt->t->a != s && lt->t->da != s) {
                printf(MSG_MEMTEST4, i, s);
                printf(MSG_MEMTEST5, lt->t->da, lt->t->da->id, lt->t->a, lt->t->a->id);
                printf(MSG_MEMTEST6, memcmp(lt->t->a, s, sizeof(StatoRete))==0);
                printf(MSG_MEMTEST7, memcmp(lt->t->da, s, sizeof(StatoRete))==0);
            }
            lt = lt->prossima;
        }
    }
}