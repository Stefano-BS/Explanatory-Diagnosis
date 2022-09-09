#include "header.h"

bool **RESTRICT ok;
Regex *temp = NULL;

void calcLout(MonitorState *RESTRICT const mu, short fault, FaultSpace * ptr) {
    if (fault < 0) {
        for (fault=0; fault<mu->nExpStates; fault++)
            if (mu->expStates[fault] == ptr) break;
    }
    assert(ptr == NULL || ptr == mu->expStates[fault]);
    freeRegex(mu->lout[fault]);
    mu->lout[fault] = emptyRegex(0);
    bool firstShot = true;
    int k;
    for (k=0; k<mu->nArcs; k++)
        if (mu->arcs[k]->from == mu->expStates[fault]) {
            if (firstShot) {
                firstShot = false;
                regexMake(mu->lout[fault], mu->arcs[k]->l, mu->lout[fault], 'c', NULL);
            }
            else regexMake(mu->lout[fault], mu->arcs[k]->l, mu->lout[fault], 'a', NULL);
        }
}

void calcLmu(MonitorState *RESTRICT const mu, bool diagnoser) {
    int j;
    if (mu->lmu != NULL) freeRegex(mu->lmu);
    mu->lmu = emptyRegex(0);
    for (j=0; j< mu->nExpStates; j++) {
        if (diagnoser && !mu->expStates[j]->b->containsFinalStates) continue;
        //calcLout(mu, j, NULL);
        regexMake(mu->lin[j], mu->lout[j], temp, 'c', NULL);
        if (j>0) regexMake(mu->lmu, temp, mu->lmu, 'a', NULL);
        else regexMake(mu->lmu, temp, mu->lmu, 'c', NULL);
    }
}

Monitoring * initializeMonitoring(Explainer * exp) {
    if (temp == NULL) temp = emptyRegex(0);
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
    const short mlen = monitor->length;
    ok = calloc(mlen, sizeof(bool*));
    for (i=0; i<mlen; i++)
        ok[i] = calloc(monitor->mu[i]->nExpStates, sizeof(bool));
    memset(ok[mlen-1], true, monitor->mu[mlen-1]->nExpStates);
    
    for (i=mlen-2; i>=0; i--)
        calcWhatToBePruned(monitor, i);
    
    debugif(DEBUG_MON, for (i=0; i<mlen; i++) {for (j=0; j<monitor->mu[i]->nExpStates; j++) printf("%d ", ok[i][j]); printf("\n");})

    MonitorState *RESTRICT mu;
    bool recalcLmu[mlen];
    memset(recalcLmu, 0, mlen*sizeof(bool));
    for (mu=monitor->mu[i=mlen-2]; i>0; mu=monitor->mu[--i]) {
        bool somePruned = false;
        for (j=0; j<mu->nExpStates; j++) {
            if (!ok[i][j]) {
                somePruned = true;
                FaultSpace * remove = mu->expStates[j];
                MonitorTrans *RESTRICT te;
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
                // Except from initial μ, we gotta prune arcs getting to pruned state, so let's look back at previous state
                MonitorState *precedingMu = monitor->mu[i-1];
                for (te=precedingMu->arcs[k=0]; k<precedingMu->nArcs; te=precedingMu->arcs[++k]) {
                    if (te->to == remove) {
                        FaultSpace * from = te->from;
                        freeRegex(te->l);
                        freeRegex(te->lp);
                        free(te);
                        memcpy(precedingMu->arcs+k, precedingMu->arcs+k+1, (precedingMu->nArcs-k)*sizeof(MonitorTrans*));
                        precedingMu->nArcs--;
                        k--;
                        calcLout(precedingMu, -1, from); // That state has 1 less outgoing arc, so a different Lout
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
        if (somePruned) recalcLmu[i-1] = recalcLmu[i] = true; // Prune a state in μi causes recalc of L(μi) as well as of L(μi-1) because Lout in its states might change
    }
    for (i=0; i<mlen; i++) {
        if (recalcLmu[i]) calcLmu(monitor->mu[i], false);
        free(ok[i]);
    }
    free(ok);
}

Monitoring* explanationEngine(Explainer *RESTRICT exp, Monitoring *RESTRICT monitor, int *RESTRICT obs, unsigned short loss, bool lazy, bool diagnoser, bool disablePruning) {
    assert((monitor == NULL && loss == 0) || (monitor->length == loss));
    if (monitor == NULL || loss==0) return initializeMonitoring(exp);
    // Extend O by the new observation o
    unsigned short i, k;
    unsigned int j;
    // Let µ denote the last node of M (before the extension)
    MonitorState *RESTRICT mu = monitor->mu[loss-1];
    //  for all fault ∈ µ do
    //      if there is no transition (fault, o, fault') in Exp then
    //          builfFaultsReachedWithObs
    //      end if
    //  end for
    if (lazy) {
        for (FaultSpace * fault=mu->expStates[i=0]; i<mu->nExpStates; fault=mu->expStates[++i])
            buildFaultsReachedWithObs(exp, fault, obs[loss-1], diagnoser);

        debugif((DEBUG_MON | DEBUG_MEMCOH), expCoherenceTest(exp));
    }
    // Extend M by a node µ' based on the new observation o
    MonitorState *RESTRICT newmu = calloc(1, sizeof(MonitorState));
    for (FaultSpace *RESTRICT mu_s=mu->expStates[i=0]; i<mu->nExpStates; mu_s=mu->expStates[++i]) {
        if (exp->nTrans == 0) break;
        for (ExplTrans *RESTRICT te=exp->trans[j=0]; j<exp->nTrans; te=exp->trans[++j]) {
            if (te->from == mu_s && te->obs == obs[loss-1]) {
                FaultSpace * dest = te->to, *newmu_s;
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
                MonitorTrans *RESTRICT mt = calloc(1, sizeof(MonitorTrans));
                mt->from = mu_s;
                mt->to = dest;
                mt->l = te->regex ? regexCpy(te->regex) : emptyRegex(0); // copy necessary?
                mt->lp = emptyRegex(0);
                Regex * fault = empty;
                if (loss>1) {
                    MonitorTrans *RESTRICT mu_a;
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
                    regexCompile(fault, te->fault);
                    fault->concrete = true;
                }
                regexMake(mt->lp, mt->l, mt->lp, 'c', NULL);
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
    
    for (FaultSpace * state=newmu->expStates[i=0]; i<newmu->nExpStates; state=newmu->expStates[++i]) {
        newmu->lin[i] = emptyRegex(0);
        MonitorTrans *arc;
        bool firstShot = true;
        for (arc=mu->arcs[j=0]; j<mu->nArcs; arc=mu->arcs[++j]) {
            if (state == arc->to) {
                if (firstShot) {
                    firstShot = false;
                    regexMake(newmu->lin[i], arc->lp, newmu->lin[i], 'c', NULL);
                }
                else regexMake(newmu->lin[i], arc->lp, newmu->lin[i], 'a', NULL);
            }
        }
    }
    // Extend E by L(µ') based on Def. 7
    for (i=0; i< newmu->nExpStates; i++)
        newmu->lout[i] = regexCpy(newmu->expStates[i]->alternativeOfDiagnoses);
    calcLmu(newmu, diagnoser);
    // X ← the set of states in µ that are not exited by any arc
    // while X != ∅
    //      Remove from µ the states in X and their entering arcs
    //      Update L(µ) in E based on Def. 7
    //      µ ← the node preceding µ in M
    //      X ← the set of states in µ that are not exited by any arc
    // end while
    if (!disablePruning && !diagnoser) pruneMonitoring(monitor);
    debugif(DEBUG_MEMCOH, monitoringCoherenceTest(monitor))
    // Update L(µ) in E based on Def. 7
    if (!diagnoser)
        for (i=0; i< mu->nExpStates; i++)   // Because even if no pruning was done, the ex last µ
            calcLout(mu, i, NULL);          // still has L(µ) built on Louts made for a last µ
    calcLmu(mu, false);
    return monitor;
}