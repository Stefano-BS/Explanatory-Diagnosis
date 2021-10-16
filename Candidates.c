#include "header.h"

INLINE(bool exitCondition(BehSpace *RESTRICT b, char mode)) {
    if (mode) {
        if (b->nStates>2) return false;
        foreachdecl(lt, b->states[0]->transitions) {
            debugif(DEBUG_DIAG, printlog("from %d to %d mark %d\n",lt->t->from->id, lt->t->to->id, lt->t->marker))
            foreachdecl(lt2, lt->next) {
                if (lt->t->marker != 0 && lt->t->marker == lt2->t->marker) return false;
            }
        }
        return true;
    }
    else return b->nTrans<=1;
}

Regex** diagnostics(BehSpace *RESTRICT b, char mode) {
    unsigned int i=0, j=0, nMarker = b->nStates+2;
    int markerMap[nMarker], k;
    if (mode==2){
        for (; i<nMarker; i++) 
            markerMap[i] = i; // Including α and ω
    }
    else if (mode==1) {
        for (; i<nMarker; i++) 
            markerMap[i] = (b->states[i]->flags & FLAG_FINAL)*i;
    }
    Regex ** ret = calloc(mode? nMarker-2 : 1, sizeof(Regex*));
    BehState * stemp = NULL;
    //Arricchimento spazio con nuovi stati iniziale e finale
    Trans *tvuota = calloc(1, sizeof(Trans));
    for (stemp=b->states[k=((int)b->nStates)-1]; k>=0; stemp=b->states[--k])          // New initial state α
        stemp->id++;
    alloc1(b, 's');
    memmove(b->states+1, b->states, b->nStates*sizeof(BehState*));
    b->nStates++;
    stemp = calloc(1, sizeof(BehState));
    b->states[0] = stemp;
    stemp->id = 0;
    BehTrans * nuovaTr = calloc(1, sizeof(BehTrans));
    nuovaTr->from = stemp;
    nuovaTr->to = b->states[1];
    nuovaTr->t = tvuota;
    nuovaTr->regex = emptyRegex(0);
    stemp->transitions = calloc(1, sizeof(struct ltrans));
    stemp->transitions->t = nuovaTr;
    struct ltrans *vecchiaTesta = b->states[1]->transitions;
    b->states[1]->transitions = calloc(1, sizeof(struct ltrans));
    b->states[1]->transitions->t = nuovaTr;
    b->states[1]->transitions->next = vecchiaTesta;
    b->nTrans++;

    alloc1(b, 's');
    BehState * fine = calloc(1, sizeof(BehState));                              // New final state ω
    fine->id = b->nStates;
    fine->flags = FLAG_FINAL;
    for (stemp = b->states[i=1]; i<b->nStates; stemp=b->states[++i]) {          // Transitions from ex final states/all states to ω
        if (mode==2 || (stemp->flags & FLAG_FINAL)) {
            BehTrans * trFinale = calloc(1, sizeof(BehTrans));
            trFinale->from = stemp;
            trFinale->to = fine;
            trFinale->t = tvuota;
            trFinale->regex = emptyRegex(0);
            struct ltrans *vecchiaTesta = stemp->transitions;
            stemp->transitions = calloc(1, sizeof(struct ltrans));
            stemp->transitions->t = trFinale;
            stemp->transitions->next = vecchiaTesta;
            vecchiaTesta = fine->transitions;
            fine->transitions = calloc(1, sizeof(struct ltrans));
            fine->transitions->t = trFinale;
            fine->transitions->next = vecchiaTesta;
            b->nTrans++;
            stemp->flags &= !FLAG_FINAL;
        }
    }
    b->states[b->nStates++] = fine;

    for (stemp=b->states[j=0]; j<b->nStates; stemp=b->states[++j]) {           // Singleton regex making
        foreachdecl(trans, stemp->transitions) {
            if (trans->t->from == stemp) {
                trans->t->regex = emptyRegex(0);
                if (trans->t->t->fault >0) {
                    regexCompile(trans->t->regex, trans->t->t->fault);
                    trans->t->regex->concrete = true;
                }
            }
        }
    }
    
    debugif(DEBUG_DIAG, printlog("-----\n"));
    while (!exitCondition(b, mode)) {
        bool continua = false;
        for (stemp = b->states[i=0]; i<b->nStates; stemp=b->states[++i]) {     // Semplificazione serie -> unità
            BehTrans *tentra, *tesce;
            if (stemp->transitions != NULL && stemp->transitions->next != NULL && stemp->transitions->next->next == NULL &&
            (((tesce = stemp->transitions->t)->from == stemp && (tentra = stemp->transitions->next->t)->to == stemp)
            || ((tentra = stemp->transitions->t)->to == stemp && (tesce = stemp->transitions->next->t)->from == stemp))
            && tesce->from != tesce->to && tentra->to != tentra->from)  {             // Elemento interno alla sequenza
                BehTrans *nt = calloc(1, sizeof(BehTrans));
                nt->from = tentra->from;
                nt->to = tesce->to;
                nt->t = tvuota;
                nt->regex = tentra->regex;  // To save CPU I'm making the new regex right into tentra's
                regexMake(tentra->regex, tesce->regex, tentra->regex, 'c', NULL);
                if (mode && nt->to->id == ((int)b->nStates)-1) {
                    if (tesce->marker != 0) nt->marker = tesce->marker;
                    else nt->marker = markerMap[stemp->id];
                    debugif(DEBUG_DIAG, printlog("1: Cut %d Marked t from %d to %d with %d\n", markerMap[stemp->id], markerMap[nt->from->id], markerMap[nt->to->id], nt->marker))
                }

                b->nTrans++;
                struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans));
                nuovaTr->t = nt;
                struct ltrans *templtrans = b->states[tentra->from->id]->transitions;
                b->states[tentra->from->id]->transitions = nuovaTr;
                b->states[tentra->from->id]->transitions->next = templtrans;
                if (tentra->from != tesce->to) {
                    templtrans = b->states[tesce->to->id]->transitions;
                    nuovaTr = calloc(1, sizeof(struct ltrans));
                    nuovaTr->t = nt;
                    b->states[tesce->to->id]->transitions = nuovaTr;
                    b->states[tesce->to->id]->transitions->next = templtrans;
                }
                freeRegex(tesce->regex);
                memcpy(markerMap+i, markerMap+i+1, (nMarker - i)*sizeof(int));
                removeBehState(b, stemp);
                continua = true;
            }
        }
        if (continua) continue;
        for (stemp=b->states[i=0]; i<b->nStates; stemp=b->states[++i]) {       // Collasso gruppi di transitions che condividono partenze e arrivi in una
            foreachdecl(trans1, stemp->transitions) {
                struct ltrans *trans2, *nodoPrecedente = NULL;
                foreachtr(trans2, stemp->transitions) {
                    if (trans1->t != trans2->t && trans1->t->from == trans2->t->from && trans1->t->to == trans2->t->to
                    && (!mode || (mode &&trans2->t->marker == trans1->t->marker))) {
                        regexMake(trans1->t->regex, trans2->t->regex, trans1->t->regex, 'a', NULL);
                        debugif(DEBUG_DIAG, printlog("Removed t with mark %d\n", trans2->t->marker))
                        freeRegex(trans2->t->regex);
                        nodoPrecedente->next = trans2->next;
                        BehState * nodopartenza = trans2->t->from == stemp ? trans2->t->to : trans2->t->from;
                        struct ltrans *listaNelNodoDiPartenza = nodopartenza->transitions, *precedenteNelNodoDiPartenza = NULL;
                        while (listaNelNodoDiPartenza != NULL) {
                            if (listaNelNodoDiPartenza->t == trans2->t) {
                                if (precedenteNelNodoDiPartenza == NULL) nodopartenza->transitions = listaNelNodoDiPartenza->next;
                                else precedenteNelNodoDiPartenza->next = listaNelNodoDiPartenza->next;
                                break;
                            }
                            precedenteNelNodoDiPartenza = listaNelNodoDiPartenza;
                            listaNelNodoDiPartenza = listaNelNodoDiPartenza->next;
                        }
                        free(trans2->t);
                        b->nTrans--;
                        continua = true;
                        break;
                    }
                    nodoPrecedente = trans2;
                }
                if (continua) break;
            }
        }
        if (continua) continue;

        bool azioneEffettuataSuQuestoStato = false;
        for (stemp=b->states[i=1]; i<b->nStates-1; stemp=b->states[++i]) {
            BehTrans * autoTransizione = NULL;
            foreachdecl(trans, stemp->transitions) {
                if (trans->t->to == trans->t->from) {
                    autoTransizione = trans->t;
                    break;
                }
            }
        foreachdecl(trans1, stemp->transitions) {                                   // Transizione entrante
                if (trans1->t->to == stemp && trans1->t->from != stemp) {           // In trans1 c'è una entrante nel nodo      
                    foreachdecl(trans2, stemp->transitions) {                       // Transizione uscente
                        if (trans2->t->from == stemp && trans2->t->to != stemp) {   // In trans2 c'è una uscente dal nodo
                            azioneEffettuataSuQuestoStato = true;                  
                            BehTrans *nt = calloc(1, sizeof(BehTrans));
                            nt->from = trans1->t->from;
                            nt->to = trans2->t->to;
                            nt->t = tvuota;
                            nt->regex = emptyRegex(0);

                            struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans));
                            nuovaTr->t = nt;
                            nuovaTr->next = nt->from->transitions;
                            nt->from->transitions = nuovaTr;

                            if (nt->to != nt->from) {
                                struct ltrans *nuovaTr2 = calloc(1, sizeof(struct ltrans));
                                nuovaTr2->t = nt;
                                nuovaTr2->next = nt->to->transitions;
                                nt->to->transitions = nuovaTr2;
                            }
                            b->nTrans++;
                            
                            if (autoTransizione) regexMake(trans1->t->regex, trans2->t->regex, nt->regex, 'r', autoTransizione->regex);
                            else regexMake(trans1->t->regex, trans2->t->regex, nt->regex, 'c', NULL);

                            if (mode && nt->to->id == ((int)b->nStates)-1) {
                                if (trans2->t->marker != 0) nt->marker = trans2->t->marker;
                                else nt->marker = markerMap[stemp->id];
                                //nt->marker = markerMap[stemp->id];
                                debugif(DEBUG_DIAG, printlog("2: Marked t from %d to %d w marker %d\n", markerMap[nt->from->id], markerMap[nt->to->id], nt->marker))
                            }
                        }
                    }
                }
            }
            if (azioneEffettuataSuQuestoStato) {
                debugif(DEBUG_DIAG, printlog("Cut %d\n", markerMap[i]))
                memcpy(markerMap+i, markerMap+i+1, (nMarker - i)*sizeof(int));
                foreachdecl(trans, stemp->transitions)
                    freeRegex(trans->t->regex);
                removeBehState(b, stemp);
                break;
            }
        }
    }
    if (mode) {
        foreachdecl(lt, b->states[0]->transitions)
            if (lt->t->marker != 0) ret[lt->t->marker-1] = lt->t->regex;
    }
    else ret[0] = b->states[0]->transitions->t->regex;
    debugif(DEBUG_DIAG, printlog("-----\n"))
    return ret;
}