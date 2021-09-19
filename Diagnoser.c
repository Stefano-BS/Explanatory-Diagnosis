#include "header.h"
char *tempRegex;
int tempRegexLen;

void regexMake(TransizioneRete* s1, TransizioneRete* s2, TransizioneRete* d, char op, TransizioneRete *autoTransizione) {
    int strl1 = strlen(s1->regex), strl2 = strlen(s2->regex), strl3 = 0;
    if (autoTransizione != NULL) strl3 = strlen(autoTransizione->regex);
    bool streq = strl1==strl2 ? strcmp(s1->regex, s2->regex)==0 : false;
    char * solution;
    int solutionLen;

    if (strl1+strl2+strl3+4 > tempRegexLen) {
        tempRegexLen = tempRegexLen*(REGEXLEVER > 1.5 ? REGEXLEVER : 1.5);
        tempRegex = realloc(tempRegex, tempRegexLen);
    }

    if (op == 'c') { // Concat
        if (strl1>0 && strl2>0) { // concat
            solutionLen = strl1+strl2+1 < REGEX? REGEX : strl1+strl2+1;
            solution = tempRegex;
            d->parentesizzata = false;
            d->concreta = s1->concreta || s2->concreta;
            sprintf(tempRegex, "%s%s", s1->regex, s2->regex);
        }
        else if (strl1>0 && strl2==0) { // copia s1 in d
            solution = s1->regex;
            solutionLen = s1->dimRegex;
            d->parentesizzata = s1->parentesizzata;
            d->concreta = s1->concreta;
        }
        else if (strl1==0 && strl2>0) {// Copia s2 in d
            solution = s2->regex;
            solutionLen = s2->dimRegex;
            d->parentesizzata = s2->parentesizzata;
            d->concreta = s2->concreta;
        }
        else if (strl2==0 && strl2==0) { // d null
            d->regex = calloc(REGEX, 1);
            d->dimRegex = REGEX;
            d->parentesizzata = true;
            d->concreta = false;
            return;
        }
    }
    else if (op == 'a') {
        if (strl1 > 0 && streq) { // Copia s1 in d
            solution = s1->regex;
            solutionLen = s1->dimRegex;
            d->parentesizzata = s1->parentesizzata;
            d->concreta = s1->concreta;
        }
        else if (strl1 > 0 && strl2 > 0) { // s1|s2
            solution = tempRegex;
            d->parentesizzata = false;
            d->concreta = s1->concreta && s2->concreta;
            if (s1->parentesizzata && s2->parentesizzata) {
                solutionLen = strl1+strl2+2 < REGEX? REGEX : strl1+strl2+2;
                sprintf(tempRegex, "%s|%s", s1->regex, s2->regex);
            } else if (s1->parentesizzata && !s2->parentesizzata) {
                solutionLen = strl1+strl2+4 < REGEX? REGEX : strl1+strl2+4;
                sprintf(tempRegex, "%s|(%s)", s1->regex, s2->regex);
            } else if (!s1->parentesizzata && s2->parentesizzata) {
                solutionLen = strl1+strl2+4 < REGEX? REGEX : strl1+strl2+4;
                sprintf(tempRegex, "(%s)|%s", s1->regex, s2->regex);
            } else {
                solutionLen = strl1+strl2+6 < REGEX? REGEX : strl1+strl2+6;
                sprintf(tempRegex, "(%s)|(%s)", s1->regex, s2->regex);
            }
        } else if (strl1 > 0 && strl2==0) {
            solution = tempRegex;
            if (s1->concreta) {
                if (s1->parentesizzata) {
                    solutionLen = strl1+2 < REGEX? REGEX : strl1+2;
                    sprintf(tempRegex, "%s?", s1->regex);
                }
                else {
                    solutionLen = strl1+4 < REGEX? REGEX : strl1+4;
                    sprintf(tempRegex, "(%s)?", s1->regex);
                }
                d->parentesizzata = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solutionLen = strl1+1 < REGEX? REGEX : strl1+1;
                strcpy(tempRegex, s1->regex);
                d->parentesizzata = s1->parentesizzata;
            }
            d->concreta = false;
        } else if (strl2 > 0) {
            solution = tempRegex;
            if (s2->concreta) {
                if (s2->parentesizzata) {
                    solutionLen = strl2+2 < REGEX? REGEX : strl2+2;
                    sprintf(tempRegex, "%s?", s2->regex);
                }
                else {
                    solutionLen = strl2+4 < REGEX? REGEX : strl2+4;
                    sprintf(tempRegex, "(%s)?", s2->regex);
                }
                d->parentesizzata = true;   // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
            }
            else {
                solutionLen = strl2+1 < REGEX? REGEX : strl2+1;
                strcpy(tempRegex, s2->regex);
                d->parentesizzata = s2->parentesizzata;
            }
            d->concreta = false;
        } else {
            d->regex = calloc(REGEX, 1);
            d->dimRegex = REGEX;
            d->parentesizzata = true;
            d->concreta = false;
            return;
        }
    }
    else if (op == 'r') {
        solutionLen = strl1+strl2+strl3+4 < REGEX ? REGEX : strl1+strl2+strl3+4;
        d->regex = calloc(solutionLen, 1);
        d->dimRegex = solutionLen;
        if (strl3 == 0) regexMake(s1, s2, d, 'c', NULL);
        else {
            if (strl1 + strl2 >0) {
                if (autoTransizione->parentesizzata) sprintf(d->regex, "%s%s*%s", s1->regex, autoTransizione->regex, s2->regex);
                else sprintf(d->regex, "%s(%s)*%s", s1->regex, autoTransizione->regex, s2->regex);
                d->parentesizzata = false;
                d->concreta = s1->concreta && s2->concreta && autoTransizione->concreta;
            }
            else {
                if (autoTransizione->parentesizzata) {
                    if (autoTransizione->regex[strl3]=='?') autoTransizione->regex[strl3] = '\0';
                    sprintf(d->regex, "%s*", autoTransizione->regex);
                }
                else sprintf(d->regex, "(%s)*", autoTransizione->regex);
                d->parentesizzata = true; // Se anche termina con *, non importa
                d->concreta = autoTransizione->concreta;
            }
        }
        return;
    }

    if (d->dimRegex < solutionLen) {
        if (d->dimRegex == 0) d->regex = calloc(solutionLen*REGEXLEVER, 1);
        else d->regex = realloc(d->regex, solutionLen*REGEXLEVER);
        d->dimRegex = solutionLen*REGEXLEVER;
    }
    if (solution != d->regex) strcpy(d->regex, solution);
}

char* diagnostica(BehSpace *b) {
    int i=0, j=0;
    StatoRete * stemp = NULL;
    tempRegexLen = REGEX*b->nStates*b->nTrans;
    tempRegex = malloc(tempRegexLen);

    //Arricchimento spazio con nuovi stati iniziale e finale
    Transizione *tvuota = calloc(1, sizeof(Transizione));
    for (stemp=b->states[j=b->nStates-1]; j>=0; stemp=b->states[--j])          // Generazione nuovo stato iniziale
        stemp->id++;
    alloc1(b, 's');
    memmove(b->states+1, b->states, b->nStates*sizeof(StatoRete*));
    b->nStates++;
    stemp = calloc(1, sizeof(StatoRete));
    b->states[0] = stemp;
    stemp->id = 0;
    TransizioneRete * nuovaTr = calloc(1, sizeof(TransizioneRete));
    nuovaTr->da = stemp;
    nuovaTr->a = b->states[1];
    nuovaTr->t = tvuota;
    stemp->transizioni = calloc(1, sizeof(struct ltrans));
    stemp->transizioni->t = nuovaTr;
    struct ltrans *vecchiaTesta = b->states[1]->transizioni;
    b->states[1]->transizioni = calloc(1, sizeof(struct ltrans));
    b->states[1]->transizioni->t = nuovaTr;
    b->states[1]->transizioni->prossima = vecchiaTesta;
    b->nTrans++;

    alloc1(b, 's');
    StatoRete * fine = calloc(1, sizeof(StatoRete));                            // Generazione nuovo stato finale
    fine->id = b->nStates;
    fine->finale = true;
    for (stemp = b->states[i=1]; i<b->nStates; stemp=b->states[++i]) {         // Collegamento ex stati finali col nuovo
        if (stemp->finale) {
            TransizioneRete * trFinale = calloc(1, sizeof(TransizioneRete));
            trFinale->da = stemp;
            trFinale->a = fine;
            trFinale->t = tvuota;
            struct ltrans *vecchiaTesta = stemp->transizioni;
            stemp->transizioni = calloc(1, sizeof(struct ltrans));
            stemp->transizioni->t = trFinale;
            stemp->transizioni->prossima = vecchiaTesta;
            vecchiaTesta = fine->transizioni;
            fine->transizioni = calloc(1, sizeof(struct ltrans));
            fine->transizioni->t = trFinale;
            fine->transizioni->prossima = vecchiaTesta;
            b->nTrans++;
            stemp->finale = false;
        }
    }
    b->states[b->nStates++] = fine;

    for (stemp=b->states[j=0]; j<b->nStates; stemp=b->states[++j]) {           // Formazione regex elementari
        struct ltrans *trans = stemp->transizioni;
        while (trans != NULL) {
            if (trans->t->da == stemp) {
                trans->t->regex = calloc(REGEX, 1);
                trans->t->dimRegex = REGEX;
                trans->t->parentesizzata = true;
                if (trans->t->t->ril >0) {
                    sprintf(trans->t->regex, "r%d", trans->t->t->ril);
                    trans->t->concreta = true;
                }
            }
            trans = trans->prossima;
        }
    }
    
    while (b->nTrans>1) {                                                        // Ciclo di diagnostica
        bool continua = false;
        for (stemp = b->states[i=0]; i<b->nStates; stemp=b->states[++i]) {     // Semplificazione serie -> unità
            TransizioneRete *tentra, *tesce;
            if (stemp->transizioni != NULL && stemp->transizioni->prossima != NULL && stemp->transizioni->prossima->prossima == NULL &&
            (((tesce = stemp->transizioni->t)->da == stemp && (tentra = stemp->transizioni->prossima->t)->a == stemp)
            || ((tentra = stemp->transizioni->t)->a == stemp && (tesce = stemp->transizioni->prossima->t)->da == stemp))
            && tesce->da != tesce->a && tentra->a != tentra->da)  {             // Elemento interno alla sequenza
                TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                nt->da = tentra->da;
                nt->a = tesce->a;
                nt->t = tvuota;
                regexMake(tentra, tesce, nt, 'c', NULL);
                
                b->nTrans++;
                struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans));
                nuovaTr->t = nt;
                struct ltrans *templtrans = b->states[tentra->da->id]->transizioni;
                b->states[tentra->da->id]->transizioni = nuovaTr;
                b->states[tentra->da->id]->transizioni->prossima = templtrans;
                if (tentra->da != tesce->a) {
                    templtrans = b->states[tesce->a->id]->transizioni;
                    nuovaTr = calloc(1, sizeof(struct ltrans));
                    nuovaTr->t = nt;
                    b->states[tesce->a->id]->transizioni = nuovaTr;
                    b->states[tesce->a->id]->transizioni->prossima = templtrans;
                }
                free(tentra->regex);
                free(tesce->regex);
                eliminaStato(b, i);
                continua = true;
            }
        }
        if (continua) continue;
        for (stemp=b->states[i=0]; i<b->nStates; stemp=b->states[++i]) {       // Collasso gruppi di transizioni che condividono partenze e arrivi in una
            struct ltrans *trans1 = stemp->transizioni;
            while (trans1 != NULL) {
                struct ltrans *trans2 = stemp->transizioni, *nodoPrecedente = NULL;
                while (trans2 != NULL) {
                    if (trans1->t != trans2->t && trans1->t->da == trans2->t->da && trans1->t->a == trans2->t->a) {
                        regexMake(trans1->t, trans2->t, trans1->t, 'a', NULL);
                        free(trans2->t->regex);
                        nodoPrecedente->prossima = trans2->prossima;
                        StatoRete * nodopartenza = trans2->t->da == stemp ? trans2->t->a : trans2->t->da;
                        struct ltrans *listaNelNodoDiPartenza = nodopartenza->transizioni, *precedenteNelNodoDiPartenza = NULL;
                        while (listaNelNodoDiPartenza != NULL) {
                            if (listaNelNodoDiPartenza->t == trans2->t) {
                                if (precedenteNelNodoDiPartenza == NULL) nodopartenza->transizioni = listaNelNodoDiPartenza->prossima;
                                else precedenteNelNodoDiPartenza->prossima = listaNelNodoDiPartenza->prossima;
                                break;
                            }
                            precedenteNelNodoDiPartenza = listaNelNodoDiPartenza;
                            listaNelNodoDiPartenza = listaNelNodoDiPartenza->prossima;
                        }
                        free(trans2->t);
                        b->nTrans--;
                        continua = true;
                        break;
                    }
                    nodoPrecedente = trans2;
                    trans2 = trans2->prossima;
                }
                if (continua) break;
                trans1 = trans1->prossima;
            }
        }
        
        if (continua) continue;

        bool azioneEffettuataSuQuestoStato = false;
        for (stemp=b->states[i=1]; i<b->nStates-1; stemp=b->states[++i]) {
            TransizioneRete * autoTransizione = NULL;
            struct ltrans *trans = stemp->transizioni;
            for (; trans != NULL; trans = trans->prossima) {
                if (trans->t->a == trans->t->da) {
                    autoTransizione = trans->t;
                    break;
                }
            }
            struct ltrans *trans1 = stemp->transizioni;
            while (trans1 != NULL) {
                if (trans1->t->a == stemp && trans1->t->da != stemp) {       // Transizione entrante
                    struct ltrans *trans2 = stemp->transizioni;             // In trans1 c'è una entrante nel nodo
                    while (trans2 != NULL) {
                        if (trans2->t->da == stemp && trans2->t->a != stemp) {      // Transizione uscente
                            azioneEffettuataSuQuestoStato = true;                  // In trans2 c'è una uscente dal nodo
                            TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                            nt->da = trans1->t->da;
                            nt->a = trans2->t->a;
                            nt->t = tvuota;

                            struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans));
                            nuovaTr->t = nt;
                            nuovaTr->prossima = nt->da->transizioni;
                            nt->da->transizioni = nuovaTr;

                            if (nt->a != nt->da) {
                                struct ltrans *nuovaTr2 = calloc(1, sizeof(struct ltrans));
                                nuovaTr2->t = nt;
                                nuovaTr2->prossima = nt->a->transizioni;
                                nt->a->transizioni = nuovaTr2;
                            }
                            b->nTrans++;
                            
                            if (autoTransizione) regexMake(trans1->t, trans2->t, nt, 'r', autoTransizione);
                            else regexMake(trans1->t, trans2->t, nt, 'c', NULL);
                        }
                        trans2 = trans2->prossima;
                    }
                }
                trans1 = trans1->prossima;
            }
            if (azioneEffettuataSuQuestoStato) {
                eliminaStato(b, i);
                break;
            }
        }
    }
    free(tempRegex);
    return b->states[0]->transizioni->t->regex;
}