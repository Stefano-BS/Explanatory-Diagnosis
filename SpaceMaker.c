#include "header.h"

bool *RESTRICT ok;
bool buildingTransCatalogue = false;

FaultSpace * context = NULL;
Explainer * expWorkingOn = NULL;
BehTransCatalog catalog;

BehState * calculateDestination(BehState *base, Trans * t, Component * c, int*RESTRICT obs, unsigned short loss) {
    short * newComponentStatus = malloc(ncomp*sizeof(short));
    short * newLinkContent = malloc(nlink*sizeof(short));
    memcpy(newLinkContent, base->linkContent, nlink*sizeof(short));
    memcpy(newComponentStatus, base->componentStatus, ncomp*sizeof(short));
    if (t->idIncomingEvent != VUOTO)                                                // Incoming event consumption
        newLinkContent[t->linkIn->intId] = VUOTO;
    newComponentStatus[c->intId] = t->to;                                           // New active state
    for (unsigned short k=0; k<t->nOutgoingEvents; k++)                             // Outgoing event placement
        newLinkContent[t->linkOut[k]->intId] = t->idOutgoingEvents[k];
    BehState * newState = generateBehState(newLinkContent, newComponentStatus);     // BehState build
    newState->obsIndex = base->obsIndex;
    if (loss>0 && obs[loss-1]!=-1) {                                                // Linear observation advancement
        if (base->obsIndex<loss && t->obs > 0 && t->obs == obs[base->obsIndex])
            newState->obsIndex++;
        newState->flags &= !FLAG_FINAL | (newState->obsIndex == loss);
    }
    return newState;
}

bool isTransEnabled(BehState *base, Component *c, Trans *t, int *obs, unsigned short loss) {
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
            nt->to = calculateDestination(base, t, c, obs, loss);
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

bool enlargeBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT from, BehState *RESTRICT new, Trans *RESTRICT mezzo) {
    BehState * alredyPresent = catalogInsertState(b, new, true);
    if (alredyPresent != NULL) {
        freeBehState(new);
        new = alredyPresent;
    }
    if (alredyPresent != NULL) { // Già c'era lo stato, ma la transizione non è detto
        struct ltrans * trans = from->transitions;
        while (trans != NULL) {
            if (trans->t->to == new && trans->t->t->id == mezzo->id) return false;
            trans = trans->next;
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        new->id = b->nStates-1;
    }
    if (mezzo != NULL) { // Lo stato iniziale, ad esempio ha NULL
        BehTrans * nuovaTransRete = calloc(1, sizeof(BehTrans));
        nuovaTransRete->to = new;
        nuovaTransRete->from = from;
        nuovaTransRete->t = mezzo;
        nuovaTransRete->regex = NULL;

        struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
        nuovaLTr->t = nuovaTransRete;
        nuovaLTr->next = new->transitions;
        new->transitions = nuovaLTr;
        
        if (new != from) {
            nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->next = from->transitions;
            from->transitions = nuovaLTr;
        }

        b->nTrans++;
    }
    b->containsFinalStates |= (new->flags & FLAG_FINAL);
    return alredyPresent == NULL;
}

void generateBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT base, int*RESTRICT obs, unsigned short loss) {
    Component *c;
    unsigned short i, j;
    for (c=components[i=0]; i<ncomp;  c=components[++i]){
        Trans *t;
        unsigned short nT = c->nTrans;
        if (nT == 0) continue;
        for (t=c->transitions[j=0]; j<nT; t=c->transitions[++j]) {
            if (isTransEnabled(base, c, t, obs, loss)) {                                // Checks okay, execution
                BehState * newState = calculateDestination(base, t, c, obs, loss);
                if (enlargeBehavioralSpace(b, base, newState, t))                       // Let's insert the new BehState & BehTrans in the BehSpace
                    generateBehavioralSpace(b, newState, obs, loss);                    // If the BehSpace wasn't enlarged (state alredy present) we cut exploration, otherwise recursive call
            }
        }
    }
}

BehSpace * BehavioralSpace(BehState * src, int * obs, unsigned short loss) {
    BehSpace * b = newBehSpace();
    if (src == NULL) src = generateBehState(NULL, NULL);
    if (loss > 0) src->flags &= !FLAG_FINAL;
    enlargeBehavioralSpace(b, NULL, src, NULL);
    generateBehavioralSpace(b, src, obs, loss);
    return b;
}

void pruneRec(BehState *RESTRICT s) { // Invocata esternamente solo a partire dagli stati finali
    ok[s->id] = true;
    foreachdecl(trans, s->transitions) {
        if (trans->t->to == s && !ok[trans->t->from->id])
            pruneRec(trans->t->from);
    }
}

void prune(BehSpace *RESTRICT b) {
    unsigned int i, bucketId;
    ok = calloc(b->nStates, sizeof(bool));
    ok[0] = true;
    foreachst(b,
        if (sl->s->flags & FLAG_FINAL) pruneRec(sl->s);
    )
    
    for (struct sList*sl=b->sMap[i=0]; i<b->hashLen; sl=b->sMap[++i]){
        while (sl) {
            if (!ok[sl->s->id]) {
                memcpy(ok+sl->s->id, ok+sl->s->id+1, (b->nStates-sl->s->id-1)*sizeof(bool));
                removeBehState(b, sl->s);
                sl = b->sMap[i];
                continue;
            }
            sl = sl->next;
        }
    }
    free(ok);
}

void decorateFaultSpace(FaultSpace * f, bool onlyFinals) {
    BehSpace *duplicated = dup(f->b, NULL, false, NULL);
    f->diagnosis = diagnostics(duplicated, 2);
    f->alternativeOfDiagnoses = emptyRegex(REGEX+REGEX*f->b->nTrans/5);
    bool firstShot=true;
    unsigned int bucketId;
    foreachst(f->b, 
        if (onlyFinals && !(sl->s->flags & FLAG_FINAL)) continue;
        if (firstShot) {
            firstShot = false;
            if (f->diagnosis[sl->s->id] != NULL && f->diagnosis[sl->s->id]->strlen>0) {
                regexMake(empty, f->diagnosis[sl->s->id], f->alternativeOfDiagnoses, 'c', NULL);
                continue;
            }
        }
        if (f->diagnosis[sl->s->id] != NULL && f->diagnosis[sl->s->id]->regex != NULL)
            regexMake(f->alternativeOfDiagnoses, f->diagnosis[sl->s->id], f->alternativeOfDiagnoses, 'a', NULL);
        else
            regexMake(f->alternativeOfDiagnoses, empty, f->alternativeOfDiagnoses, 'a', NULL);
    )
    freeBehSpace(duplicated);
}

void faultSpaceExtend(BehState *RESTRICT base, int *obsStates, BehTrans **obsTrs, bool *ok) {
    ok[base->id] = true;
    unsigned int i=0;
    foreachdecl(lt, base->transitions)
        if (lt->t->from == base && lt->t->t->obs != 0) {
            while (obsStates[i] != -1) i++;
            obsStates[i] = base->id;
            obsTrs[i] = lt->t;
        }
    foreachdecl(lt, base->transitions)
        if (lt->t->from == base && lt->t->t->obs == 0 && !ok[lt->t->to->id])
            faultSpaceExtend(lt->t->to, obsStates+i, obsTrs+i, ok);
}

FaultSpace * faultSpace(FaultSpaceMaps *RESTRICT map, BehSpace *RESTRICT b, BehState *RESTRICT base, BehTrans **obsTrs, bool decorateOnlyFinals) {
    unsigned int k, bucketId;
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    map->idMapFromOrigin = calloc(b->nStates, sizeof(int));
    map->exitStates = malloc(b->nTrans*sizeof(int));
    memset(map->exitStates, -1, b->nStates*sizeof(int));

    bool *ok = calloc(b->nStates, sizeof(bool));  // MAY I USE idMapFromOrigin instead?
    faultSpaceExtend(base, map->exitStates, obsTrs, ok); // state ids and transitions refer to the original space, not a dup copy
    ret->b = dup(b, ok, true, &map->idMapFromOrigin);
    free(ok);

    BehState *r;
    foreachstb(ret->b) // make base its inital state
        r = sl->s;
        if (//k!=0 //&& base->obsIndex == r->obsIndex
        memcmp(base->linkContent, r->linkContent, nlink*sizeof(short)) == 0
        && memcmp(base->componentStatus, r->componentStatus, ncomp*sizeof(short)) == 0) {
            if (r->id == 0) break;
            k = r->id;                          // update ids
            stateById(ret->b, 0)->id = k;
            r->id = 0;
            int tempId, swap1, swap2;           // swap id map
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
    generateBehavioralSpace(ret->b, base, fakeObs, 1);  // FakeObs will prevent from expanding in any observable direction
    decorateFaultSpace(ret, diagnoser);
    context = NULL;
    expWorkingOn = NULL;
    buildingTransCatalogue = false;
    return ret;
}

BehSpace * uncompiledMonitoring(BehSpace * b, int * obs, unsigned short loss) {
    foreachst(b, sl->s->flags &= !FLAG_FINAL;)
    foreachst(b, generateBehavioralSpace(b, sl->s, obs, loss);)
    if (loss) prune(b);
    else {
        foreachst(b,
            sl->s->flags = FLAG_FINAL;
            for (unsigned short j=0; j<nlink; j++)
                sl->s->flags &= (sl->s->linkContent[j] == VUOTO);
        )
    }
    return b;
}