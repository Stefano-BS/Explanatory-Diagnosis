#include "header.h"

unsigned int *RESTRICT ok;
bool buildingTransCatalogue = false;

FaultSpace * context = NULL;
Explainer * expWorkingOn = NULL;
BehTransCatalog catalog;

O3(BehState * calculateDestination(BehState *RESTRICT base, Trans *RESTRICT t, Component *RESTRICT c, int*RESTRICT obs, unsigned short loss, bool build, char finalStategy)) {
    static short *tempCS, *tempLC;
    if (tempCS == NULL) tempCS = malloc(ncomp*sizeof(short));
    if (tempLC == NULL) tempLC = malloc(nlink*sizeof(short));
    short * newComponentStatus, * newLinkContent;
    if (build) {
        newComponentStatus = malloc(ncomp*sizeof(short));
        newLinkContent = malloc(nlink*sizeof(short));
    }
    else {
        memcpy(tempLC, base->linkContent, nlink*sizeof(short));
        memcpy(tempCS, base->componentStatus, ncomp*sizeof(short));
        newComponentStatus = tempCS;
        newLinkContent = tempLC;
    }
    memcpy(newLinkContent, base->linkContent, nlink*sizeof(short));
    memcpy(newComponentStatus, base->componentStatus, ncomp*sizeof(short));
    if (t->idIncomingEvent != VUOTO)                                                // Incoming event consumption
        newLinkContent[t->linkIn->intId] = VUOTO;
    newComponentStatus[c->intId] = t->to;                                           // New active state
    for (unsigned short k=0; k<t->nOutgoingEvents; k++)                             // Outgoing event placement
        newLinkContent[t->linkOut[k]->intId] = t->idOutgoingEvents[k];
    BehState * newState = generateBehState(newLinkContent, newComponentStatus, finalStategy);
    newState->obsIndex = base->obsIndex;
    if (loss>0 && obs[loss-1]!=-1) {                                                // Linear observation advancement
        if (base->obsIndex<loss && t->obs > 0 && t->obs == obs[base->obsIndex])
            newState->obsIndex++;
        if (finalStategy == 0) newState->flags &= !FLAG_FINAL | (newState->obsIndex == loss);
        else if (finalStategy == 1) newState->flags = newState->obsIndex == loss ? FLAG_FINAL : 0;
    }
    return newState;
}

O3(bool isTransEnabled(BehState *RESTRICT base, Component *RESTRICT c, Trans *RESTRICT t, int *RESTRICT obs, unsigned short loss, char finalStategy)) {
    unsigned int k;
    if ((base->componentStatus[c->intId] == t->from) &&                                             // Trans enabled if its from component is in the correct status
    (t->idIncomingEvent == VUOTO || t->idIncomingEvent == base->linkContent[t->linkIn->intId])) {   // Trans' incoming event okay
        if (!buildingTransCatalogue && loss>0 && ((base->obsIndex>=loss && t->obs > 0) ||           // Having seen all observations, we can't see anymore
        (base->obsIndex<loss && t->obs > 0 && t->obs != obs[base->obsIndex])))                      // Neither can we observe anything not consistent with obs
            return false;
        for (k=0; k<t->nOutgoingEvents; k++)                                                        // Check if outgoing event's link are empty
            if (base->linkContent[t->linkOut[k]->intId] != VUOTO && !(t->idIncomingEvent != VUOTO && t->linkIn->intId == t->linkOut[k]->intId))
                return false;                                                                       // A Tr can consume an event in a link and immediately occupy it again
        if (buildingTransCatalogue && t->obs != 0) {
            BehTrans * nt = calloc(1, sizeof(BehTrans));
            nt->to = calculateDestination(base, t, c, obs, loss, true, finalStategy);
            nt->from = base;
            nt->t = t;

            struct tList *pt = catalog.tList[catalog.length];
            while (pt != NULL) {
                if (behTransCompareTo(nt, pt->t, false, loss)) return false;
                pt = pt->next;
            }

            for (FaultSpace * fault=expWorkingOn->faults[k=0]; k<expWorkingOn->nFaultSpaces; fault=expWorkingOn->faults[++k]) {
                BehState * initialState = stateById(fault->b, 0);
                if (behStateCompareTo(initialState, nt->to, false, loss)) {
                    struct tList * pt = malloc(sizeof(struct tList));
                    pt->tempFault = fault;
                    pt->t = nt;
                    pt->next = catalog.tList[catalog.length];
                    catalog.tList[catalog.length] = pt;
                    return false;                                                                   // If the BehTrans' destination fault alredy exist, keep track in extra bucket
                }
            }
            unsigned int hash = hashBehState(catalog.length, nt->to);                                                        // Otherwise store pending BehTrans
            pt = catalog.tList[hash];
            while (pt != NULL) {
                if (context == pt->tempFault && behTransCompareTo(nt, pt->t, false, loss)) return false;
                pt = pt->next;
            }
            if (pt == NULL) {
                debugif(DEBUG_MON, printlog("Insert BehTrans in bucket %d\n", hash);)
                pt = catalog.tList[hash];
                catalog.tList[hash] = malloc(sizeof(struct tList));
                catalog.tList[hash]->t = nt;
                catalog.tList[hash]->tempFault = context;
                catalog.tList[hash]->next = pt;
            }
            return false;
        }
        else return true;
    }
    return false;
}

O3(bool enlargeBehavioralSpace(BehSpace * b, BehState * from, BehState * new, Trans *RESTRICT mezzo)) {
    BehState * alredyPresent = insertState(b, new, true);
    if (alredyPresent != NULL) {
        new->componentStatus = new->linkContent = NULL;
        freeBehState(new);
        new = alredyPresent;
        struct ltrans * trans = from->transitions;
        while (trans != NULL) {
            if (trans->t->to == new && trans->t->t == mezzo) return false;
            trans = trans->next;
        }
    } else {
        new->id = b->nStates-1;
        short * newComponentStatus = malloc(ncomp*sizeof(short));
        short * newLinkContent = malloc(nlink*sizeof(short));
        memcpy(newComponentStatus, new->componentStatus, ncomp*sizeof(short));
        memcpy(newLinkContent, new->linkContent, nlink*sizeof(short));
        new->componentStatus = newComponentStatus;
        new->linkContent = newLinkContent;
        b->containsFinalStates |= (new->flags & FLAG_FINAL);
    }
    if (mezzo != NULL) { // Initial state has NULL
        BehTrans * nt = calloc(1, sizeof(BehTrans));
        nt->to = new;
        nt->from = from;
        nt->t = mezzo;

        struct ltrans *nlt = malloc(sizeof(struct ltrans));
        nlt->t = nt;
        nlt->next = new->transitions;
        new->transitions = nlt;
        
        if (new != from) {
            nlt = malloc(sizeof(struct ltrans));
            nlt->t = nt;
            nlt->next = from->transitions;
            from->transitions = nlt;
        }

        b->nTrans++;
    }
    return alredyPresent == NULL;
}

O3(void generateBehavioralSpace(BehSpace *b, BehState *base, int*RESTRICT obs, unsigned short loss, char finalStategy)) {
    unsigned short i, j;
    for (Component * c=components[i=0]; i<ncomp;  c=components[++i]) {
        unsigned short nT = c->nTrans;
        for (Trans *t=c->transitions[j=0]; j<nT; t=c->transitions[++j]) {
            if (isTransEnabled(base, c, t, obs, loss, finalStategy)) {
                BehState * newState = calculateDestination(base, t, c, obs, loss, false, finalStategy);
                if (enlargeBehavioralSpace(b, base, newState, t))                       // Let's insert the new BehState & BehTrans in the BehSpace
                    generateBehavioralSpace(b, newState, obs, loss, finalStategy);      // If the BehSpace wasn't enlarged (state alredy present) we cut exploration, otherwise recursive call
            }
        }
    }
}

BehSpace * BehavioralSpace(BehState * src, int * obs, unsigned short loss, char finalStategy) {
    BehSpace * b = newBehSpace();
    if (src == NULL) src = generateBehState(NULL, NULL, 0);
    if (loss > 0) src->flags &= !FLAG_FINAL;
    enlargeBehavioralSpace(b, NULL, src, NULL);
    generateBehavioralSpace(b, src, obs, loss, finalStategy);
    return b;
}

O3(void pruneRec(BehState *RESTRICT s)) { // Invocata esternamente solo a partire dagli stati finali
    ok[s->id] = true;
    foreachdecl(trans, s->transitions) {
        if (trans->t->to == s && !ok[trans->t->from->id])
            pruneRec(trans->t->from);
    }
}

O3(void prune(BehSpace *RESTRICT b)) {
    unsigned int i, bucketId, len=b->nStates, current=0;
    ok = calloc(len, sizeof(unsigned int));
    ok[0] = true;
    foreachst(b,
        if (sl->s->flags & FLAG_FINAL) pruneRec(sl->s);
    )
    
    for (struct sList*sl=b->sMap[i=0]; i<b->hashLen; sl=b->sMap[++i]){
        while (sl) {
            if (!ok[sl->s->id]) {
                removeBehState(b, sl->s, false);
                sl = b->sMap[i];
                continue;
            }
            sl = sl->next;
        }
    }
    unsigned int *okPt = ok;
    for (i=0; i<len; i++) {
        *okPt = *okPt? current++ : 0;
        okPt++;
    }
    foreachst(b, sl->s->id = ok[sl->s->id])
    free(ok);
}

void decorateFaultSpace(FaultSpace * f, bool onlyFinals) {
    BehSpace *duplicated = dup(f->b, NULL, false, NULL, true);
    f->diagnosis = diagnostics(duplicated, 2);
    f->alternativeOfDiagnoses = emptyRegex(REGEX+REGEX*f->b->nTrans/5);
    bool firstShot=true;
    unsigned int bucketId;
    foreachst(f->b,
        if (onlyFinals && !(sl->s->flags & FLAG_FINAL)) {sl = sl->next; continue;}
        if (firstShot) {
            firstShot = false;
            if (f->diagnosis[sl->s->id] != NULL && f->diagnosis[sl->s->id]->strlen>0) {
                regexMake(empty, f->diagnosis[sl->s->id], f->alternativeOfDiagnoses, 'c', NULL);
                sl = sl->next; continue;
            }
        }
        if (f->diagnosis[sl->s->id] != NULL && f->diagnosis[sl->s->id]->regex != NULL)
            regexMake(f->alternativeOfDiagnoses, f->diagnosis[sl->s->id], f->alternativeOfDiagnoses, 'a', NULL);
        else
            regexMake(f->alternativeOfDiagnoses, empty, f->alternativeOfDiagnoses, 'a', NULL);
    )
    freeBehSpace(duplicated);
}

void faultSpaceExtend(BehState *base, int *obsStates, BehTrans **obsTrs, bool *ok) {
    foreachdecl(lt, base->transitions)
        if (lt->t->from == base && lt->t->t->obs != 0) {
            while (*obsStates != -1) {
                obsStates++;
                obsTrs++;
            }
            obsStates[0] = base->id;
            obsTrs[0] = lt->t;
        }
    ok[base->id] = true;
    foreachdecl(lt, base->transitions)
        if (lt->t->from == base && lt->t->t->obs == 0 && !ok[lt->t->to->id])
            faultSpaceExtend(lt->t->to, obsStates, obsTrs, ok);
}

FaultSpace * faultSpace(FaultSpaceMaps *RESTRICT map, BehSpace *RESTRICT b, BehState *RESTRICT base, BehTrans **obsTrs, bool decorateOnlyFinals) {
    unsigned int k, bucketId;
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    map->idMapFromOrigin = calloc(b->nStates, sizeof(int));
    map->exitStates = malloc(b->nTrans*sizeof(int));
    memset(map->exitStates, -1, b->nTrans*sizeof(int));

    bool *ok = calloc(b->nStates, sizeof(bool));
    faultSpaceExtend(base, map->exitStates, obsTrs, ok); // state ids and transitions refer to the original space, not a dup copy
    ret->b = dup(b, ok, true, &map->idMapFromOrigin, false);
    free(ok);

    BehState *r;
    foreachstb(ret->b) // make base its inital state
        r = sl->s;
        if (behStateCompareTo(base, r, false, false)) {
            if (r->id == 0) break;
            k = r->id;                          // update ids
            stateById(ret->b, 0)->id = k;
            r->id = 0;
            int tempId, swap1=0, swap2=0;           // swap id map
            for (unsigned int index=0; index<b->nStates; index++) {
                if (map->idMapFromOrigin[index]==0) swap1=index;
                if (map->idMapFromOrigin[index]==(int)k) swap2=index;
            }
            tempId = map->idMapFromOrigin[swap1];
            map->idMapFromOrigin[swap1] = map->idMapFromOrigin[swap2];
            map->idMapFromOrigin[swap2] = tempId;
            break;
        }
    foreachstc

    for (k=0; k<b->nTrans; k++) {
        if (map->exitStates[k] == -1) {k++; break;}
        map->exitStates[k] = map->idMapFromOrigin[map->exitStates[k]];
    }
    k = k==0 ? 1 : k;
    map->exitStates = realloc(map->exitStates, k*sizeof(int));
    obsTrs = realloc(obsTrs, k*sizeof(BehTrans*));
    obsTrs[k-1] = NULL;
    map->idMapToOrigin = calloc(ret->b->nStates, sizeof(int));
    for (k=0; k<b->nStates; k++)
        if (map->idMapFromOrigin[k] != -1) map->idMapToOrigin[map->idMapFromOrigin[k]] = k;
    
    decorateFaultSpace(ret, decorateOnlyFinals);
    return ret;
}

doC11(int faultJob(void *RESTRICT params) {
    struct FaultSpaceParams*RESTRICT ps = (struct FaultSpaceParams*) params;
    ps->ret[0] = faultSpace(ps->map, ps->b, ps->s, ps->obsTrs, ps->decorateOnlyFinals);
    return 0;
})

/* Call like:
    int nSpaces=0;
    BehTrans *** obsTrs;
    FaultSpace ** ret = faultSpaces(b, &nSpaces, &obsTrs);*/
FaultSpace ** faultSpaces(FaultSpaceMaps ***RESTRICT maps, BehSpace *RESTRICT b, unsigned int *RESTRICT nSpaces, BehTrans ****RESTRICT obsTrs, bool decorateOnlyFinals) {
    unsigned int i, j=0, bucketId;
    *nSpaces = 1;
    foreachstb(b)
        if (sl->s->id > 0) {
            foreachdecl(lt, sl->s->transitions) {
                if (lt->t->to == sl->s && lt->t->t->obs != 0) {
                    (*nSpaces)++;
                    break;
                }
            }
        }
    foreachstc
    FaultSpace ** ret = malloc(*nSpaces*sizeof(BehSpace *));
    *obsTrs = malloc(*nSpaces * sizeof(BehTrans**));
    *maps = malloc(*nSpaces * sizeof(FaultSpaceMaps**));
    for (i=0; i<*nSpaces; i++) {
        (*maps)[i] = malloc(sizeof(FaultSpaceMaps));
        (*obsTrs)[i] = calloc(b->nTrans, sizeof(BehTrans**));
    }
    ret[0] = faultSpace((*maps)[0], b, stateById(b, 0), (*obsTrs)[0], decorateOnlyFinals);
    doC11(
        thrd_t thr[*nSpaces];
        struct FaultSpaceParams params[*nSpaces];
    )
    foreachst(b,
        if (sl->s->id > 0) {
        foreachdecl(lt, sl->s->transitions) {
            if (lt->t->to == sl->s && lt->t->t->obs != 0) {
                j++;
                doC99(ret[j] = faultSpace((*maps)[j], b, sl->s, (*obsTrs)[j], decorateOnlyFinals);)
                doC11(
                    params[j].ret = &ret[j];
                    params[j].b = b;
                    params[j].map = (*maps)[j];
                    params[j].s = sl->s;
                    params[j].obsTrs = (*obsTrs)[j];
                    params[j].decorateOnlyFinals = decorateOnlyFinals;
                    thrd_create(&(thr[j]), faultJob, &(params[j]));
                )
                break;
            }
        }
    })
    doC11(
        for (i=1; i<*nSpaces; i++)
            thrd_join(thr[i], NULL);
    )
    return ret;
}

FaultSpace * makeLazyFaultSpace(Explainer * expCtx, BehState * base, bool diagnoser) {
    int fakeObs[1] = {-1};
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    context = ret;
    expWorkingOn = expCtx;
    buildingTransCatalogue = true;
    ret->b = newBehSpace();
    enlargeBehavioralSpace(ret->b, NULL, base, NULL);
    generateBehavioralSpace(ret->b, base, fakeObs, 1, diagnoser? 0 : 2);  // FakeObs will prevent from expanding in any observable direction
    decorateFaultSpace(ret, diagnoser);
    context = NULL;
    expWorkingOn = NULL;
    buildingTransCatalogue = false;
    return ret;
}

BehSpace * uncompiledMonitoring(BehSpace * b, int * obs, unsigned short loss, bool posteriori) {
    unsigned int bucketId, initialNStates = b->nStates;
    foreachst(b, sl->s->flags = 0;)
    b->containsFinalStates = false;
    BEGINNING: bucketId=0;
    foreachst(b,
        unsigned int hd = b->hashLen;
        if (sl->s->obsIndex == loss-1) {
            generateBehavioralSpace(b, sl->s, obs, loss, !posteriori);
            if (hd != b->hashLen) goto BEGINNING;
        }
    )
    if (initialNStates == b->nStates) prune(b);
    return b;
}