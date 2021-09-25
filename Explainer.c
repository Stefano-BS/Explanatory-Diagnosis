#include "header.h"

Explainer * makeExplainer(BehSpace *b) {
    int i, j, k;
    TransizioneRete ***obsTrs, *currentTr;
    Explainer * exp = calloc(1, sizeof(Explainer));
    exp->faults = faultSpaces(b, &(exp->nFaultSpaces), &obsTrs);
    bool mask[b->nStates];
    memset(mask, true, b->nStates);
    FaultSpace *currentFault;
    for (currentFault=exp->faults[i=0]; i<exp->nFaultSpaces; currentFault=exp->faults[++i]) {
        int nStates = currentFault->b->nStates;
        BehSpace *duplicated = dup(currentFault->b, mask, false, NULL);
        char** diagnosis = diagnostica(duplicated, true);
        freeBehSpace(duplicated);
        j=0;
        while ((currentTr = obsTrs[i][j]) != NULL) {
            TransExpl *nt = calloc(1, sizeof(TransExpl));
            nt->from = currentFault;
            nt->fromStateId = currentFault->idMapFromOrigin[currentTr->da->id];
            nt->toStateId = currentTr->a->id;

            FaultSpace * f;                                                     // Finding reference to destination fault space,
            for (f=exp->faults[k=0]; k<exp->nFaultSpaces; f=exp->faults[++k]) { // means geeting the one
                if (f->idMapToOrigin[0] == nt->toStateId) {                     // that was initialized with the meant destination
                    nt->to = f;
                    break;
                }
            }
            if (nt->to == NULL) printf(MSG_EXP_FAULT_NOT_FOUND);

            nt->obs = currentTr->t->oss;
            nt->ril = currentTr->t->ril;
            nt->regex = diagnosis[currentFault->exitStates[j]];
            if (nt->regex[0] == '\0') {
                nt->regex = calloc(4, 1);
                sprintf(nt->regex, "%lc", eps);
            }
            alloc1trExp(exp);
            exp->trans[exp->nTrans++] = nt;
            j++;
        }
        free(obsTrs[i]);

        if (DEBUG_MODE) {
            char filename[100];
            strcpy(filename, nomeFile);
            sprintf(nomeFile, "debug/%d", i);
            stampaSpazioComportamentale(currentFault->b, false, false);
            strcpy(nomeFile, filename);
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