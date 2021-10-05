#include "header.h"

int nlink=0, ncomp=0;
Component **RESTRICT components;
Link **RESTRICT links;

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
    b->sizeofS=1;
    b->states = malloc(b->sizeofS* sizeof(BehState*));
    if (b->states==NULL) printf(MSG_MEMERR);
    return b;
}

/*  c: Component
    e: ExplTrans in an Explainer
    f: FaultSpace in an Explainer
    l: Link
    m: MonitorTrans in a MonitorState
    s: BehState in BehSpace
    t: Trans in a Component
*/
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
            if (comp->nTrans+1 > sizeofTrComp[comp->intId]) {
                sizeofTrComp[comp->intId] += 10;
                Trans ** spazio = realloc(comp->transitions, sizeofTrComp[comp->intId]*sizeof(Trans*));
                if (spazio == NULL) printf(MSG_MEMERR);
                else comp->transitions = spazio;
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
        case 'f': {
            Explainer * exp = (Explainer *) ptr;
            if (exp->nFaultSpaces+1 > exp->sizeofFaults) {
                exp->sizeofFaults += 5;
                FaultSpace ** array = realloc(exp->faults, exp->sizeofFaults*sizeof(FaultSpace*));
                if (array == NULL) printf(MSG_MEMERR);
                else exp->faults = array;
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
    nuovo->transitions = calloc(5, sizeof(Trans*));
    nuovo->intId = ncomp;
    sizeofTrComp[ncomp] = 5;
    components[ncomp++] = nuovo;
    return nuovo;
}

Component* compById(int id) {
    for (int i=0; i<ncomp; i++)
        if (components[i]->id == id) return components[i];
    printf(MSG_COMP_NOT_FOUND, id);
    return NULL;
}

Link* linkById(int id) {
    for (int i=0; i<nlink; i++)
        if (links[i]->id == id) return links[i];
    printf(MSG_LINK_NOT_FOUND, id);
    return NULL;
}

int hashBehState(BehState *s) {
    int hash = 0;
    for (int i=0; i<ncomp; i++) hash = ((hash << 3) + s->componentStatus[i]) % catalog.length;
    for (int i=0; i<nlink; i++) hash = ((hash << 3) + s->linkContent[i]+1) % catalog.length;
    return hash;
}

bool behTransCompareTo(BehTrans * t1, BehTrans *t2) {
    return  t1->marker == t2->marker && t1->t == t2->t
            && behStateCompareTo(t1->from, t2->from) && behStateCompareTo(t1->to, t2->to);
}

INLINE(bool behStateCompareTo(BehState * s1, BehState * s2)) {
    return  // s1->flags == s2->flags && s1->obsIndex == s2->obsIndex &&
            memcmp(s1->componentStatus, s2->componentStatus, ncomp*sizeof(int)) == 0
            && memcmp(s1->linkContent, s2->linkContent, nlink*sizeof(int)) == 0;
}

BehState * generateBehState(int *RESTRICT linkContent, int *RESTRICT componentStatus) {
    BehState *s = calloc(1, sizeof(BehState));
    s->id = VUOTO;
    s->componentStatus = malloc(ncomp*sizeof(int));
    s->linkContent = malloc(nlink*sizeof(int));
    if (linkContent != NULL) {
        memcpy(s->linkContent, linkContent, nlink*sizeof(int));
        s->flags = FLAG_FINAL;
        for (int i=0; i<nlink; i++)
            s->flags &= (s->linkContent[i] == VUOTO); // Works because there are no other flags set
    }
    else memset(s->linkContent, VUOTO, nlink*sizeof(int));
    if (componentStatus != NULL) memcpy(s->componentStatus, componentStatus, ncomp*sizeof(int));
    else memset(s->componentStatus, 0, ncomp*sizeof(int));
    return s;
}

void removeBehState(BehSpace *RESTRICT b, BehState *RESTRICT delete) {
    // Paragoni tra stati per puntatori a locazione di memoria, non per id
    int deleteId = delete->id, i;
    struct ltrans * temp, *trans, *transPrima, *temp2;
    temp = delete->transitions;
    trans = transPrima = temp2 = NULL;
    
    while (temp != NULL) { // Ciclo di rimozione di doppioni
        trans = temp->next;
        transPrima = trans;
        while (trans != NULL) {
            if (trans->t == temp->t) {
                temp2 = transPrima;
                transPrima->next = trans->next;
                free(trans);
                trans = temp2;
            }
            transPrima = trans;
            trans = trans->next;
        }
        temp = temp->next;
    }

    temp = delete->transitions;
    while (temp != NULL) {
        BehTrans * tr = temp->t;
        BehState  *eliminaAncheDa = NULL;

        if (tr->from != delete && tr->to != delete)
            printf(MSG_STATE_ANOMALY, deleteId); // Situazione anomala, ma non importa, qui
        else {
            eliminaAncheDa = NULL;
            if (tr->from == delete && tr->to != delete) eliminaAncheDa = tr->to;
            else if (tr->from != delete && tr->to == delete) eliminaAncheDa = tr->from;

            if (eliminaAncheDa != NULL) {
                transPrima = temp2 = NULL;
                for (trans = eliminaAncheDa->transitions; trans != NULL; trans = trans->next) {
                    if (trans->t == tr) {
                        temp2 = trans->next;
                        free(trans);
                        if (transPrima == NULL) eliminaAncheDa->transitions = temp2;
                        else transPrima->next = temp2;
                        break;
                    }
                    else transPrima = trans;
                }
            }
            b->nTrans--;
            free(tr);
            temp->t = NULL;
        }
        temp = temp->next;
        free(delete->transitions);
        delete->transitions = temp;
    }
    freeBehState(delete);
    
    b->nStates--;
    memcpy(b->states+deleteId, b->states+deleteId+1, (b->nStates - deleteId)*sizeof(BehState*));
    for (i=0; i<b->nStates; i++)                                         // Abbasso l'id degli stati successivi
        if (b->states[i]->id>=deleteId) b->states[i]->id--;
}

void freeBehState(BehState *s) {
    if (s->linkContent != NULL) free(s->linkContent);
    if (s->componentStatus != NULL) free(s->componentStatus);
    if (s->transitions != NULL) free(s->transitions); // Struttura ricorsiva il cui contenuto deve essere già stato liberato
    free(s);
}

/* Call like:
    bool mask[b->nStates];
    memset(mask, true, b->nStates);
    BehSpace * duplicated = dup(b, mask, false); */
BehSpace * dup(BehSpace *RESTRICT b, bool mask[], bool silence, int**RESTRICT map) {
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
            new->linkContent = malloc(nlink*sizeof(int));
            new->componentStatus = malloc(ncomp*sizeof(int));
            memcpy(new->linkContent, s->linkContent, nlink*sizeof(int));
            memcpy(new->componentStatus, s->componentStatus, ncomp*sizeof(int));
            new->flags = s->flags;
            new->obsIndex = s->obsIndex;
            new->id = (*map)[i];
            struct ltrans *trans = s->transitions, *temp;
            while (trans != NULL) {     // Transition list copy...
                BehTrans *t = trans->t;
                if (!silence || t->t->obs == 0) {
                    int mapA = (*map)[t->to->id], mapDa = (*map)[t->from->id]; 
                    if (mapA != -1 && mapDa != -1) {
                        struct ltrans *newList = calloc(1, sizeof(struct ltrans));
                        temp = new->transitions;
                        new->transitions = newList;
                        newList->next = temp;
                        if (new->id <= mapA && new->id <= mapDa) { // Alloc nt only once. Dislikes double autotransitions
                            BehTrans *nt = calloc(1, sizeof(BehTrans));
                            dup->nTrans++;
                            newList->t = nt;
                            nt->to = dup->states[mapA];
                            nt->from = dup->states[mapDa];
                            nt->t = t->t;
                            nt->regex = regexCpy(t->regex);
                            nt->marker = t->marker;
                        }
                        else {  // If execution goes here, it means the BehTrans has alredy been created, so we search for its pointer
                            int idSt = mapA < mapDa ? mapA : mapDa;
                            temp = dup->states[idSt]->transitions;
                            while (temp != NULL) {
                                if (temp->t->to->id == mapA && temp->t->from->id == mapDa
                                && temp->t->t == t->t) {
                                    newList->t = temp->t;
                                    break;
                                }
                                temp = temp->next;
                            }
                        }
                    }
                }
                trans = trans->next;
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

/* Not freeing its arcs, just basic structures*/
void freeMonitoringState(MonitorState *RESTRICT mu) {
    free(mu->arcs);
    free(mu->expStates);
    freeRegex(mu->lmu);
    int i=0;
    for (; i<mu->nExpStates; i++) {
        if (mu->lin[i]) freeRegex(mu->lin[i]);
        if (mu->lout[i]) freeRegex(mu->lout[i]);
    }
    free(mu->lin);
    free(mu->lout);
    free(mu);
}

void freeMonitoring(Monitoring *RESTRICT mon) {
    int i, j;
    for (i=0; i<mon->length; i++) {
        MonitorTrans *arc;
        if (mon->mu[i]->nArcs)
            for (arc=mon->mu[i]->arcs[j=0]; j<mon->mu[i]->nArcs; arc=mon->mu[i]->arcs[++j]) {
                freeRegex(arc->l);
                freeRegex(arc->lp);
                free(arc);
            }
        freeMonitoringState(mon->mu[i]);
    }
    free(mon->mu);
    free(mon);
}

void behCoherenceTest(BehSpace *b){
    int i;
    BehState *s;
    printf(MSG_MEMTEST1, b->nStates, b->nTrans);
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (s->id != i) printf(MSG_MEMTEST2, i, s->id);
        foreachdecl(lt, s->transitions) {
            if (lt->t->to->id != i && lt->t->from->id != i)
                printf(MSG_MEMTEST3, i, lt->t->from->id, lt->t->to->id);
            if (lt->t->to != s && lt->t->from != s) {
                printf(MSG_MEMTEST4, i, s);
                printf(MSG_MEMTEST5, lt->t->from, lt->t->from->id, lt->t->to, lt->t->to->id);
                printf(MSG_MEMTEST6, memcmp(lt->t->to, s, sizeof(BehState))==0);
                printf(MSG_MEMTEST7, memcmp(lt->t->from, s, sizeof(BehState))==0);
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
        if (exp->maps != NULL) 
            for (j=0; j<s->b->nStates; j++) {
                if (exp->maps[i]->idMapToOrigin[j] == -1) printf(MSG_MEMTEST11, i, j);
                if (exp->maps[i]->idMapFromOrigin[exp->maps[i]->idMapToOrigin[j]] != j) printf(MSG_MEMTEST12, i, j, j);
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