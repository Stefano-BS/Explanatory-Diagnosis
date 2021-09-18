#include "header.h"

int nlink=0, ncomp=0, nStatiS=0, nTransSp=0;
Componente **componenti;
Link **links;
StatoRete ** statiSpazio;

// Rendo bufferizzata la gestione della memoria heap per (tutte) le struttre dati (strutture di puntatori e strutture a contatori)
int *sizeofTrComp;
int sizeofSR=50, sizeofCOMP=5, sizeofLINK=5;                    // Dimensioni di partenza

void allocamentoIniziale(void){
    componenti = malloc(sizeofCOMP* sizeof(Componente*));                   // Puntatori ai componenti
    sizeofTrComp = malloc(sizeofCOMP*sizeof(int));                          // Dimensioni degli array puntatori a transizioni nei componenti
    links = malloc(sizeofLINK* sizeof(Link*));                              // Puntatori ai link
    statiSpazio = malloc(sizeofSR* sizeof(StatoRete*));                     // Puntatori agli stati rete
}

void alloc1(char o) {   // Gestione strutture globali (livello zero)
    if (o=='s') {
        if (nStatiS+1 > sizeofSR) {
            sizeofSR += 50;
            StatoRete ** spazio = realloc(statiSpazio, sizeofSR*sizeof(StatoRete*));
            if (spazio == NULL) printf(MSG_MEMERR);
            else statiSpazio = spazio;
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

void aggiungiTransizioneInCoda(StatoRete *s, TransizioneRete *t) {
    struct ltrans *ptr = s->transizioni;
    while (ptr != NULL) ptr = ptr->prossima;
    struct ltrans *nuovoElem = calloc(1, sizeof(struct ltrans));
    ptr->prossima = nuovoElem;
    nuovoElem->t = t;
}

Componente * nuovoComponente(void) {
    alloc1('c');
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

void eliminaStato(int i) {
    // Paragoni tra stati per puntatori a locazione di memoria, non per id
    StatoRete *s = statiSpazio[i];
    struct ltrans * temp, *trans, *transPrima, *temp2;
    temp = s->transizioni;
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

    temp = s->transizioni;
    while (temp != NULL) {
        TransizioneRete * tr = temp->t;
        StatoRete  *eliminaAncheDa = NULL;

        if (tr->da != s && tr->a != s)
            printf(MSG_STATE_ANOMALY, i); // Situazione anomala, ma non importa, qui
        else {
            eliminaAncheDa = NULL;
            if (tr->da == s && tr->a != s) eliminaAncheDa = tr->a;
            else if (tr->da != s && tr->a == s) eliminaAncheDa = tr->da;

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
            nTransSp--;
            free(tr);
            temp->t = NULL;
        }
        temp = temp->prossima;
        free(s->transizioni);
        s->transizioni = temp;
    }
    freeStatoRete(s);
    
    nStatiS--;
    memcpy(statiSpazio+i, statiSpazio+i+1, (nStatiS-i)*sizeof(StatoRete*));
    int j=0;
    for (; j<nStatiS; j++)                                         // Abbasso l'id degli stati successivi
        if (statiSpazio[j]->id>=i) statiSpazio[j]->id--;
}

void freeStatoRete(StatoRete *s) {
    if (s->contenutoLink != NULL) free(s->contenutoLink);
    if (s->statoComponenti != NULL) free(s->statoComponenti);
    if (s->transizioni != NULL) free(s->transizioni); // Struttura ricorsiva il cui contenuto deve essere già stato liberato
    free(s);
}

void memCoherenceTest(void){
    int i;
    StatoRete *s;
    printf(MSG_MEMTEST1, nStatiS, nTransSp);
    for (s=statiSpazio[i=0]; i<nStatiS; s=statiSpazio[++i]) {
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