#include "header.h"

unsigned short nlink=0, ncomp=0;
Component **RESTRICT components;
Link **RESTRICT links;

// Buffered memory allocation (reduces realloc frequency)
unsigned short *sizeofTrComp;
unsigned short sizeofCOMP=5, sizeofLINK=5;                                 // Initial sizes

void netAlloc(unsigned short sComp, unsigned short sLink){
    if (sComp) sizeofCOMP = sComp;
    if (sLink) sizeofLINK = sLink;
    components = malloc(sizeofCOMP*sizeof(Component*));
    sizeofTrComp = malloc(sizeofCOMP*sizeof(short));              // Array of pointers to transitions inside components' sizes
    links = malloc(sizeofLINK* sizeof(Link*));
    if (components==NULL || sizeofTrComp==NULL || links==NULL) printf(MSG_MEMERR);
}

unsigned int BehSpaceSizeEsteem(void) {
    unsigned int i, j, behSizeEsteem=0;
    for (Component *c=components[i=0]; i<ncomp; c=components[++i]) {
        behSizeEsteem += c->nStates;
        behSizeEsteem += c->nTrans;
        for (Trans * t=c->transitions[j=0]; j<c->nTrans; t=c->transitions[++j]) {
            if (t->nOutgoingEvents==0) behSizeEsteem++;
            if (t->idIncomingEvent==VUOTO) behSizeEsteem++;
            if (t->nOutgoingEvents<2) behSizeEsteem++;
            if (t->obs==0) behSizeEsteem++;
        }
    }
    return hashlen(behSizeEsteem);
}

BehSpace * newBehSpace(void) {
    BehSpace* b = calloc(1, sizeof(BehSpace));
    b->hashLen = HASH_NO;
    b->sMap = calloc(b->hashLen, sizeof(struct sList*));
    if (b->sMap==NULL) printf(MSG_MEMERR);
    return b;
}

/*  c: Component
    e: ExplTrans in an Explainer
    f: FaultSpace in an Explainer
    l: Link
    m: MonitorTrans in a MonitorState
    t: Trans in a Component
*/
void alloc1(void *ptr, char o) {   // Call before inserting anything new in any of these structures
    switch (o) {
        case 'c': {
            if (ncomp+1 > sizeofCOMP) {
                sizeofCOMP += 5;
                Component ** mem = realloc(components, sizeofCOMP*sizeof(Component*));
                unsigned short* mem2 = realloc(sizeofTrComp, sizeofCOMP*sizeof(unsigned short));
                if ((mem == NULL) | (mem2 == NULL)) printf(MSG_MEMERR);
                else {components = mem; sizeofTrComp = mem2;}
            }
        } break;
        case 'l': {
            if (nlink+1 > sizeofLINK) {
                sizeofLINK += 5;
                Link ** mem = realloc(links, sizeofLINK*sizeof(Link*));
                if (mem == NULL) printf(MSG_MEMERR);
                else links = mem;
            }
        } break;
        case 't': {
            Component * comp = (Component*) ptr;
            if (comp->nTrans+1 > sizeofTrComp[comp->intId]) {
                sizeofTrComp[comp->intId] += 10;
                Trans ** mem = realloc(comp->transitions, sizeofTrComp[comp->intId]*sizeof(Trans*));
                if (mem == NULL) printf(MSG_MEMERR);
                else comp->transitions = mem;
            }
        } break;
        case 'e': {
            Explainer * exp = (Explainer *) ptr;
            if (exp->nTrans+1 > exp->sizeofTrans) {
                exp->sizeofTrans += 10;
                ExplTrans ** mem = realloc(exp->trans, exp->sizeofTrans*sizeof(ExplTrans*));
                if (mem == NULL) printf(MSG_MEMERR);
                else exp->trans = mem;
            }
        } break;
        case 'f': {
            Explainer * exp = (Explainer *) ptr;
            if (exp->nFaultSpaces+1 > exp->sizeofFaults) {
                exp->sizeofFaults += 5;
                FaultSpace ** mem = realloc(exp->faults, exp->sizeofFaults*sizeof(FaultSpace*));
                if (mem == NULL) printf(MSG_MEMERR);
                else exp->faults = mem;
            }
        } break;
        case 'm': {
            MonitorState * mu = (MonitorState *) ptr;
            if (mu->nArcs+1 > mu->sizeofArcs) {
                mu->sizeofArcs += 10;
                MonitorTrans ** mem = realloc(mu->arcs, mu->sizeofArcs*sizeof(MonitorTrans*));
                if (mem == NULL) printf(MSG_MEMERR);
                else mu->arcs = mem;
            }
        } break;
    }
}

Component * newComponent(void) {
    alloc1(NULL, 'c');
    Component *ret = calloc(1, sizeof(Component));
    ret->transitions = calloc(5, sizeof(Trans*));
    ret->intId = ncomp;
    sizeofTrComp[ncomp] = 5;
    components[ncomp++] = ret;
    return ret;
}

Component* compById(short id) {
    for (short i=0; i<ncomp; i++)
        if (components[i]->id == id) return components[i];
    printf(MSG_COMP_NOT_FOUND, id);
    return NULL;
}

Link* linkById(short id) {
    for (short i=0; i<nlink; i++)
        if (links[i]->id == id) return links[i];
    printf(MSG_LINK_NOT_FOUND, id);
    return NULL;
}

INLINE(unsigned int hashBehState(unsigned int hashLen, BehState *s)) {
    if (hashLen<2) return 0;
    unsigned int hash = s->obsIndex+1;
    short * cs = s->componentStatus;
    for (unsigned short i=0; i<ncomp; i++) hash = ((hash << 3) + cs[i]);
    //for (unsigned short i=0; i<nlink; i++) hash = ((hash << 4) + s->linkContent[i]+1) % hashLen;
    return hash % hashLen;
}

INLINE(bool behTransCompareTo(BehTrans * t1, BehTrans *t2, bool flags, bool obsIndex)) {
    return  t1->marker == t2->marker && t1->t == t2->t
            && behStateCompareTo(t1->from, t2->from, flags, obsIndex) && behStateCompareTo(t1->to, t2->to, flags, obsIndex);
}

INLINE(bool behStateCompareTo(BehState * s1, BehState * s2, bool flags, bool obsIndex)) {
    if (flags && s1->flags != s2->flags) return false;
    if (obsIndex && s1->obsIndex != s2->obsIndex) return false;
    return  memcmp(s1->componentStatus, s2->componentStatus, ncomp*sizeof(short)) == 0
            && memcmp(s1->linkContent, s2->linkContent, nlink*sizeof(short)) == 0;
}

void initCatalogue(void) {
    catalog.length = BehSpaceSizeEsteem();
    printlog("Hash: %u\n", catalog.length);
    catalog.tList = calloc(catalog.length+1, sizeof(struct tList*));
}

void resizeBehSpace(BehSpace * b) {
    unsigned int ns = b->nStates,
    newSize = hashlen(ns);
    if (newSize <= b->hashLen) return;
    debugif(DEBUG_MEMCOH, printlog("Resize BehSpace %c (%p) from %d to %d on %d states\n", 'A'+(char)((unsigned long long)b%25), b, b->hashLen, newSize, ns));
    BehSpace* new = calloc(1, sizeof(BehSpace));
    new->sMap = calloc(newSize, sizeof(struct sList*));
    new->hashLen = newSize;
    if (new->sMap==NULL) printf(MSG_MEMERR);
    unsigned int bucketId;
    foreachst(b, insertState(new, sl->s, false));
    struct sList * pt;
    for(struct sList*sl=b->sMap[bucketId=0];bucketId<b->hashLen;sl=b->sMap[++bucketId]) {
        while (sl) {
            pt=sl->next;
            free(sl);
            sl = pt;
        }
    }
    free(b->sMap);
    b->sMap = new->sMap;
    b->hashLen = newSize;
    debugif(DEBUG_MEMCOH, behCoherenceTest(b))
    free(new);
}

// If the state was alredy present, returns a pointer to it
BehState * insertState(BehSpace * b, BehState * s, bool controls) {
    unsigned int hash = s->componentStatus ? hashBehState(b->hashLen, s) : 0;
    struct sList * pt = b->sMap[hash];
    if (controls) {
        while (pt != NULL) {
            if (behStateCompareTo(pt->s, s, false, true)) return pt->s;
            pt = pt->next;
        }
        unsigned int ns = b->nStates;
        if (b->hashLen < (hashlen(ns))) {
            resizeBehSpace(b);
            hash = s->componentStatus ? hashBehState(b->hashLen, s) : 0;
        }
        pt = b->sMap[hash];
    }
    b->sMap[hash] = malloc(sizeof(struct sList));
    b->sMap[hash]->next = pt;
    b->sMap[hash]->s = s;
    b->nStates++;
    return NULL;
}

BehState * stateById(BehSpace * b, int id) {
    unsigned int bucketId;
    foreachst(b, if (sl->s->id == id) return sl->s;)
    return NULL;
}

BehState * generateBehState(short *RESTRICT linkContent, short *RESTRICT componentStatus) {
    BehState *s = calloc(1, sizeof(BehState));
    if (linkContent != NULL) s->linkContent = linkContent; //memcpy(s->linkContent, linkContent, nlink*sizeof(short));
    else {
        s->linkContent = malloc(nlink*sizeof(short));
        memset(s->linkContent, VUOTO, nlink*sizeof(short));
    }
    if (componentStatus != NULL) s->componentStatus = componentStatus; //memcpy(s->componentStatus, componentStatus, ncomp*sizeof(short));
    else s->componentStatus = calloc(ncomp, sizeof(short));
    s->flags = FLAG_FINAL;
    for (unsigned short i=0; i<nlink; i++)
        s->flags &= (s->linkContent[i] == VUOTO); // Works because there are no other flags set
    return s;
}

void removeBehState(BehSpace *RESTRICT b, BehState *RESTRICT delete, bool idJob) {
    int deleteId = delete->id;
    unsigned int bucketId, hash = delete->componentStatus ? hashBehState(b->hashLen, delete) : 0;
    struct ltrans * temp, *trans, *transPrima, *temp2;
    temp = delete->transitions;
    trans = transPrima = temp2 = NULL;
    
    foreachtr(temp, temp) {
        transPrima = temp;
        foreachtr(trans, temp->next) {
            if (trans->t == temp->t) {
                transPrima->next = trans->next;
                free(trans);
                trans = transPrima;
            }
            transPrima = trans;
        }
    }

    temp = delete->transitions;
    while (temp != NULL) {
        BehTrans * tr = temp->t;
        BehState  *eliminaAncheDa = NULL;

        if (tr->from != delete && tr->to != delete)
            printf(MSG_STATE_ANOMALY, deleteId); // Weird
        else {
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
        }
        temp = temp->next;
        free(delete->transitions);
        delete->transitions = temp;
    }

    struct sList * pt = b->sMap[hash], *tmp;
    if (pt->s == delete) {
        b->sMap[hash] = pt->next;
        free(pt);
    }
    else {
        while (pt->next->s != delete) pt = pt->next;
        tmp = pt->next->next;
        free(pt->next);
        pt->next = tmp;
    }
    freeBehState(delete);
    b->nStates--;
    if (idJob)
        foreachst(b, 
            if (sl->s->id>=deleteId) sl->s->id--;
        )
}

void freeBehState(BehState *s) {
    if (s->linkContent != NULL) free(s->linkContent);
    if (s->componentStatus != NULL) free(s->componentStatus);
    if (s->transitions != NULL) free(s->transitions); // Struttura ricorsiva il cui contenuto deve essere già stato liberato
    free(s);
}

void freeCatalogue(void) {
    void * tmp;
    if (catalog.tList) {
        for (unsigned int i = 0; i<catalog.length+1; i++) {
            struct tList * pt = catalog.tList[i];
            while (pt) {
                tmp = pt->next;
                free(pt);
                pt = tmp;
            }
        }
        free(catalog.tList);
    }
}

/* Call like:
    bool mask[b->nStates];
    memset(mask, true, b->nStates);
    BehSpace * duplicated = dup(b, mask, false, NULL); */
BehSpace * dup(BehSpace *RESTRICT b, bool mask[], bool silence, int**RESTRICT map) {
    debugif(DEBUG_MEMCOH, behCoherenceTest(b));
    unsigned int i;
    int ns = 0;

    if (map == NULL) {
        int *temp = malloc(b->nStates*sizeof(int));
        map = &temp;
    }
    if (mask) {
        for (i=0; i<b->nStates; i++) {
            (*map)[i] = mask[i] ? ns : -1;     // Map: id->id from old to new space
            ns = mask[i] ? ns+1 : ns;
        }
    } else {
        ns = b->nStates;
        for (i=0; i<b->nStates; i++) 
            (*map)[i] = i;
    }
    
    BehSpace *dup = calloc(1, sizeof(BehSpace));
    dup->hashLen = hashlen(ns);
    dup->sMap = calloc(dup->hashLen, sizeof(struct sList*));

    BehState * new, *s;
    unsigned int bucketId;
    foreachstb(b)
        s = sl->s;
        if (mask == NULL || mask[s->id]) {
            new = calloc(1, sizeof(BehState));
            dup->containsFinalStates |= (s->flags & FLAG_FINAL);    // Calculate containing final states
            new->linkContent = malloc(nlink*sizeof(short));
            new->componentStatus = malloc(ncomp*sizeof(short));
            memcpy(new->linkContent, s->linkContent, nlink*sizeof(short));
            memcpy(new->componentStatus, s->componentStatus, ncomp*sizeof(short));
            new->flags = s->flags;
            new->obsIndex = s->obsIndex;
            new->id = (*map)[s->id];
            insertState(dup, new, false);
            struct ltrans *trans = s->transitions;
            struct ltrans *temp;
            while (trans != NULL) {     // Transition list copy...
                BehTrans *t = trans->t;
                if (!silence || t->t->obs == 0) {
                    int mapA = (*map)[t->to->id];
                    int mapDa = (*map)[t->from->id]; 
                    if (mapA != -1 && mapDa != -1) {
                        BehState * tDest = stateById(dup, mapA);
                        BehState * tSrc = stateById(dup, mapDa);
                        if (tDest && tSrc) {
                            BehTrans *nt = calloc(1, sizeof(BehTrans));
                            dup->nTrans++;
                            nt->to = tDest;
                            nt->from = tSrc;
                            nt->t = t->t;
                            nt->regex = regexCpy(t->regex);
                            nt->marker = t->marker;
                            struct ltrans *newList = calloc(1, sizeof(struct ltrans));
                            newList->t = nt;
                            temp = tSrc->transitions;
                            tSrc->transitions = newList;
                            newList->next = temp;
                            if (tSrc != tDest) {
                                struct ltrans *newList = calloc(1, sizeof(struct ltrans));
                                newList->t = nt;
                                temp = tDest->transitions;
                                tDest->transitions = newList;
                                newList->next = temp;
                            }
                        }
                    }
                }
                trans = trans->next;
            }
        }
    foreachstc
    debugif(DEBUG_MEMCOH, behCoherenceTest(dup));
    return dup;
}

void freeBehSpace(BehSpace *b) {
    unsigned int bucketId;
    foreachst(b, 
        removeBehState(b, sl->s, false);
        sl = b->sMap[bucketId];
        continue;
    );
    free(b->sMap);
    free(b);
}

/* Not freeing its arcs, just basic structures*/
void freeMonitoringState(MonitorState *RESTRICT mu) {
    free(mu->arcs);
    free(mu->expStates);
    freeRegex(mu->lmu);
    for (unsigned short i=0; i<mu->nExpStates; i++) {
        if (mu->lin[i]) freeRegex(mu->lin[i]);
        if (mu->lout[i]) freeRegex(mu->lout[i]);
    }
    free(mu->lin);
    free(mu->lout);
    free(mu);
}

void freeMonitoring(Monitoring *RESTRICT mon) {
    unsigned short j;
    for (unsigned short i=0; i<mon->length; i++) {
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
    BehState *s;
    printf(MSG_MEMTEST1, b->nStates, b->nTrans, b->hashLen);
    unsigned int bucketId;
    foreachst(b, 
        s = sl->s;
        foreachdecl(lt, s->transitions) {
            if (lt->t->to->id != (int)s->id && lt->t->from->id != (int)s->id)
                printf(MSG_MEMTEST3, s->id, lt->t->from->id, lt->t->to->id);
            if (lt->t->to != s && lt->t->from != s) {
                printf(MSG_MEMTEST4, s->id, s);
                printf(MSG_MEMTEST5, lt->t->from, lt->t->from->id, lt->t->to, lt->t->to->id);
                printf(MSG_MEMTEST6, memcmp(lt->t->to, s, sizeof(BehState))==0);
                printf(MSG_MEMTEST7, memcmp(lt->t->from, s, sizeof(BehState))==0);
            }
        }
    )
}

void expCoherenceTest(Explainer *exp){
    unsigned int i, j;
    FaultSpace * s;
    printf(MSG_MEMTEST9, exp->nFaultSpaces, exp->nTrans);
    for (s=exp->faults[i=0]; i<exp->nFaultSpaces; s=exp->faults[++i]) {
        behCoherenceTest(s->b);
        if (exp->maps != NULL) 
            for (j=0; j<s->b->nStates; j++) {
                if (exp->maps[i]->idMapToOrigin[j] == -1) printf(MSG_MEMTEST11, i, j);
                if (exp->maps[i]->idMapFromOrigin[exp->maps[i]->idMapToOrigin[j]] != (int)j) printf(MSG_MEMTEST12, i, j, j);
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

void monitoringCoherenceTest(Monitoring *mon) {
    unsigned short i, j, k;
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