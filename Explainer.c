#include "header.h"

Explainer * makeExplainer(BehSpace *b, bool diagnoser) {
    unsigned int i;
    BehTrans ***obsTrs;
    Explainer * exp = calloc(1, sizeof(Explainer));
    exp->faults = faultSpaces(&(exp->maps), b, &(exp->nFaultSpaces), &obsTrs, diagnoser);
    exp->sizeofFaults = exp->nFaultSpaces;
    FaultSpace *currentFault;
    for (currentFault=exp->faults[i=0]; i<exp->nFaultSpaces; currentFault=exp->faults[++i]) {
        unsigned int j=0, k;
        BehTrans* currentTr;
        while ((currentTr = obsTrs[i][j]) != NULL) {
            ExplTrans *nt = calloc(1, sizeof(ExplTrans));
            nt->from = currentFault;
            nt->fromStateId = exp->maps[i]->idMapFromOrigin[currentTr->from->id];
            stateById(currentFault->b, nt->fromStateId)->flags |= FLAG_SILENT_FINAL;
            nt->toStateId = currentTr->to->id;

            FaultSpace * f;                                                     // Finding reference to destination fault space,
            for (f=exp->faults[k=0]; k<exp->nFaultSpaces; f=exp->faults[++k]) { // means geeting the one that
                if (exp->maps[k]->idMapToOrigin[0] == nt->toStateId) {          // was initialized with the meant destination
                    nt->to = f;
                    break;
                }
            }
            if (nt->to == NULL) printf(MSG_EXP_FAULT_NOT_FOUND);

            nt->obs = currentTr->t->obs;
            nt->fault = currentTr->t->fault;
            nt->regex = currentFault->diagnosis[exp->maps[i]->exitStates[j]];
            alloc1(exp, 'e');
            exp->trans[exp->nTrans++] = nt;
            j++;
        }
        free(obsTrs[i]);

        debugif (DEBUG_FAULT_DOT, {
            char filename[100];
            strcpy(filename, inputDES);
            sprintf(inputDES, "debug/%d", i);
            printBehSpace(currentFault->b, true, false, false);
            strcpy(inputDES, filename);
            int nStates = currentFault->b->nStates;
            printlog("Fault space %d having %d states\n", i, nStates);
            for (int j=0; j<nStates; j++) {
                if (currentFault->diagnosis[j]->regex[0] == '\0') printf("F%d S%d: %lc\n", i, j, eps);
                else printf("F%d S%d: %.10000s\n", i, j, currentFault->diagnosis[j]->regex);
            }
        })
    }
    free(obsTrs);
    debugif(DEBUG_MEMCOH, expCoherenceTest(exp))
    return exp;
}

Explainer * makeLazyExplainer(Explainer *exp, BehState* base, bool diagnoser) {
    if (exp == NULL) {
        exp = calloc(1, sizeof(Explainer));
        initCatalogue();
    }
    alloc1(exp, 'f');                                           // Build the fault space rooted in base
    FaultSpace * newFault = makeLazyFaultSpace(exp, base, diagnoser);
    debugif(DEBUG_MON, printlog("Placing fault %p in position %d\n", newFault, exp->nFaultSpaces);)
    exp->faults[exp->nFaultSpaces++] = newFault;
    // Deal with transitions: save them right away with wild pointers, or keep a list of them?
    int hash = hashBehState(catalog.length, base);
    struct tList * pt = catalog.tList[hash], *before=NULL;      // In this bucket are (also) BehTrans going to this fault space
    debugif(DEBUG_MON, printlog("Looking for BehTrans in bucket %d\n", hash);)
    while (pt != NULL) {
        debugif(DEBUG_MON, printlog("Compare %p w %p\n", pt->t->to, base);)
        if (behStateCompareTo(pt->t->to, base, false, false)) {
            ExplTrans *nt = calloc(1, sizeof(ExplTrans));
            nt->to = newFault;
            nt->from = pt->tempFault;
            nt->obs = pt->t->t->obs;
            nt->fault = pt->t->t->fault;
            nt->fromStateId = pt->t->from->id;
            stateById(pt->tempFault->b, nt->fromStateId)->flags |= FLAG_SILENT_FINAL;
            nt->toStateId = 0;
            for (unsigned int k=0; k<nt->from->b->nStates; k++)
                if (behStateCompareTo(pt->t->from, stateById(nt->from->b, k), false, false)) {
                    nt->regex = nt->from->diagnosis[k];
                    break;
                }
            alloc1(exp, 'e');
            exp->trans[exp->nTrans++] = nt;
            // Remove BehTrans from catalog
            if (before == NULL) {
                catalog.tList[hash] = pt->next;
                free(pt);
                pt = catalog.tList[hash];
            } else {
                before->next = pt->next;
                free(pt);
                pt = before->next;
            }
            continue;
        }
        before = pt;
        pt = pt->next;
    }
    pt = catalog.tList[catalog.length];
    while (pt != NULL) {
        ExplTrans *nt = calloc(1, sizeof(ExplTrans));
        nt->to = pt->tempFault;
        nt->from = newFault;
        nt->obs = pt->t->t->obs;
        nt->fault = pt->t->t->fault;
        nt->fromStateId = pt->t->from->id;
        stateById(newFault->b, nt->fromStateId)->flags |= FLAG_SILENT_FINAL;
        nt->toStateId = 0;
        for (unsigned int k=0; k<nt->from->b->nStates; k++)
            if (behStateCompareTo(pt->t->from, stateById(nt->from->b, k), false, false)) {
                nt->regex = nt->from->diagnosis[k];
                break;
            }
        alloc1(exp, 'e');
        exp->trans[exp->nTrans++] = nt;
        catalog.tList[catalog.length] = pt->next;
        free(pt);
        pt = catalog.tList[catalog.length];
    }
    return exp;
}

void buildFaultsReachedWithObs(Explainer * exp, FaultSpace * fault, int obs, bool diagnoser) {
    debugif(DEBUG_MON, printlog("Building on obs %d from fault %p\n", obs, fault);)
    //  for all x ∈ fault, (x, t, x') ∈ X* do
    //      Let fault' denote the fault space of x'
    //      if fault' not in Exp then
    //          Create the state fault' in Exp
    //      end if
    //      Insert the transition (fault,(o, L(x), f), fault') i into Exp->trans
    //  end for
    for (unsigned int i=0; i<catalog.length; i++) {
        struct tList * pt;
        NEXT: pt = catalog.tList[i];
        while (pt != NULL) {
            if (fault==pt->tempFault && pt->t->t->obs == obs) {
                unsigned int bucketId;
                foreachst(fault->b, 
                    if (behStateCompareTo(sl->s, pt->t->from, false, false)) {
                        makeLazyExplainer(exp, pt->t->to, diagnoser); // Not removing here the tList record because it will be removed inside makeLazyExplainer
                        goto NEXT;
                    }
                )
            }
            pt = pt->next;
        }
    }
}