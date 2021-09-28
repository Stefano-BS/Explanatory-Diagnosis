#include "header.h"


INLINE(Regex * emptyRegex(int))
Regex * emptyRegex(int size) {
    Regex * new = malloc(sizeof(Regex));
    size = size <= REGEX ? REGEX : size;
    new->size = size;
    new->concrete = false;
    new->bracketed = true;
    new->regex = malloc(size);
    new->regex[0] = '\0';
    return new;
}

INLINE(void freeRegex(Regex *RESTRICT))
void freeRegex(Regex *RESTRICT r) {
    if (r->regex) free(r->regex);
    free(r);
}

Regex * regexCpy(Regex *RESTRICT src) {
    if (src == NULL) return NULL;
    Regex * ret = malloc(sizeof(Regex));
    memcpy(ret, src, sizeof(Regex));
    if (ret->size>0) {
        ret->regex = malloc(ret->size);
        strncpy(ret->regex, src->regex, ret->size);
        ret->regex[ret->size-1] = '\0';
    }
    else ret->regex = NULL;
    return ret;
}

void regexMake(Regex* s1, Regex* s2, Regex* d, char op, Regex *autoTransizione) {
    static char *regexBuf = NULL;
    static int regexBufLen = 0;
    if (regexBuf==NULL) regexBuf = calloc(regexBufLen=REGEX, 1);

    int strl1 = strlen(s1->regex), strl2 = strlen(s2->regex), strl3 = 0;
    if (autoTransizione != NULL) strl3 = strlen(autoTransizione->regex);
    bool streq = strl1==strl2 ? strcmp(s1->regex, s2->regex)==0 : false;
    char * solution = NULL;
    int solutionLen = 0;

    if (strl1+strl2+strl3+6 > regexBufLen) {
        do regexBufLen *= (REGEXLEVER > 1.5 ? REGEXLEVER : 1.5);
        while (strl1+strl2+strl3+6 > regexBufLen);
        printlog("REALLOC regexBuf w %d was %d\n", regexBufLen);
        regexBuf = realloc(regexBuf, regexBufLen);
    }

    if (op == 'c') { // Concat
        if (strl1>0 && strl2>0) { // concat
            solutionLen = strl1+strl2+1 < REGEX? REGEX : strl1+strl2+1;
            solution = regexBuf;
            d->bracketed = false;
            d->concrete = s1->concrete || s2->concrete;
            sprintf(regexBuf, "%s%s", s1->regex, s2->regex);
        }
        else if (strl1>0 && strl2==0) { // copia s1 in d
            solution = s1->regex;
            solutionLen = s1->size;
            d->bracketed = s1->bracketed;
            d->concrete = s1->concrete;
        }
        else if (strl1==0 && strl2>0) {// Copia s2 in d
            solution = s2->regex;
            solutionLen = s2->size;
            d->bracketed = s2->bracketed;
            d->concrete = s2->concrete;
        }
        else if (strl2==0 && strl2==0) { // d null
            if (d->regex == NULL) {
                d->regex = calloc(REGEX, 1);
                d->size = REGEX;
            } else d->regex[0] = '\0';
            d->bracketed = true;
            d->concrete = false;
            return;
        }
    }
    else if (op == 'a') {
        if (strl1 > 0 && streq) { // Copia s1 in d
            solution = s1->regex;
            solutionLen = s1->size;
            d->bracketed = s1->bracketed;
            d->concrete = s1->concrete;
        }
        else if (strl1 > 0 && strl2 > 0) { // s1|s2
            solution = regexBuf;
            d->bracketed = false;
            d->concrete = s1->concrete && s2->concrete;
            if (s1->bracketed && s2->bracketed) {
                solutionLen = strl1+strl2+2 < REGEX? REGEX : strl1+strl2+2;
                sprintf(regexBuf, "%s|%s", s1->regex, s2->regex);
            } else if (s1->bracketed && !s2->bracketed) {
                solutionLen = strl1+strl2+4 < REGEX? REGEX : strl1+strl2+4;
                sprintf(regexBuf, "%s|(%s)", s1->regex, s2->regex);
            } else if (!s1->bracketed && s2->bracketed) {
                solutionLen = strl1+strl2+4 < REGEX? REGEX : strl1+strl2+4;
                sprintf(regexBuf, "(%s)|%s", s1->regex, s2->regex);
            } else {
                solutionLen = strl1+strl2+6 < REGEX? REGEX : strl1+strl2+6;
                sprintf(regexBuf, "(%s)|(%s)", s1->regex, s2->regex);
            }
        } else if (strl1 > 0 && strl2==0) {
            solution = regexBuf;
            if (s1->concrete) {
                if (s1->bracketed) {
                    solutionLen = strl1+2 < REGEX? REGEX : strl1+2;
                    sprintf(regexBuf, "%s?", s1->regex);
                }
                else {
                    solutionLen = strl1+4 < REGEX? REGEX : strl1+4;
                    sprintf(regexBuf, "(%s)?", s1->regex);
                }
                d->bracketed = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solutionLen = strl1+1 < REGEX? REGEX : strl1+1;
                strcpy(regexBuf, s1->regex);
                d->bracketed = s1->bracketed;
            }
            d->concrete = false;
        } else if (strl2 > 0) {
            solution = regexBuf;
            if (s2->concrete) {
                if (s2->bracketed) {
                    solutionLen = strl2+2 < REGEX? REGEX : strl2+2;
                    sprintf(regexBuf, "%s?", s2->regex);
                }
                else {
                    solutionLen = strl2+4 < REGEX? REGEX : strl2+4;
                    sprintf(regexBuf, "(%s)?", s2->regex);
                }
                d->bracketed = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solutionLen = strl2+1 < REGEX? REGEX : strl2+1;
                strcpy(regexBuf, s2->regex);
                d->bracketed = s2->bracketed;
            }
            d->concrete = false;
        } else {
            d->regex = calloc(REGEX, 1);
            d->size = REGEX;
            d->bracketed = true;
            d->concrete = false;
            return;
        }
    }
    else if (op == 'r') {
        solutionLen = strl1+strl2+strl3+4 < REGEX ? REGEX : strl1+strl2+strl3+4;
        d->regex = malloc(solutionLen);
        d->regex[0] = '\0';
        d->size = solutionLen;
        if (strl3 == 0) regexMake(s1, s2, d, 'c', NULL);
        else {
            if (strl1 + strl2 >0) {
                if (autoTransizione->bracketed) sprintf(d->regex, "%s%s*%s", s1->regex, autoTransizione->regex, s2->regex);
                else sprintf(d->regex, "%s(%s)*%s", s1->regex, autoTransizione->regex, s2->regex);
                d->bracketed = false;
                d->concrete = s1->concrete && s2->concrete && autoTransizione->concrete;
            }
            else {
                if (autoTransizione->bracketed) {
                    if (autoTransizione->regex[strl3]=='?') autoTransizione->regex[strl3] = '\0';
                    sprintf(d->regex, "%s*", autoTransizione->regex);
                }
                else sprintf(d->regex, "(%s)*", autoTransizione->regex);
                d->bracketed = true; // Se anche termina con *, non importa
                d->concrete = autoTransizione->concrete;
            }
        }
        return;
    }

    if (d->size < solutionLen) {
        if (d->size == 0) {
            d->regex = malloc(solutionLen*REGEXLEVER);
            d->regex[0] = '\0';
        } else {
            printlog("REALLOC d->regex w %d was %d \n", solutionLen*REGEXLEVER, d->size);
            d->regex = realloc(d->regex, solutionLen*REGEXLEVER);
        }
        d->size = solutionLen*REGEXLEVER;
    }
    if (solution != d->regex) {
        strncpy(d->regex, solution, solutionLen);
        d->regex[d->size-1] = '\0';
    }
}

INLINE(bool exitCondition(BehSpace *RESTRICT, bool))
bool exitCondition(BehSpace *RESTRICT b, bool mode2) {
    if (mode2) {
        if (b->nStates>2) return false;
        foreachdecl(lt, b->states[0]->transitions) {
            printlog("from %d to %d mark %d\n",lt->t->from->id, lt->t->to->id, lt->t->marker);
            foreachdecl(lt2, lt->next) {
                if (lt->t->marker != 0 && lt->t->marker == lt2->t->marker) return false;
            }
        }
        return true;
    }
    else return b->nTrans<=1;
}

Regex** diagnostics(BehSpace *b, bool mode2) {
    int i=0, j=0, nMarker = b->nStates+2, markerMap[nMarker];
    for (; i<nMarker; i++) 
        markerMap[i] = i; // Including α and ω
    Regex ** ret = calloc(mode2? nMarker-2 : 1, sizeof(Regex*));
    BehState * stemp = NULL;
    // tempRegex = emptyRegex(REGEX*b->nStates*b->nTrans);

    //Arricchimento spazio con nuovi stati iniziale e finale
    Trans *tvuota = calloc(1, sizeof(Trans));
    for (stemp=b->states[j=b->nStates-1]; j>=0; stemp=b->states[--j])          // New initial state α
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
    BehState * fine = calloc(1, sizeof(BehState));                            // New final state ω
    fine->id = b->nStates;
    fine->flags = FLAG_FINAL;
    for (stemp = b->states[i=1]; i<b->nStates; stemp=b->states[++i]) {         // Transitions from ex final states/all states to ω
        if (mode2 || (stemp->flags & FLAG_FINAL)) {
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
                    sprintf(trans->t->regex->regex, "r%d", trans->t->t->fault);
                    trans->t->regex->concrete = true;
                }
            }
        }
    }
    
    printlog("-----\n");
    while (!exitCondition(b, mode2)) {
        // for(i=0;i<b->nStates;i++)printlog("%d ",markerMap[i]);printlog("\n");
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
                nt->regex = emptyRegex(0);
                regexMake(tentra->regex, tesce->regex, nt->regex, 'c', NULL);
                if (mode2 && nt->to->id == b->nStates-1) {
                    if (tesce->marker != 0) nt->marker = tesce->marker;
                    else nt->marker = markerMap[stemp->id];
                    printlog("1: Cut %d Marked t from %d to %d with %d\n", markerMap[stemp->id], markerMap[nt->from->id], markerMap[nt->to->id], nt->marker);
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
                freeRegex(tentra->regex);
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
                foreach(trans2, stemp->transitions) {
                    if (trans1->t != trans2->t && trans1->t->from == trans2->t->from && trans1->t->to == trans2->t->to
                    && (!mode2 || (mode2 &&trans2->t->marker == trans1->t->marker))) {
                        regexMake(trans1->t->regex, trans2->t->regex, trans1->t->regex, 'a', NULL);
                        printlog("Removed t with mark %d\n", trans2->t->marker);
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
                if (trans1->t->to == stemp && trans1->t->from != stemp) {             // In trans1 c'è una entrante nel nodo      
                    foreachdecl(trans2, stemp->transitions) {                     // Transizione uscente
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

                            if (mode2 && nt->to->id == b->nStates-1) {
                                if (trans2->t->marker != 0) nt->marker = trans2->t->marker;
                                else nt->marker = markerMap[stemp->id];
                                //nt->marker = markerMap[stemp->id];
                                printlog("2: Marked t from %d to %d w marker %d\n", markerMap[nt->from->id], markerMap[nt->to->id], nt->marker);
                            }
                        }
                    }
                }
            }
            if (azioneEffettuataSuQuestoStato) {
                printlog("Cut %d\n", markerMap[i]);
                memcpy(markerMap+i, markerMap+i+1, (nMarker - i)*sizeof(int));
                removeBehState(b, stemp);
                break;
            }
        }
    }
    // freeRegex(tempRegex);
    // tempRegex = NULL;
    if (mode2) {
        foreachdecl(lt, b->states[0]->transitions)
            if (lt->t->marker != 0) ret[lt->t->marker-1] = lt->t->regex;
    }
    else ret[0] = b->states[0]->transitions->t->regex;
    printlog("-----\n");
    return ret;
}