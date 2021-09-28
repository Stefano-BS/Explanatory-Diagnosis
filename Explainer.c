#include "header.h"

bool **ok;
Regex * empty = NULL;

Explainer * makeExplainer(BehSpace *b) {
    if (empty == NULL) empty = emptyRegex(0);
    int i, j, k;
    BehTrans ***obsTrs, *currentTr;
    Explainer * exp = calloc(1, sizeof(Explainer));
    exp->faults = faultSpaces(b, &(exp->nFaultSpaces), &obsTrs);
    bool mask[b->nStates];
    memset(mask, true, b->nStates);
    FaultSpace *currentFault;
    for (currentFault=exp->faults[i=0]; i<exp->nFaultSpaces; currentFault=exp->faults[++i]) {
        int nStates = currentFault->b->nStates;
        BehSpace *duplicated = dup(currentFault->b, mask, false, NULL);
        currentFault->diagnosis = diagnostics(duplicated, true);
        currentFault->alternativeOfDiagnoses = emptyRegex(REGEX+REGEX*currentFault->b->nStates/2);
        for (j=0; j<currentFault->b->nStates; j++)
            if (currentFault->diagnosis[j] != NULL && currentFault->diagnosis[j]->regex != NULL)
                regexMake(currentFault->alternativeOfDiagnoses, currentFault->diagnosis[j], currentFault->alternativeOfDiagnoses, 'a', NULL);
            else
                regexMake(currentFault->alternativeOfDiagnoses, empty, currentFault->alternativeOfDiagnoses, 'a', NULL);
        freeBehSpace(duplicated);
        j=0;
        while ((currentTr = obsTrs[i][j]) != NULL) {
            ExplTrans *nt = calloc(1, sizeof(ExplTrans));
            nt->from = currentFault;
            nt->fromStateId = currentFault->idMapFromOrigin[currentTr->from->id];
            currentFault->b->states[nt->fromStateId]->flags |= FLAG_SILENT_FINAL;
            nt->toStateId = currentTr->to->id;

            FaultSpace * f;                                                     // Finding reference to destination fault space,
            for (f=exp->faults[k=0]; k<exp->nFaultSpaces; f=exp->faults[++k]) { // means geeting the one that
                if (f->idMapToOrigin[0] == nt->toStateId) {                     // was initialized with the meant destination
                    nt->to = f;
                    break;
                }
            }
            if (nt->to == NULL) printf(MSG_EXP_FAULT_NOT_FOUND);

            nt->obs = currentTr->t->obs;
            nt->fault = currentTr->t->fault;
            nt->regex = currentFault->diagnosis[currentFault->exitStates[j]];
            alloc1(exp, 'e');
            exp->trans[exp->nTrans++] = nt;
            j++;
        }
        free(obsTrs[i]);

        if (DEBUG_MODE) {
            char filename[100];
            strcpy(filename, inputDES);
            sprintf(inputDES, "debug/%d", i);
            printBehSpace(currentFault->b, false, false, false);
            strcpy(inputDES, filename);
            printlog("Fault space %d having %d states\n", i, nStates);
            for (j=0; j<nStates; j++) {
                if (currentFault->diagnosis[j]->regex[0] == '\0') printf("F%d S%d: %lc\n", i, j, eps);
                else printf("F%d S%d: %.10000s\n", i, j, currentFault->diagnosis[j]->regex);
            }
        }
    }
    free(obsTrs);
    if (DEBUG_MODE) expCoherenceTest(exp);
    return exp;
}


Monitoring * initializeMonitoring(Explainer * exp) {
    Monitoring *m = calloc(1, sizeof(Monitoring));
    m->mu = malloc(sizeof(MonitorState*));
    MonitorState *mu0 = calloc(1, sizeof(MonitorState));
    mu0->expStates = malloc(sizeof(FaultSpace*));
    mu0->expStates[0] = exp->faults[0];
    mu0->nExpStates = 1;
    mu0->lin = malloc(sizeof(Regex*));
    mu0->lin[0] = emptyRegex(0);
    mu0->lout = malloc(sizeof(Regex*));
    mu0->lout[0] = emptyRegex(0);
    mu0->lmu = regexCpy(exp->faults[0]->alternativeOfDiagnoses);
    alloc1(mu0, 'm');
    m->mu[0] = mu0;
    m->length=1;
    return m;
}

void calcWhatToBePruned(Monitoring *monitor, int level) {
    int i, j, k;
    MonitorState *mu = monitor->mu[level];
    FaultSpace *faultFrom, *faultTo;
    MonitorTrans *te;
    for (faultFrom=mu->expStates[i=0]; i<mu->nExpStates; faultFrom=mu->expStates[++i]) {
        for (te=mu->arcs[j=0]; j<mu->nArcs; te=mu->arcs[++j]) {
            if (te->from == faultFrom) {
                for (faultTo=monitor->mu[level+1]->expStates[k=0]; k<monitor->mu[level+1]->nExpStates; faultTo=monitor->mu[level+1]->expStates[++k]) {
                    if (te->to == faultTo && ok[level+1][k]) {ok[level][i]=true; break;}
                }
                if (ok[level][i]) break;
            }
        }
    }
}

void pruneMonitoring(Monitoring * monitor) {
    int i, j, k;
    const int mlen = monitor->length;
    ok = calloc(mlen, sizeof(bool*));
    for (i=0; i<mlen; i++)
        ok[i] = calloc(monitor->mu[i]->nExpStates, sizeof(bool));
    memset(ok[mlen-1], true, monitor->mu[mlen-1]->nExpStates);
    
    for (i=mlen-2; i>=0; i--)
        calcWhatToBePruned(monitor, i);
    
    if (DEBUG_MODE) for (i=0; i<mlen; i++) {for (j=0; j<monitor->mu[i]->nExpStates; j++) printf("%d ", ok[i][j]); printf("\n");}

    MonitorState *mu;
    for (mu=monitor->mu[i=0]; i<mlen-1; mu=monitor->mu[++i]) {
        for (j=0; j<mu->nExpStates; j++) {
            if (!ok[i][j]) {
                FaultSpace * remove = mu->expStates[j];
                MonitorTrans *te;
                for (te=mu->arcs[k=0]; k<mu->nArcs; te=mu->arcs[++k]) {
                    if (te->from == remove) {
                        freeRegex(te->l);
                        freeRegex(te->lp);
                        free(te);
                        memcpy(mu->arcs+k, mu->arcs+k+1, (mu->nArcs-k)*sizeof(MonitorTrans*));
                        mu->nArcs--;
                        k--;
                    }
                }
                if (i>0) {  // Except from initial μ, we gotta prune arcs getting to pruned state, so let's look back at previous state
                    MonitorState *precedingMu = monitor->mu[i-1];
                    for (te=precedingMu->arcs[k=0]; k<precedingMu->nArcs; te=precedingMu->arcs[++k]) {
                        if (te->to == remove) {
                            freeRegex(te->l);
                            freeRegex(te->lp);
                            free(te);
                            memcpy(precedingMu->arcs+k, precedingMu->arcs+k+1, (precedingMu->nArcs-k)*sizeof(MonitorTrans*));
                            precedingMu->nArcs--;
                            k--;
                        }
                    }
                }
                memcpy(mu->expStates+j, mu->expStates+j+1, (mu->nExpStates-j)*sizeof(FaultSpace*));
                freeRegex(mu->lin[j]);
                freeRegex(mu->lout[j]);
                memcpy(mu->lout+j, mu->lout+j+1, (mu->nExpStates-j)*sizeof(Regex*));
                memcpy(mu->lin+j, mu->lin+j+1, (mu->nExpStates-j)*sizeof(Regex*));
                mu->nExpStates--;
                memcpy(ok[i]+j, ok[i]+j+1, (mu->nExpStates-j)*sizeof(bool));
                j--;
            }
        }
    }
    for (i=0; i<mlen; i++) free(ok[i]);
    free(ok);
}

Monitoring* explanationEngine(Explainer *RESTRICT exp, Monitoring *RESTRICT monitor, int *RESTRICT obs, int loss) {
    if (monitor == NULL || loss==0)  // Assert before calling loss==monitor->length
        return initializeMonitoring(exp);
    // Extend O by the new observation o
    int i, j, k;
    // Let µ denote the last node of M (before the extension)
    MonitorState *mu = monitor->mu[loss-1];
    // Extend M by a node µ' based on the new observation o
    MonitorState *newmu = calloc(1, sizeof(MonitorState));
    FaultSpace * mu_s;
    for (mu_s=mu->expStates[i=0]; i<mu->nExpStates; mu_s=mu->expStates[++i]) {
        ExplTrans *te;
        for (te=exp->trans[j=0]; j<exp->nTrans; te=exp->trans[++j]) {
            if (te->from == mu_s && te->obs == obs[loss-1]) {
                FaultSpace *dest = te->to, *newmu_s;
                bool destAlredyPresent = false;
                if (newmu->nExpStates)
                    for (newmu_s=newmu->expStates[k=0]; k<newmu->nExpStates; newmu_s=newmu->expStates[++k]) 
                        destAlredyPresent |= (newmu_s == dest);
                if (!destAlredyPresent) {
                    newmu->expStates = realloc(newmu->expStates, (newmu->nExpStates+1)*sizeof(FaultSpace*));
                    newmu->lin = realloc(newmu->lin, (newmu->nExpStates+1)*sizeof(Regex*));
                    newmu->lout = realloc(newmu->lout, (newmu->nExpStates+1)*sizeof(Regex*));
                    newmu->expStates[newmu->nExpStates++] = dest;
                }
                MonitorTrans *mt = calloc(1, sizeof(MonitorTrans));
                mt->from = mu_s;
                mt->to = dest;
                mt->l = regexCpy(te->regex); // copy necessary?
                mt->lp = emptyRegex(0);
                Regex * fault = empty, *l = empty;
                if (loss>1) {
                    MonitorTrans *mu_a;
                    bool firstShot = true;
                    for (mu_a=monitor->mu[loss-2]->arcs[k=0]; k<monitor->mu[loss-2]->nArcs; mu_a=monitor->mu[loss-2]->arcs[++k]) {
                        if (mu_a->to == mu_s) {
                            if (firstShot) {
                                firstShot = false;
                                regexMake(mt->lp, mu_a->lp, mt->lp, 'c', NULL);
                            }
                            else regexMake(mt->lp, mu_a->lp, mt->lp, 'a', NULL);
                        }
                    }
                }
                if (te->fault) {
                    fault = emptyRegex(0);
                    sprintf(fault->regex, "r%d", te->fault);
                    fault->concrete = true;
                }
                if (te->regex != NULL && te->regex->regex != NULL) l = te->regex;
                regexMake(mt->lp, l, mt->lp, 'c', NULL);
                regexMake(mt->lp, fault, mt->lp, 'c', NULL);
                alloc1(mu, 'm');
                mu->arcs[mu->nArcs] = mt;
                mu->nArcs++;
            }
        }
    }
    if (mu->nArcs + newmu->nExpStates) {
        newmu->lmu = emptyRegex(0);
        monitor->length++;
        monitor->mu = realloc(monitor->mu, monitor->length*sizeof(MonitorState*));
        monitor->mu[monitor->length-1] = newmu;
    }
    else return NULL;
    FaultSpace *state;
    for (state=newmu->expStates[i=0]; i<newmu->nExpStates; state=newmu->expStates[++i]) {
        newmu->lin[i] = emptyRegex(0);
        MonitorTrans *arc;
        bool firstShot = true;
        for (arc=mu->arcs[j=0]; j<mu->nArcs; arc=mu->arcs[++j]) {
            if (state == arc->to) {
                if (firstShot) {
                    firstShot = false;
                    regexMake(newmu->lin[i], arc->l, newmu->lin[i], 'c', NULL);
                }
                else regexMake(newmu->lin[i], arc->l, newmu->lin[i], 'a', NULL);
            }
        }
    }
    // Extend E by L(µ') based on Def. 7
    for (i=0; i< newmu->nExpStates; i++)
        newmu->lout[i] = regexCpy(newmu->expStates[i]->alternativeOfDiagnoses);
    Regex * temp = emptyRegex(0);
    for (i=0; i< newmu->nExpStates; i++) {
        regexMake(newmu->lin[i], newmu->lout[i], temp, 'c', NULL);
        regexMake(newmu->lmu, temp, newmu->lmu, 'a', NULL);
    }
    // X ← the set of states in µ that are not exited by any arc
    // while X != ∅
    //      Remove from µ the states in X and their entering arcs
    //      Update L(µ) in E based on Def. 7 <-- no
    //      µ ← the node preceding µ in M
    //      X ← the set of states in µ that are not exited by any arc
    // end while
    pruneMonitoring(monitor);
    if (DEBUG_MODE) monitoringCoherenceTest(monitor);
    // Update L(µ) in E based on Def. 7
    for (mu=monitor->mu[i=0]; i<monitor->length-1; mu=monitor->mu[++i]) {
        for (j=0; j<mu->nExpStates; j++) {
            freeRegex(mu->lout[j]);
            mu->lout[j] = emptyRegex(0);
            bool firstShot = true;
            for (k=0; k<mu->nArcs; k++)
                if (mu->arcs[k]->from == mu->expStates[j]) {
                    if (firstShot) {
                        firstShot = false;
                        regexMake(mu->lout[j], mu->arcs[k]->lp, mu->lout[j], 'c', NULL);
                    }
                    else regexMake(mu->lout[j], mu->arcs[k]->lp, mu->lout[j], 'a', NULL);
                }
        }
        freeRegex(mu->lmu);
        mu->lmu = emptyRegex(0);
        for (j=0; j< mu->nExpStates; j++) {
            regexMake(mu->lin[j], mu->lout[j], temp, 'c', NULL);
            regexMake(mu->lmu, temp, mu->lmu, 'a', NULL);
        }
    }
    freeRegex(temp);
    return monitor;
}