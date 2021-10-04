#include "header.h"

bool *RESTRICT ok;
bool buildingTransCatalogue = false;

FaultSpace * context = NULL;
Explainer * expWorkingOn = NULL;
BehSpaceCatalog catalog;

BehState * calculateDestination(BehState *base, Trans * t, Component * c, int*RESTRICT obs, int loss) {
    int newComponentStatus[ncomp], newLinkContent[nlink];
    memcpy(newLinkContent, base->linkContent, nlink*sizeof(int));
    memcpy(newComponentStatus, base->componentStatus, ncomp*sizeof(int));
    if (t->idIncomingEvent != VUOTO)                                                // Incoming event consumption
        newLinkContent[t->linkIn->intId] = VUOTO;
    newComponentStatus[c->intId] = t->to;                                           // New active state
    for (int k=0; k<t->nOutgoingEvents; k++)                                        // Outgoing event placement
        newLinkContent[t->linkOut[k]->intId] = t->idOutgoingEvents[k];
    BehState * newState = generateBehState(newLinkContent, newComponentStatus);     // BehState build
    newState->obsIndex = base->obsIndex;
    if (loss>0) {                                                                   // Linear observation advancement
        if (base->obsIndex<loss && t->obs > 0 && t->obs == obs[base->obsIndex])
            newState->obsIndex = base->obsIndex+1;
        newState->flags &= !FLAG_FINAL | (newState->obsIndex == loss);
    }
    return newState;
}

bool isTransEnabled(BehState *base, Component *c, Trans *t, int *obs, int loss) {
    int k;
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

            int hash = hashBehState(nt->to);                                                        // Otherwise store pending BehTrans
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

bool enlargeBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT precedente, BehState *RESTRICT nuovo, Trans *RESTRICT mezzo, int loss) {
    int i;
    bool giaPresente = false;
    BehState *s;
    if (b->nStates>0) {
        for (s=b->states[i=0]; i<b->nStates; s=(i<b->nStates ? b->states[++i] : s)) { // Per ogni stato dello spazio comportamentale
            if (memcmp(nuovo->componentStatus, s->componentStatus, ncomp*sizeof(int)) == 0
            && memcmp(nuovo->linkContent, s->linkContent, nlink*sizeof(int)) == 0) {
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

void generateBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT base, int*RESTRICT obs, int loss) {
    Component *c;
    int i, j;
    for (c=components[i=0]; i<ncomp;  c=components[++i]){
        Trans *t;
        int nT = c->nTrans;
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

void pruneRec(BehState *RESTRICT s) { // Invocata esternamente solo a partire dagli stati finali
    ok[s->id] = true;
    foreachdecl(trans, s->transitions) {
        if (trans->t->to == s && !ok[trans->t->from->id])
            pruneRec(trans->t->from);
    }
}

void prune(BehSpace *RESTRICT b) {
    BehState *s;
    int i;
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
    int k;
    bool mask[f->b->nStates];
    memset(mask, true, f->b->nStates);
    BehSpace *duplicated = dup(f->b, mask, false, NULL);
    f->diagnosis = diagnostics(duplicated, true);
    f->alternativeOfDiagnoses = emptyRegex(REGEX+REGEX*f->b->nTrans/5);
    for (k=0; k<f->b->nStates; k++)
        if (f->diagnosis[k] != NULL && f->diagnosis[k]->regex != NULL)
            regexMake(f->alternativeOfDiagnoses, f->diagnosis[k], f->alternativeOfDiagnoses, 'a', NULL);
        else
            regexMake(f->alternativeOfDiagnoses, empty, f->alternativeOfDiagnoses, 'a', NULL);
    freeBehSpace(duplicated);
}

void faultSpaceExtend(BehState *RESTRICT base, int *obsStates, BehTrans **obsTrs) {
    ok[base->id] = true;
    int i=0;
    foreachdecl(lt, base->transitions) {
        if (lt->t->from == base && lt->t->t->obs != 0) {
            while (obsStates[i] != -1) i++;
            obsStates[i] = base->id;
            obsTrs[i] = lt->t;
        }
    }
    foreach(lt, base->transitions)
        if (lt->t->from == base && lt->t->t->obs == 0 && !ok[lt->t->to->id])
            faultSpaceExtend(lt->t->to, obsStates+i, obsTrs+i);
}

FaultSpace * faultSpace(BehSpace *RESTRICT b, BehState *RESTRICT base, BehTrans **obsTrs) {
    int k, index=0;
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    ret->idMapFromOrigin = calloc(b->nStates, sizeof(int));
    ret->exitStates = malloc(b->nTrans*sizeof(int));
    memset(ret->exitStates, -1, b->nStates*sizeof(int));

    ok = calloc(b->nStates, sizeof(bool));
    faultSpaceExtend(base, ret->exitStates, obsTrs); // state ids and transitions refer to the original space, not a dup copy
    ret->b = dup(b, ok, true, &ret->idMapFromOrigin);
    free(ok);

    BehState *r, *temp;
    for (r=ret->b->states[k=0]; k<ret->b->nStates; r=ret->b->states[++k]) { // make base its inital state
        if (//k!=0 //&& base->obsIndex == r->obsIndex
        memcmp(base->linkContent, r->linkContent, nlink*sizeof(int)) == 0
        && memcmp(base->componentStatus, r->componentStatus, ncomp*sizeof(int)) == 0) {
            temp = ret->b->states[0];           // swap ptrs
            ret->b->states[0] = r;
            ret->b->states[k] = temp;
            temp->id = k;                       // update ids
            r->id = 0;
            int tempId, swap1, swap2;           // swap id map
            for (index=0; index<b->nStates; index++) {
                if (ret->idMapFromOrigin[index]==0) swap1=index;
                if (ret->idMapFromOrigin[index]==k) swap2=index;
            }
            tempId = ret->idMapFromOrigin[swap1];
            ret->idMapFromOrigin[swap1] = ret->idMapFromOrigin[swap2];
            ret->idMapFromOrigin[swap2] = tempId;
            break;
        }
    }
    for (k=0; k<b->nTrans; k++) {
        if (ret->exitStates[k] == -1) {k++; break;}
        ret->exitStates[k] = ret->idMapFromOrigin[ret->exitStates[k]];
    }
    k = k==0 ? 1 : k;
    ret->exitStates = realloc(ret->exitStates, k*sizeof(int));
    obsTrs = realloc(obsTrs, k*sizeof(BehTrans*));
    obsTrs[k-1] = NULL;
    ret->idMapToOrigin = calloc(ret->b->nStates, sizeof(int));
    for (k=0; k<b->nStates; k++)
        if (ret->idMapFromOrigin[k] != -1) ret->idMapToOrigin[ret->idMapFromOrigin[k]] = k;
    
    decorateFaultSpace(ret);
    return ret;
}

/* Call like:
    int nSpaces=0;
    BehTrans *** obsTrs;
    FaultSpace ** ret = faultSpaces(b, &nSpaces, &obsTrs);*/
FaultSpace ** faultSpaces(BehSpace *RESTRICT b, int *RESTRICT nSpaces, BehTrans ****RESTRICT obsTrs) {
    BehState * s;
    int i, j=0;
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
    *obsTrs = (BehTrans***)calloc(*nSpaces, sizeof(BehTrans**));
    for (i=0; i<*nSpaces; i++) {
        (*obsTrs)[i] = calloc(b->nTrans, sizeof(BehTrans**));
    }
    ret[0] = faultSpace(b, b->states[0], (*obsTrs)[0]);
    for (s=b->states[i=1]; i<b->nStates; s=b->states[++i]) {
        foreachdecl(lt, s->transitions) {
            if (lt->t->to == s && lt->t->t->obs != 0) {
                j++;
                ret[j] = faultSpace(b, s, (*obsTrs)[j]);
                break;
            }
        }
    }
    return ret;
}

FaultSpace * makeLazyFaultSpace(Explainer * expCtx, BehState * base) {
    int fakeObs[1] = {-1}; //, i, nExitStates=0;
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    context = ret;
    expWorkingOn = expCtx;
    buildingTransCatalogue = true;
    ret->b = newBehSpace();
    enlargeBehavioralSpace(ret->b, NULL, base, NULL, 0);
    generateBehavioralSpace(ret->b, base, fakeObs, 1);  // FakeObs will prevent from expanding in any observable direction
    //ret->exitStates = malloc(ret->b->nStates* sizeof(int));
    /*BehState *s;
    for (s=ret->b->states[i=0]; i<ret->b->nStates; s=ret->b->states[++i]) {
        int hash = hashBehState(s);
        struct sList *pt = catalog.sList[hash];
        while (pt != NULL) {
            if (behStateCompareTo(pt->s, s)) break;
            pt = pt->next;
        }
        if (pt == NULL) {
            pt = catalog.sList[hash];
            catalog.sList[hash] = malloc(sizeof(struct sList));
            catalog.sList[hash]->s = s;
            catalog.sList[hash]->next = pt;
        }
        
        //for (t=components[i=0]->transitions[j=0]; i<ncomp; t=(j+1==components[i]->nTrans? components[++i]->transitions[j=0] : components[i]->transitions[++j])) {     // Double var auto cycle <3
        //for (Component * c=components[k=0]; k<ncomp; c=components[++k])
            for (Trans *t=c->transitions[j=0]; j<c->nTrans; t=c->transitions[++j])
                if (t->obs != 0 && isTransEnabled(s, components[k], t, NULL, 0))
                    ret->exitStates[nExitStates++] = i;
    }*/
    //ret->exitStates = realloc(ret->exitStates, nExitStates*sizeof(int));
    decorateFaultSpace(ret);
    context = NULL;
    expWorkingOn = NULL;
    buildingTransCatalogue = false;
    return ret;
}