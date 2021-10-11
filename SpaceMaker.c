#include "header.h"

bool *RESTRICT ok;
bool buildingTransCatalogue = false;

FaultSpace * context = NULL;
Explainer * expWorkingOn = NULL;
BehSpaceCatalog catalog;

BehState * calculateDestination(BehState *base, Trans * t, Component * c, int*RESTRICT obs, unsigned short loss) {
    short newComponentStatus[ncomp], newLinkContent[nlink];
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
                if (behTransCompareTo(nt, pt->t)) return false;
                pt = pt->next;
            }

            for (FaultSpace * fault=expWorkingOn->faults[k=0]; k<expWorkingOn->nFaultSpaces; fault=expWorkingOn->faults[++k])
                if (behStateCompareTo(fault->b->states[0], nt->to)) {
                    struct tList * pt = malloc(sizeof(struct tList));
                    pt->tempFault = fault;
                    pt->t = nt;
                    pt->next = catalog.tList[catalog.length];
                    catalog.tList[catalog.length] = pt;
                    return false;                                                                   // If the BehTrans' destination fault alredy exist, keep track in extra bucket
                }

            unsigned int hash = hashBehState(nt->to);                                                        // Otherwise store pending BehTrans
            pt = catalog.tList[hash];
            while (pt != NULL) {
                if (context == pt->tempFault && behTransCompareTo(nt, pt->t)) return false;
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

bool enlargeBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT precedente, BehState *RESTRICT nuovo, Trans *RESTRICT mezzo, unsigned short loss) {
    unsigned int i;
    bool giaPresente = false;
    BehState *s;
    if (b->nStates>0) {
        for (s=b->states[i=0]; i<b->nStates; s=(i<b->nStates ? b->states[++i] : s)) {
            if (memcmp(nuovo->componentStatus, s->componentStatus, ncomp*sizeof(short)) == 0
            && memcmp(nuovo->linkContent, s->linkContent, nlink*sizeof(short)) == 0) {
                giaPresente = (loss>0 ? nuovo->obsIndex == s->obsIndex : true);
                if (giaPresente) {
                    freeBehState(nuovo); // Questa istruzione previene la duplicazione in memoria di stati con stessa semantica
                    nuovo = s;  // Non è solo una questione di prestazioni, ma di mantenimento relazione biunivoca ptr <-> id
                    break;
                }
            }
        }
    }
    if (giaPresente) { // Già c'era lo stato, ma la transizione non è detto
        struct ltrans * trans = precedente->transitions;
        nuovo->id = s->id;
        while (trans != NULL) {
            if (trans->t->to == nuovo && trans->t->t->id == mezzo->id) return false;
            trans = trans->next;
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        alloc1(b, 's');
        nuovo->id = b->nStates;
        b->states[b->nStates++] = nuovo;
    }
    if (mezzo != NULL) { // Lo stato iniziale, ad esempio ha NULL
        BehTrans * nuovaTransRete = calloc(1, sizeof(BehTrans));
        nuovaTransRete->to = nuovo;
        nuovaTransRete->from = precedente;
        nuovaTransRete->t = mezzo;
        nuovaTransRete->regex = NULL;

        struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
        nuovaLTr->t = nuovaTransRete;
        nuovaLTr->next = nuovo->transitions;
        nuovo->transitions = nuovaLTr;
        
        if (nuovo != precedente) {
            nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->next = precedente->transitions;
            precedente->transitions = nuovaLTr;
        }

        b->nTrans++;
    }
    return !giaPresente;
}

void generateBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT base, int*RESTRICT obs, unsigned short loss) {
    Component *c;
    unsigned short i, j;
    for (c=components[i=0]; i<ncomp;  c=components[++i]){
        Trans *t;
        unsigned short nT = c->nTrans;
        if (nT == 0) continue;
        for (t=c->transitions[j=0]; j<nT; t=c->transitions[++j]) {
            if (isTransEnabled(base, c, t, obs, loss)) {                                    // Checks okay, execution
                BehState * newState = calculateDestination(base, t, c, obs, loss);
                bool avanzamento = enlargeBehavioralSpace(b, base, newState, t, loss);      // Let's insert the new BehState & BehTrans in the BehSpace
                if (avanzamento) generateBehavioralSpace(b, newState, obs, loss);           // If the BehSpace wasn't enlarged (state alredy present) we cut exploration, otherwise recursive call
            }
        }
    }
}

BehSpace * BehavioralSpace(BehState * src, int * obs, unsigned short loss) {
    BehSpace * b = newBehSpace();
    if (src == NULL) src = generateBehState(NULL, NULL);
    if (loss > 0) src->flags &= !FLAG_FINAL;
    enlargeBehavioralSpace(b, NULL, src, NULL, loss);
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
    BehState *s;
    unsigned int i;
    ok = calloc(b->nStates, sizeof(bool));
    ok[0] = true;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (s->flags & FLAG_FINAL) pruneRec(s);
    }
    
    for (i=0; i<b->nStates; i++) {
        if (!ok[i]) {
            removeBehState(b, b->states[i]);
            memcpy(ok+i, ok+i+1, (b->nStates-i)*sizeof(bool));
            i--;
        }
    }
    free(ok);
}

void decorateFaultSpace(FaultSpace * f) {
    bool mask[f->b->nStates];
    memset(mask, true, f->b->nStates);
    BehSpace *duplicated = dup(f->b, mask, false, NULL);
    f->diagnosis = diagnostics(duplicated, true);
    f->alternativeOfDiagnoses = emptyRegex(REGEX+REGEX*f->b->nTrans/5);
    bool firstShot=true;
    for (unsigned int k=0; k<f->b->nStates; k++) {
        if (firstShot) {
            firstShot = false;
            if (f->diagnosis[k] != NULL && f->diagnosis[k]->strlen>0) {
                regexMake(empty, f->diagnosis[k], f->alternativeOfDiagnoses, 'c', NULL);
                continue;
            }
        }
        if (f->diagnosis[k] != NULL && f->diagnosis[k]->regex != NULL)
            regexMake(f->alternativeOfDiagnoses, f->diagnosis[k], f->alternativeOfDiagnoses, 'a', NULL);
        else
            regexMake(f->alternativeOfDiagnoses, empty, f->alternativeOfDiagnoses, 'a', NULL);
    }
    freeBehSpace(duplicated);
}

void faultSpaceExtend(BehState *RESTRICT base, int *obsStates, BehTrans **obsTrs, bool *ok) {
    ok[base->id] = true;
    unsigned int i=0;
    foreachdecl(lt, base->transitions) {
        if (lt->t->from == base && lt->t->t->obs != 0) {
            while (obsStates[i] != -1) i++;
            obsStates[i] = base->id;
            obsTrs[i] = lt->t;
        }
    }
    foreachtr(lt, base->transitions)
        if (lt->t->from == base && lt->t->t->obs == 0 && !ok[lt->t->to->id])
            faultSpaceExtend(lt->t->to, obsStates+i, obsTrs+i, ok);
}

FaultSpace * faultSpace(FaultSpaceMaps *RESTRICT map, BehSpace *RESTRICT b, BehState *RESTRICT base, BehTrans **obsTrs) {
    unsigned int k, index=0;
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    map->idMapFromOrigin = calloc(b->nStates, sizeof(int));
    map->exitStates = malloc(b->nTrans*sizeof(int));
    memset(map->exitStates, -1, b->nStates*sizeof(int));

    bool *ok = calloc(b->nStates, sizeof(bool));
    faultSpaceExtend(base, map->exitStates, obsTrs, ok); // state ids and transitions refer to the original space, not a dup copy
    ret->b = dup(b, ok, true, &map->idMapFromOrigin);
    free(ok);

    BehState *r, *temp;
    for (r=ret->b->states[k=0]; k<ret->b->nStates; r=ret->b->states[++k]) { // make base its inital state
        if (//k!=0 //&& base->obsIndex == r->obsIndex
        memcmp(base->linkContent, r->linkContent, nlink*sizeof(short)) == 0
        && memcmp(base->componentStatus, r->componentStatus, ncomp*sizeof(short)) == 0) {
            temp = ret->b->states[0];           // swap ptrs
            ret->b->states[0] = r;
            ret->b->states[k] = temp;
            temp->id = k;                       // update ids
            r->id = 0;
            int tempId, swap1, swap2;           // swap id map
            for (index=0; index<b->nStates; index++) {
                if (map->idMapFromOrigin[index]==0) swap1=index;
                if (map->idMapFromOrigin[index]==(int)k) swap2=index;
            }
            tempId = map->idMapFromOrigin[swap1];
            map->idMapFromOrigin[swap1] = map->idMapFromOrigin[swap2];
            map->idMapFromOrigin[swap2] = tempId;
            break;
        }
    }
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
    
    decorateFaultSpace(ret);
    return ret;
}

doC11(int faultJob(void *RESTRICT params) {
    struct FaultSpaceParams*RESTRICT ps = (struct FaultSpaceParams*) params;
    ps->ret[0] = faultSpace(ps->map, ps->b, ps->s, ps->obsTrs);
    return 0;
})

/* Call like:
    int nSpaces=0;
    BehTrans *** obsTrs;
    FaultSpace ** ret = faultSpaces(b, &nSpaces, &obsTrs);*/
FaultSpace ** faultSpaces(FaultSpaceMaps ***RESTRICT maps, BehSpace *RESTRICT b, unsigned int *RESTRICT nSpaces, BehTrans ****RESTRICT obsTrs) {
    BehState * s;
    unsigned int i, j=0;
    *nSpaces = 1;
    for (s=b->states[i=1]; i<b->nStates; s=b->states[++i]) {
        foreachdecl(lt, s->transitions) {
            if (lt->t->to == s && lt->t->t->obs != 0) {
                (*nSpaces)++;
                break;
            }
        }
    }
    FaultSpace ** ret = malloc(*nSpaces*sizeof(BehSpace *));
    *obsTrs = malloc(*nSpaces * sizeof(BehTrans**));
    *maps = malloc(*nSpaces * sizeof(FaultSpaceMaps**));
    for (i=0; i<*nSpaces; i++) {
        (*maps)[i] = malloc(sizeof(FaultSpaceMaps));
        (*obsTrs)[i] = calloc(b->nTrans, sizeof(BehTrans**));
    }
    ret[0] = faultSpace((*maps)[0], b, b->states[0], (*obsTrs)[0]);
    doC11(
        thrd_t thr[*nSpaces];
        struct FaultSpaceParams params[*nSpaces];
    )
    for (s=b->states[i=1]; i<b->nStates; s=b->states[++i]) {
        foreachdecl(lt, s->transitions) {
            if (lt->t->to == s && lt->t->t->obs != 0) {
                j++;
                doC99(ret[j] = faultSpace((*maps)[j], b, s, (*obsTrs)[j]);)
                doC11(
                    params[j].ret = &ret[j];
                    params[j].b = b;
                    params[j].map = (*maps)[j];
                    params[j].s = s;
                    params[j].obsTrs = (*obsTrs)[j];
                    thrd_create(&(thr[j]), faultJob, &(params[j]));
                )
                break;
            }
        }
    }
    doC11(
        for (i=1; i<*nSpaces; i++)
            thrd_join(thr[i], NULL);
    )
    return ret;
}

FaultSpace * makeLazyFaultSpace(Explainer * expCtx, BehState * base) {
    int fakeObs[1] = {-1};
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    context = ret;
    expWorkingOn = expCtx;
    buildingTransCatalogue = true;
    ret->b = newBehSpace();
    enlargeBehavioralSpace(ret->b, NULL, base, NULL, 0);
    generateBehavioralSpace(ret->b, base, fakeObs, 1);  // FakeObs will prevent from expanding in any observable direction
    decorateFaultSpace(ret);
    context = NULL;
    expWorkingOn = NULL;
    buildingTransCatalogue = false;
    return ret;
}