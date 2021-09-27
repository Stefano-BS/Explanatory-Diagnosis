#include "header.h"

int nlink=0, ncomp=0;
Component **components;
Link **links;

// Buffered memory allocation (reduces realloc frequency)
int *sizeofTrComp;
int sizeofCOMP=5, sizeofLINK=5;                                 // Initial sizes

void netAlloc(void){
    components = malloc(sizeofCOMP* sizeof(Component*));
    sizeofTrComp = malloc(sizeofCOMP*sizeof(int));              // Array of pointers to transitions inside components' sizes
    links = malloc(sizeofLINK* sizeof(Link*));
    if (components==NULL || sizeofTrComp==NULL || links==NULL) printf(MSG_MEMERR);
}

BehSpace * newBehSpace(void) {
    BehSpace* b = calloc(1, sizeof(BehSpace));
    b->sizeofS=5;
    b->states = malloc(b->sizeofS* sizeof(BehState*));
    if (b->states==NULL) printf(MSG_MEMERR);
    return b;
}

void alloc1(void *ptr, char o) {   // Call before inserting anything new in any of these structures
    switch (o) {
        case 's': {
            BehSpace * b = (BehSpace*) ptr;
            if (b->nStates+1 > b->sizeofS) {
                b->sizeofS *= 1.25;
                b->sizeofS += 5;
                BehState ** spazio = realloc(b->states, b->sizeofS*sizeof(BehState*));
                if (spazio == NULL) printf(MSG_MEMERR);
                else b->states = spazio;
            }
        } break;
        case 'c': {
            if (ncomp+1 > sizeofCOMP) {
                sizeofCOMP += 5;
                Component ** spazio = realloc(components, sizeofCOMP*sizeof(Component*));
                int* sottoAlloc = realloc(sizeofTrComp, sizeofCOMP*sizeof(int));
                if ((spazio == NULL) | (sottoAlloc == NULL)) printf(MSG_MEMERR);
                else {components = spazio; sizeofTrComp = sottoAlloc;}
            }
        } break;
        case 'l': {
            if (nlink+1 > sizeofLINK) {
                sizeofLINK += 5;
                Link ** spazio = realloc(links, sizeofLINK*sizeof(Link*));
                if (spazio == NULL) printf(MSG_MEMERR);
                else links = spazio;
            }
        } break;
        case 't': {
            Component * comp = (Component*) ptr;
            if (comp->nTransizioni+1 > sizeofTrComp[comp->intId]) {
                sizeofTrComp[comp->intId] += 10;
                Trans ** spazio = realloc(comp->transizioni, sizeofTrComp[comp->intId]*sizeof(Trans*));
                if (spazio == NULL) printf(MSG_MEMERR);
                else comp->transizioni = spazio;
            }
        } break;
        case 'e': {
            Explainer * exp = (Explainer *) ptr;
            if (exp->nTrans+1 > exp->sizeofTrans) {
                exp->sizeofTrans += 10;
                ExplTrans ** array = realloc(exp->trans, exp->sizeofTrans*sizeof(ExplTrans*));
                if (array == NULL) printf(MSG_MEMERR);
                else exp->trans = array;
            }
        } break;
        case 'm': {
            MonitorState * mu = (MonitorState *) ptr;
            if (mu->nArcs+1 > mu->sizeofArcs) {
                mu->sizeofArcs += 10;
                MonitorTrans ** array = realloc(mu->arcs, mu->sizeofArcs*sizeof(MonitorTrans*));
                if (array == NULL) printf(MSG_MEMERR);
                else mu->arcs = array;
            }
        } break;
    }
}

Component * newComponent(void) {
    alloc1(NULL, 'c');
    Component *nuovo = calloc(1, sizeof(Component));
    nuovo->transizioni = calloc(5, sizeof(Trans*));
    nuovo->intId = ncomp;
    sizeofTrComp[ncomp] = 5;
    components[ncomp++] = nuovo;
    return nuovo;
}

Component* compById(int id) {
    int i=0;
    for (; i<ncomp; i++)
        if (components[i]->id == id) return components[i];
    printf(MSG_COMP_NOT_FOUND, id);
    return NULL;
}

Link* linkById(int id) {
    int i=0;
    for (; i<nlink; i++)
        if (links[i]->id == id) return links[i];
    printf(MSG_LINK_NOT_FOUND, id);
    return NULL;
}

BehState * generateBehState(int *contenutoLink, int *statiAttivi) {
    BehState *s = calloc(1, sizeof(BehState));
    s->id = VUOTO;
    if (contenutoLink != NULL) {
        s->contenutoLink = malloc(nlink*sizeof(int));
        memcpy(s->contenutoLink, contenutoLink, nlink*sizeof(int));
        s->flags = FLAG_FINAL;
        int i;
        for (i=0; i<nlink; i++)
            s->flags &= (s->contenutoLink[i] == VUOTO); // Works because there are no other flags set
    }
    else s->contenutoLink = NULL;
    if (statiAttivi != NULL) {
        s->statoComponenti = malloc(ncomp*sizeof(int));
        memcpy(s->statoComponenti, statiAttivi, ncomp*sizeof(int));
    }
    else s->statoComponenti = NULL;
    return s;
}

void removeBehState(BehSpace *b, BehState * delete) {
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
        BehTrans * tr = temp->t;
        BehState  *eliminaAncheDa = NULL;

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
    freeBehState(delete);
    
    b->nStates--;
    memcpy(b->states+deleteId, b->states+deleteId+1, (b->nStates - deleteId)*sizeof(BehState*));
    for (i=0; i<b->nStates; i++)                                         // Abbasso l'id degli stati successivi
        if (b->states[i]->id>=deleteId) b->states[i]->id--;
}

void freeBehState(BehState *s) {
    if (s->contenutoLink != NULL) free(s->contenutoLink);
    if (s->statoComponenti != NULL) free(s->statoComponenti);
    if (s->transizioni != NULL) free(s->transizioni); // Struttura ricorsiva il cui contenuto deve essere già stato liberato
    free(s);
}

/* Call like:
    bool mask[b->nStates];
    memset(mask, true, b->nStates);
    BehSpace * duplicated = dup(b, mask, false); */
BehSpace * dup(BehSpace *b, bool mask[], bool silence, int** map) {
    int i, ns = 0;
    if (map == NULL) {
        int *temp = malloc(b->nStates*sizeof(int));
        map = &temp;
    }
    //else *map = malloc(b->nStates*sizeof(int));
    for (i=0; i<b->nStates; i++) {
        (*map)[i] = mask[i] ? ns : -1;     // Map: id->id from old to new space
        ns = mask[i] ? ns+1 : ns;
    }
    
    BehSpace *dup = calloc(1, sizeof(BehSpace));    // Not using newBehSpace since knowing in advance exact target size
    dup->nStates = ns;                              // lets avoid realloc process and avoid wasting space
    dup->sizeofS = ns;
    dup->states = calloc(ns, sizeof(BehState *));

    BehState * s, * new;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {    // Preallocate states to have valid pointers
        if (mask[i]) dup->states[(*map)[i]] = calloc(1, sizeof(BehState));
    }
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (mask[i]) {
            new = dup->states[(*map)[i]];  // State information copy...
            new->contenutoLink = malloc(nlink*sizeof(int));
            new->statoComponenti = malloc(ncomp*sizeof(int));
            memcpy(new->contenutoLink, s->contenutoLink, nlink*sizeof(int));
            memcpy(new->statoComponenti, s->statoComponenti, ncomp*sizeof(int));
            new->flags = s->flags;
            new->indiceOsservazione = s->indiceOsservazione;
            new->id = (*map)[i];
            struct ltrans *trans = s->transizioni, *temp;
            while (trans != NULL) {     // Transition list copy...
                BehTrans *t = trans->t;
                if (!silence || t->t->oss == 0) {
                    int mapA = (*map)[t->a->id], mapDa = (*map)[t->da->id]; 
                    if (mapA != -1 && mapDa != -1) {
                        struct ltrans *newList = calloc(1, sizeof(struct ltrans));
                        temp = new->transizioni;
                        new->transizioni = newList;
                        newList->prossima = temp;
                        if (new->id <= mapA && new->id <= mapDa) { // Alloc nt only once. Dislikes double autotransitions
                            BehTrans *nt = calloc(1, sizeof(BehTrans));
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
                        else {  // If execution goes here, it means the BehTrans has alredy been created, so we search for its pointer
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
        removeBehState(b, b->states[i]);
    free(b);
}

void behCoherenceTest(BehSpace *b){
    int i;
    BehState *s;
    printf(MSG_MEMTEST1, b->nStates, b->nTrans);
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (s->id != i) printf(MSG_MEMTEST2, i, s->id);
        foreachdecl(lt, s->transizioni) {
            if (lt->t->a->id != i && lt->t->da->id != i)
                printf(MSG_MEMTEST3, i, lt->t->da->id, lt->t->a->id);
            if (lt->t->a != s && lt->t->da != s) {
                printf(MSG_MEMTEST4, i, s);
                printf(MSG_MEMTEST5, lt->t->da, lt->t->da->id, lt->t->a, lt->t->a->id);
                printf(MSG_MEMTEST6, memcmp(lt->t->a, s, sizeof(BehState))==0);
                printf(MSG_MEMTEST7, memcmp(lt->t->da, s, sizeof(BehState))==0);
            }
        }
    }
}

void expCoherenceTest(Explainer *exp){
    int i, j;
    FaultSpace * s;
    printf(MSG_MEMTEST9, exp->nFaultSpaces, exp->nTrans);
    for (s=exp->faults[i=0]; i<exp->nFaultSpaces; s=exp->faults[++i]) {
        behCoherenceTest(s->b);
        for (j=0; j<s->b->nStates; j++) {
            if (s->idMapToOrigin[j] == -1) printf(MSG_MEMTEST11, i, j);
            if (s->idMapFromOrigin[s->idMapToOrigin[j]] != j) printf(MSG_MEMTEST12, i, j, j);
        }
        //int z;for(z=0;z<s->b->nStates;z++)printf("%d ",s->idMapToOrigin[z]);printf("\n");for(z=0;z<12;z++)printf("%d ",s->idMapFromOrigin[z]);printf("\n");
    }
    
    ExplTrans * tr;
    for (tr=exp->trans[i=0]; i<exp->nTrans; tr=exp->trans[++i]) {
        if (tr->obs == 0) printf(MSG_MEMTEST10, i);
        bool okFrom=false, okTo=false;
        for (s=exp->faults[j=0]; j<exp->nFaultSpaces; s=exp->faults[++j]) {
            okFrom |= tr->from == s;
            okTo |= tr->to == s;
        }
        if (!okFrom || !okTo) printf(MSG_MEMTEST8);
    }
}

void monitoringCoherenceTest(Monitoring *mon){
    int i, j, k;
    MonitorState * mu;
    printf(MSG_MEMTEST13, mon->length);
    for (mu=mon->mu[i=0]; i<mon->length; mu=mon->mu[++i]) {
        printf(MSG_MEMTEST14, i, mu->nExpStates, mu->nArcs);
        if (i<mon->length-1) { // Check every fault has exiting arc
            FaultSpace *f;
            for (f=mu->expStates[j=0]; j<mu->nExpStates; f=mu->expStates[++j]) {
                MonitorTrans * te;
                bool okDeparture = false;
                for (te=mu->arcs[k=0]; k<mu->nArcs; te=mu->arcs[++k]) 
                    if (te->from == f) okDeparture = true;
                if (!okDeparture) printf(MSG_MEMTEST15, i);
            }
        }
        if (i>0) { // Check every fault has entering arc
            FaultSpace *f;
            for (f=mu->expStates[j=0]; j<mu->nExpStates; f=mu->expStates[++j]) {
                MonitorTrans * te;
                bool okArrival = false;
                for (te=mon->mu[i-1]->arcs[k=0]; k<mon->mu[i-1]->nArcs; te=mon->mu[i-1]->arcs[++k]) 
                    if (te->to == f) okArrival = true;
                if (!okArrival) printf(MSG_MEMTEST16, i);
            }
        }
    }
}