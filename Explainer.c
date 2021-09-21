#include "header.h"

Explainer * makeExplainer(BehSpace *b) {
    int i, *swapIds;
    Explainer * exp = calloc(1, sizeof(Explainer));
    exp->faults = faultSpaces(b, &(exp->nFaultSpaces), &swapIds);
    bool mask[b->nStates];
    memset(mask, true, b->nStates);
    for (i=0; i<exp->nFaultSpaces; i++) {
        int nStates = exp->faults[i]->nStates, j;
        BehSpace *duplicated = dup(exp->faults[i], mask, false);
        char** diagnosis = diagnostica(duplicated, true);

        if (DEBUG_MODE) {
            printlog("Swap %d\n", swapIds[i]);
            sprintf(nomeFile, "debug/%d", i);
            stampaSpazioComportamentale(exp->faults[i], false);
            printlog("Fault space %d having %d states\n", i, nStates);
            for (j=0; j<nStates; j++) {
                if (diagnosis[j][0] == '\0') printf("F%d S%d: %lc\n", i, j, eps);
                else printf("F%d S%d: %.10000s\n", i, j, diagnosis[j]);
            }
        }
    }
    free(swapIds);
    return exp;
}