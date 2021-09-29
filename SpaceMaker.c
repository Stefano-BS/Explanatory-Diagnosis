#include "header.h"

bool *RESTRICT ok;

bool enlargeBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT precedente, BehState *RESTRICT nuovo, Trans *RESTRICT mezzo, int loss) {
    int i;
    bool giaPresente = false;
    BehState *s;
    if (b->nStates>0) {
        for (s=b->states[i=0]; i<b->nStates; s=(i<b->nStates ? b->states[++i] : s)) { // Per ogni stato dello spazio comportamentale
            if (memcmp(nuovo->componentStatus, s->componentStatus, ncomp*sizeof(int)) == 0
            && memcmp(nuovo->linkContent, s->linkContent, nlink*sizeof(int)) == 0) {
                giaPresente = (loss>0 ? nuovo->obsIndex == s->obsIndex : true);
                if (giaPresente) {
                    freeBehState(nuovo); // Questa istruzione previene la duplicazione in memoria di stati con stessa semantica
                    nuovo = s;  // Non è solo una questione di prestazioni, ma di mantenimento relazione biunivoca ptr <-> id
                    break;
                }
            }
        }
    }
    if (giaPresente) { // Già c'era lo stato, ma la transizione non è detto
        struct ltrans * trans = precedente->transitions;
        nuovo->id = s->id;
        while (trans != NULL) {
            if (trans->t->to == nuovo && trans->t->t->id == mezzo->id) return false;
            trans = trans->next;
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        alloc1(b, 's');
        nuovo->id = b->nStates;
        b->states[b->nStates++] = nuovo;
    }
    if (mezzo != NULL) { // Lo stato iniziale, ad esempio ha NULL
        BehTrans * nuovaTransRete = calloc(1, sizeof(BehTrans));
        nuovaTransRete->to = nuovo;
        nuovaTransRete->from = precedente;
        nuovaTransRete->t = mezzo;
        nuovaTransRete->regex = NULL;

        struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
        nuovaLTr->t = nuovaTransRete;
        nuovaLTr->next = nuovo->transitions;//statiSpazio[nuovo->id]->transitions;
        nuovo->transitions = nuovaLTr; //statiSpazio[nuovo->id]->transitions = nuovaLTr;
        
        if (nuovo != precedente) {
            nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->next = precedente->transitions; //statiSpazio[precedente->id]->transitions;
            precedente->transitions = nuovaLTr; //statiSpazio[precedente->id]->transitions = nuovaLTr;
        }

        b->nTrans++;
    }
    return !giaPresente;
}

void generateBehavioralSpace(BehSpace *RESTRICT b, BehState *RESTRICT attuale, int*RESTRICT obs, int loss) {
    Component *c;
    int i, j, k;
    for (c=components[i=0]; i<ncomp;  c=components[++i]){
        Trans *t;
        int nT = c->nTrans;
        if (nT == 0) continue;
        for (t=c->transitions[j=0]; j<nT; t=(j<nT ? c->transitions[++j] : t)) {
            if ((attuale->componentStatus[c->intId] == t->from) &&                          // Transizione abilitata se stato attuale = from
            (t->idIncomingEvent == VUOTO || t->idIncomingEvent == attuale->linkContent[t->linkIn->intId])) { // Ok eventi in ingresso
                if (loss>0 && ((attuale->obsIndex>=loss && t->obs > 0) ||    // Osservate tutte, non se ne possono più vedere
                (attuale->obsIndex<loss && t->obs > 0 && t->obs != obs[attuale->obsIndex])))
                    continue; // Transizione non compatibile con l'osservazione lineare
                bool ok = true;
                for (k=0; k<t->nOutgoingEvents; k++) {                                        // I link di uscita sono vuoti
                    if (attuale->linkContent[t->linkOut[k]->intId] != VUOTO
                    && !(t->idIncomingEvent != VUOTO && t->linkIn->intId == t->linkOut[k]->intId)) { // L'evento in ingresso svuoterà questo link
                        ok = false;
                        break;
                    }
                }
                if (ok) { // La transizione è abilitata: esecuzione
                    int nuoviStatiAttivi[ncomp], nuovoStatoLink[nlink];
                    memcpy(nuovoStatoLink, attuale->linkContent, nlink*sizeof(int));
                    memcpy(nuoviStatiAttivi, attuale->componentStatus, ncomp*sizeof(int));
                    if (t->idIncomingEvent != VUOTO) // Consumo link in ingresso
                        nuovoStatoLink[t->linkIn->intId] = VUOTO;
                    nuoviStatiAttivi[c->intId] = t->to; // Nuovo stato attivo
                    for (k=0; k<t->nOutgoingEvents; k++) // Riempimento link in uscita
                        nuovoStatoLink[t->linkOut[k]->intId] = t->idOutgoingEvents[k];
                    BehState * nuovoStato = generateBehState(nuovoStatoLink, nuoviStatiAttivi);
                    nuovoStato->obsIndex = attuale->obsIndex;
                    if (loss>0) {
                        if (attuale->obsIndex<loss && t->obs > 0 && t->obs == obs[attuale->obsIndex])
                            nuovoStato->obsIndex = attuale->obsIndex+1;
                        nuovoStato->flags &= !FLAG_FINAL | (nuovoStato->obsIndex == loss);
                    }
                    // Ora bisogna inserire il nuovo stato e la nuova transizione nello spazio
                    bool avanzamento = enlargeBehavioralSpace(b, attuale, nuovoStato, t, loss);
                    //Se non è stato aggiunto un nuovo stato allora stiamo come prima: taglio
                    if (avanzamento) generateBehavioralSpace(b, nuovoStato, obs, loss);
                }
            }
        }
    }
}

void pruneRec(BehState *RESTRICT s) { // Invocata esternamente solo a partire dagli stati finali
    ok[s->id] = true;
    foreachdecl(trans, s->transitions) {
        if (trans->t->to == s && !ok[trans->t->from->id])
            pruneRec(trans->t->from);
    }
}

void prune(BehSpace *RESTRICT b) {
    BehState *s;
    int i;
    ok = calloc(b->nStates, sizeof(bool));
    ok[0] = true;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (s->flags & FLAG_FINAL) pruneRec(s);
    }
    
    for (i=0; i<b->nStates; i++) {
        if (!ok[i]) {
            removeBehState(b, b->states[i]);
            memcpy(ok+i, ok+i+1, (b->nStates-i)*sizeof(bool));
            i--;
        }
    }
    free(ok);
}

void faultSpaceExtend(BehState *RESTRICT base, int *obsStates, BehTrans **obsTrs) {
    ok[base->id] = true;
    int i=0;
    foreachdecl(lt, base->transitions) {
        if (lt->t->from == base && lt->t->t->obs != 0) {
            while (obsStates[i] != -1) i++;
            obsStates[i] = base->id;
            obsTrs[i] = lt->t;
        }
    }
    foreach(lt, base->transitions)
        if (lt->t->t->obs == 0 && !ok[lt->t->to->id])
            faultSpaceExtend(lt->t->to, obsStates+i, obsTrs);
}

FaultSpace * faultSpace(BehSpace *RESTRICT b, BehState *RESTRICT base, BehTrans **obsTrs) {
    int k, index=0;
    FaultSpace * ret = calloc(1, sizeof(FaultSpace));
    ret->idMapFromOrigin = calloc(b->nStates, sizeof(int));
    ret->exitStates = malloc(b->nTrans*sizeof(int));
    memset(ret->exitStates, -1, b->nTrans*sizeof(int));

    ok = calloc(b->nStates, sizeof(bool));
    faultSpaceExtend(base, ret->exitStates, obsTrs); // state ids and transitions refer to the original space, not a dup copy
    ret->b = dup(b, ok, true, &ret->idMapFromOrigin);
    free(ok);

    BehState *r, *temp;
    for (r=ret->b->states[k=0]; k<ret->b->nStates; r=ret->b->states[++k]) { // make base its inital state
        if (//k!=0 //&& base->obsIndex == r->obsIndex
        memcmp(base->linkContent, r->linkContent, nlink*sizeof(int)) == 0
        && memcmp(base->componentStatus, r->componentStatus, ncomp*sizeof(int)) == 0) {
            temp = ret->b->states[0];           // swap ptrs
            ret->b->states[0] = r;
            ret->b->states[k] = temp;
            temp->id = k;                       // update ids
            r->id = 0;
            int tempId, swap1, swap2;           // swap id map
            for (index=0; index<b->nStates; index++) {
                if (ret->idMapFromOrigin[index]==0) swap1=index;
                if (ret->idMapFromOrigin[index]==k) swap2=index;
            }
            tempId = ret->idMapFromOrigin[swap1];
            ret->idMapFromOrigin[swap1] = ret->idMapFromOrigin[swap2];
            ret->idMapFromOrigin[swap2] = tempId;
            break;
        }
    }
    for (k=0; k<b->nTrans; k++) {
        if (ret->exitStates[k] == -1) break;
        ret->exitStates[k] = ret->idMapFromOrigin[ret->exitStates[k]];
    }
    ret->exitStates = realloc(ret->exitStates, k*sizeof(int));
    ret->idMapToOrigin = calloc(ret->b->nStates, sizeof(int));
    for (k=0; k<b->nStates; k++)
        if (ret->idMapFromOrigin[k] != -1) ret->idMapToOrigin[ret->idMapFromOrigin[k]] = k;
    return ret;
}

/* Call like:
    int nSpaces=0;
    BehTrans *** obsTrs;
    FaultSpace ** ret = faultSpaces(b, &nSpaces, &obsTrs);*/
FaultSpace ** faultSpaces(BehSpace *RESTRICT b, int *RESTRICT nSpaces, BehTrans ****RESTRICT obsTrs) {
    BehState * s;
    int i, j=0;
    *nSpaces = 1;
    for (s=b->states[i=1]; i<b->nStates; s=b->states[++i]) {
        foreachdecl(lt, s->transitions) {
            if (lt->t->to == s && lt->t->t->obs != 0) {
                (*nSpaces)++;
                break;
            }
        }
    }
    FaultSpace ** ret = malloc(*nSpaces*sizeof(BehSpace *));
    *obsTrs = (BehTrans***)calloc(*nSpaces, sizeof(BehTrans**));
    for (i=0; i<*nSpaces; i++) {
        (*obsTrs)[i] = calloc(b->nTrans, sizeof(BehTrans**));
    }
    ret[0] = faultSpace(b, b->states[0], (*obsTrs)[0]);
    for (s=b->states[i=1]; i<b->nStates; s=b->states[++i]) {
        foreachdecl(lt, s->transitions) {
            if (lt->t->to == s && lt->t->t->obs != 0) {
                j++;
                ret[j] = faultSpace(b, s, (*obsTrs)[j]);
                break;
            }
        }
    }
    return ret;
}