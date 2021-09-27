#include "header.h"

bool **ok;

Explainer * makeExplainer(BehSpace *b) {
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
        char** diagnosis = diagnostics(duplicated, true);
        freeBehSpace(duplicated);
        j=0;
        while ((currentTr = obsTrs[i][j]) != NULL) {
            ExplTrans *nt = calloc(1, sizeof(ExplTrans));
            nt->from = currentFault;
            nt->fromStateId = currentFault->idMapFromOrigin[currentTr->da->id];
            currentFault->b->states[nt->fromStateId]->flags |= FLAG_SILENT_FINAL;
            nt->toStateId = currentTr->a->id;

            FaultSpace * f;                                                     // Finding reference to destination fault space,
            for (f=exp->faults[k=0]; k<exp->nFaultSpaces; f=exp->faults[++k]) { // means geeting the one that
                if (f->idMapToOrigin[0] == nt->toStateId) {                     // was initialized with the meant destination
                    nt->to = f;
                    break;
                }
            }
            if (nt->to == NULL) printf(MSG_EXP_FAULT_NOT_FOUND);

            nt->obs = currentTr->t->oss;
            nt->ril = currentTr->t->ril;
            nt->regex = diagnosis[currentFault->exitStates[j]];
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
                if (diagnosis[j][0] == '\0') printf("F%d S%d: %lc\n", i, j, eps);
                else printf("F%d S%d: %.10000s\n", i, j, diagnosis[j]);
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
                            free(te);
                            memcpy(precedingMu->arcs+k, precedingMu->arcs+k+1, (precedingMu->nArcs-k)*sizeof(MonitorTrans*));
                            precedingMu->nArcs--;
                            k--;
                        }
                    }
                }
                memcpy(mu->expStates+j, mu->expStates+j+1, (mu->nExpStates-j)*sizeof(FaultSpace*));
                mu->nExpStates--;
                memcpy(ok[i]+j, ok[i]+j+1, (mu->nExpStates-j)*sizeof(bool));
                j--;
            }
        }
    }
    for (i=0; i<mlen; i++) free(ok[i]);
    free(ok);
}

Monitoring* explanationEngine(Explainer * exp, Monitoring *monitor, int * obs, int loss) {
    if (monitor == NULL || loss==0) return initializeMonitoring(exp); // Assert before calling loss==monitor->length
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
                    newmu->expStates[newmu->nExpStates++] = dest;
                }
                MonitorTrans *mt = calloc(1, sizeof(MonitorTrans));
                mt->from = mu_s;
                mt->to = dest;
                alloc1(mu, 'm');
                mu->arcs[mu->nArcs] = mt;
                mu->nArcs++;
            }
        }
    }
    monitor->length++;
    monitor->mu = realloc(monitor->mu, monitor->length*sizeof(MonitorState*));
    monitor->mu[monitor->length-1] = newmu;
    // Extend E by L(µ') based on Def. 7

    // X ← the set of states in µ that are not exited by any arc
    // while X != ∅
    //      Remove from µ the states in X and their entering arcs
    //      Update L(µ) in E based on Def. 7
    //      µ ← the node preceding µ in M
    //      X ← the set of states in µ that are not exited by any arc
    // end while
    pruneMonitoring(monitor);
    if (DEBUG_MODE) monitoringCoherenceTest(monitor);
    // Update L(µ) in E based on Def. 7
    return monitor;
}