#include "header.h"

INLINE(Regex * emptyRegex(unsigned int size)) {
    Regex * new = malloc(sizeof(Regex));
    if (new == NULL) {
        printf(MSG_MEMERR);
        new = malloc(sizeof(Regex));
    }
    size = size <= REGEX ? REGEX : size;
    new->size = size;
    new->strlen = 0;
    new->concrete = false;
    new->bracketed = true;
    new->regex = malloc(size);
    if (new->regex == NULL) {
        printf(MSG_MEMERR);
        new->regex = malloc(size);
    }
    new->regex[0] = '\0';
    return new;
}

INLINE(void freeRegex(Regex *RESTRICT r)) {
    if (r->regex) free(r->regex);
    free(r);
}

Regex * regexCpy(Regex *RESTRICT src) {
    if (src == NULL) return NULL;
    assert((src->size > src->strlen && src->strlen > 0) || src->strlen==0);
    Regex * ret = malloc(sizeof(Regex));
    memcpy(ret, src, sizeof(Regex));
    if (ret->size>0) {
        ret->regex = malloc(ret->size);
        memcpy(ret->regex, src->regex, ret->strlen);
        ret->regex[ret->strlen] = '\0';
    }
    else ret->regex = NULL;
    return ret;
}

doC11(mtx_t mutex;)
bool hasInit=false;

void regexMake(Regex* s1, Regex* s2, Regex* d, char op, Regex *s3) {
    doC11(
        if (!hasInit) {
            hasInit=true;
            mtx_init(&mutex, mtx_plain);
        }
    )
    static char *RESTRICT regexBuf = NULL;
    static size_t regexBufLen = 0;
    if (regexBuf==NULL) regexBuf = calloc(regexBufLen=REGEX, 1);

    unsigned int strl1 = s1->strlen, strl2 = s2->strlen, strl3 = 0; //int strl1 = strlen(s1->regex), strl2 = strlen(s2->regex), strl3 = 0;
    if (s3 != NULL) strl3 = s3->strlen; //strl3 = strlen(s3->regex);
    char * solution = NULL;
    unsigned int solLen = 0;

    doC11(mtx_lock(&mutex);)
    if (strl1+strl2+strl3+6 > regexBufLen) {
        do regexBufLen *= (REGEXLEVER > 1.5 ? REGEXLEVER : 1.5);
        while (strl1+strl2+strl3+6 > regexBufLen);
        debugif(DEBUG_REGEX, printlog("REALLOC regexBuf w %lu\n", regexBufLen))
        regexBuf = realloc(regexBuf, regexBufLen);
    }
    
    debugif(DEBUG_REGEX, {
        assert(strl1 < s1->size && strl2 < s2->size && (s3 == NULL || strl3 < s3->size));
        assert(strl1 == strlen(s1->regex) && strl2 == strlen(s2->regex));
        assert(s3 == NULL || strl3 == strlen(s3->regex));
    })
    
    if (op == 'c') {                                                        // Concatenation
        if (strl1>0 && strl2>0) { // concat
            solLen = strl1+strl2;
            solution = regexBuf;
            d->bracketed = false;
            d->concrete = s1->concrete || s2->concrete;
            unsigned int minStrl = strl1<strl2? strl1:strl2, maxStrl = strl1>strl2? strl1:strl2;
            char *longer = strl1>strl2? s1->regex:s2->regex;
            if (minStrl+1==maxStrl && longer[minStrl]=='*' && memcmp(s1->regex, s2->regex, minStrl)==0) {
                if (d->regex == s2->regex || d->regex == s1->regex) {
                    if (d->size <= maxStrl) realloc(d->regex, maxStrl*REGEXLEVER);
                    d->regex[minStrl] = '+';
                    doC11(mtx_unlock(&mutex);)
                    return;
                }
                else sprintf(regexBuf, "%s+", s1->regex);
            }
            if (d->regex == s1->regex && s1->regex != s2->regex && s1->size > solLen) {
                s1->strlen = solLen;
                s1->regex[solLen] = '\0';
                doC11(mtx_unlock(&mutex);)  // Copy does not need mutex protection
                memcpy(s1->regex+strl1, s2->regex, strl2);
                doC99(assert(strlen(s1->regex) == solLen));
                return;
            }
            sprintf(regexBuf, "%s%s", s1->regex, s2->regex);
        }
        else if (strl1>0 && strl2==0) { // copy s1 in d
            solution = s1->regex;
            solLen = strl1;
            d->bracketed = s1->bracketed;
            d->concrete = s1->concrete;
        }
        else if (strl1==0 && strl2>0) {// copy s2 in d
            solution = s2->regex;
            solLen = strl2;
            d->bracketed = s2->bracketed;
            d->concrete = s2->concrete;
        }
        else if (strl2==0 && strl2==0) { // empty
            doC11(mtx_unlock(&mutex);)
            if (d->regex == NULL) {
                d->regex = malloc(REGEX);
                d->size = REGEX;
            }
            d->regex[0] = '\0';
            d->strlen = 0;
            d->bracketed = true;
            d->concrete = false;
            return;
        }
    }
    else if (op == 'a') {                                                   // Alternative
        bool streq12 = strl1==strl2 ? strcmp(s1->regex, s2->regex)==0 : false;
        if (strl1 > 0 && streq12) { // Copia s1 in d
            solution = s1->regex;
            solLen = strl1;
            d->bracketed = s1->bracketed;
            d->concrete = s1->concrete;
        }
        else if (strl1 > 0 && strl2 > 0) { // s1|s2
            solution = regexBuf;
            if (s1->bracketed && s2->bracketed) {
                solLen = strl1+strl2+1;
                if (d->regex==s1->regex && s1->size > solLen) {
                    solution = s1->regex;
                    solution[strl1] = '|';
                    memcpy(solution+strl1+1, s2->regex, strl2);
                    solution[solLen] = '\0';
                    assert(strlen(solution) == solLen);
                    s1->strlen = solLen;
                }
                else if (d->regex==s2->regex && s2->size > solLen) {
                    solution = s2->regex;
                    solution[strl2] = '|';
                    memcpy(solution+strl2+1, s1->regex, strl1);
                    solution[solLen] = '\0';
                    assert(strlen(solution) == solLen);
                    s2->strlen = solLen;
                }
                else sprintf(regexBuf, "%s|%s", s1->regex, s2->regex);
            }
            else if (s1->bracketed && !s2->bracketed) {
                solLen = strl1+strl2+3;
                if (d->regex==s1->regex && s1->size > solLen) {
                    solution = s1->regex;
                    sprintf(solution+strl1, "|(%s)", s2->regex);
                    assert(strlen(solution) == solLen);
                    s1->strlen = solLen;
                }
                else sprintf(regexBuf, "%s|(%s)", s1->regex, s2->regex);
            }
            else if (!s1->bracketed && s2->bracketed) {
                solLen = strl1+strl2+3;
                if (d->regex==s2->regex && s2->size > solLen) {
                    solution = s2->regex;
                    sprintf(solution+strl2, "|(%s)", s1->regex);
                    assert(strlen(solution) == solLen);
                    s2->strlen = solLen;
                }
                else sprintf(regexBuf, "(%s)|%s", s1->regex, s2->regex);
            } else {
                solLen = strl1+strl2+5;
                sprintf(regexBuf, "(%s)|(%s)", s1->regex, s2->regex);
            }
            d->bracketed = false;
            d->concrete = s1->concrete && s2->concrete;
        }
        else if (strl1 > 0 && strl2==0) {
            solution = regexBuf;
            if (s1->concrete) {
                if (s1->bracketed) {
                    solLen = strl1+1;
                    if (d->regex==s1->regex && d->size>solLen) {
                        solution = s1->regex;
                        solution[strl1] = '?';
                        solution[solLen] = '\0';
                    }
                    else sprintf(regexBuf, "%s?", s1->regex);
                }
                else {
                    solLen = strl1+3;
                    sprintf(regexBuf, "(%s)?", s1->regex);
                }
                d->bracketed = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solLen = strl1;
                strcpy(regexBuf, s1->regex);
                d->bracketed = s1->bracketed;
            }
            d->concrete = false;
        } else if (strl2 > 0) {
            solution = regexBuf;
            if (s2->concrete) {
                if (s2->bracketed) {
                    solLen = strl2+1;
                    if (d->regex==s2->regex && d->size > solLen) {
                        solution = s2->regex;
                        solution[strl2] = '?';
                        solution[solLen] = '\0';
                        assert(strlen(solution) == solLen);
                    }
                    else sprintf(regexBuf, "%s?", s2->regex);
                }
                else {
                    solLen = strl2+3;
                    sprintf(regexBuf, "(%s)?", s2->regex);
                }
                d->bracketed = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solLen = strl2;
                if (d->regex==s2->regex) solution = s2->regex;
                else strcpy(regexBuf, s2->regex);
                d->bracketed = s2->bracketed;
            }
            d->concrete = false;
        }
        else {
            doC11(mtx_unlock(&mutex);)
            if (d->regex == NULL) {
                d->regex = calloc(REGEX, 1);
                d->size = REGEX;
            }
            d->regex[0] = '\0';
            d->strlen = 0;
            d->bracketed = true;
            d->concrete = false;
            return;
        }
    }
    else if (op == 'r') {                                                   // Recursion
        if (strl3 == 0) {
            doC11(mtx_unlock(&mutex);)
            regexMake(s1, s2, d, 'c', NULL);
        }
        else {
            assert(d->regex!=s1->regex && d->regex!= s2->regex && d->regex!=s3->regex);
            d->size = strl1+strl2+strl3+4 < REGEX ? REGEX : strl1+strl2+strl3+4;
            d->regex = realloc(d->regex, d->size);
            d->regex[0] = '\0';
            bool streq13 = strl1==strl3 ? strcmp(s1->regex, s3->regex)==0 : false;
            if (strl1 + strl2 >0) {
                unsigned int minStrl = strl1<strl3? strl1:strl3, maxStrl = strl1>strl3? strl1:strl3;
                char *longer = strl1>strl3? s1->regex:s3->regex;
                if (streq13 && s1->bracketed){
                    sprintf(d->regex, "%s+%s", s1->regex, s2->regex);
                    d->strlen=strl1+strl2+1;
                }
                else if (minStrl+1==maxStrl && longer[minStrl]=='*' && memcmp(s1->regex, s3->regex, minStrl)==0)
                    sprintf(d->regex, "%s+%s", s1->regex, s2->regex);
                else if (s3->bracketed) {
                    if (s3->regex[strl3-1]=='?') {
                        sprintf(d->regex, "%s%s", s1->regex, s3->regex);
                        d->regex[strl1+strl3-1]='*';
                        strcpy(d->regex+strl1+strl3, s2->regex);
                        d->strlen = strl1+strl2+strl3;
                    }
                    else {
                        sprintf(d->regex, "%s%s*%s", s1->regex, s3->regex, s2->regex);
                        d->strlen = strl1+strl2+strl3+1;
                    }
                    assert(strlen(d->regex) == d->strlen);
                } else {
                    sprintf(d->regex, "%s(%s)*%s", s1->regex, s3->regex, s2->regex);
                    d->strlen = strl1+strl2+strl3+3;
                    assert(strlen(d->regex) == d->strlen);
                }
                d->bracketed = false;
                d->concrete = s1->concrete && s2->concrete && s3->concrete;
            }
            else {
                if (s3->bracketed) {
                    if (s3->regex[strl3-1]=='?') {
                        memcpy(d->regex, s3->regex, strl3);
                        d->regex[strl3-1] = '*';
                        d->regex[strl3] = '\0';
                        d->strlen = strl3;
                        assert(strlen(d->regex) == d->strlen);
                    }
                    else {
                        sprintf(d->regex, "%s*", s3->regex);
                        d->strlen = strl3+1;
                        assert(strlen(d->regex) == d->strlen);
                    }
                }
                else {
                    sprintf(d->regex, "(%s)*", s3->regex);
                    d->strlen = strl3+3;
                    assert(strlen(d->regex) == d->strlen);
                }
                d->bracketed = true; // Se anche termina con *, non importa
                d->concrete = s3->concrete;
            }
        }
        doC11(mtx_unlock(&mutex);)
        return;
    }

    d->strlen = solLen;
    if (solution != d->regex) {
        if (d->size <= solLen) {
            if (d->size == 0) {
                d->regex = malloc(solLen*REGEXLEVER);
                d->regex[0] = '\0';
            } else {
                debugif(DEBUG_REGEX, printlog("REALLOC d->regex w %u was %u \n", (unsigned int)(solLen*REGEXLEVER), d->size))
                d->regex = realloc(d->regex, solLen*REGEXLEVER);
            }
            d->size = solLen*REGEXLEVER;
        }
        memcpy(d->regex, solution, solLen);
        d->regex[solLen] = '\0';
        debugif(DEBUG_REGEX, if (strlen(d->regex) != d->strlen) {printlog("Different strlen %ld vs %u - '%s' '", strlen(d->regex), d->strlen, d->regex); for(int i=0; i<d->strlen; i++)printf("%c",d->regex[i]);printf("'\n"); fflush(stdout);exit(0);});
    }
    doC11(mtx_unlock(&mutex);)
}

INLINE(bool exitCondition(BehSpace *RESTRICT b, bool mode2)) {
    if (mode2) {
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

Regex** diagnostics(BehSpace *RESTRICT b, bool mode2) {
    unsigned int i=0, j=0, nMarker = b->nStates+2;
    int markerMap[nMarker], k;
    for (; i<nMarker; i++) 
        markerMap[i] = i; // Including α and ω
    Regex ** ret = calloc(mode2? nMarker-2 : 1, sizeof(Regex*));
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
                    sprintf(trans->t->regex->regex, "r%hu", trans->t->t->fault);
                    trans->t->regex->strlen = 1+(unsigned int)ceilf(log10f((float)(trans->t->t->fault+1)));
                    trans->t->regex->concrete = true;
                }
            }
        }
    }
    
    debugif(DEBUG_DIAG, printlog("-----\n"));
    while (!exitCondition(b, mode2)) {
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
                if (mode2 && nt->to->id == ((int)b->nStates)-1) {
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
                    && (!mode2 || (mode2 &&trans2->t->marker == trans1->t->marker))) {
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

                            if (mode2 && nt->to->id == ((int)b->nStates)-1) {
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
    if (mode2) {
        foreachdecl(lt, b->states[0]->transitions)
            if (lt->t->marker != 0) ret[lt->t->marker-1] = lt->t->regex;
    }
    else ret[0] = b->states[0]->transitions->t->regex;
    debugif(DEBUG_DIAG, printlog("-----\n"))
    return ret;
}