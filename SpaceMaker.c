#include "header.h"

int *ok;

bool ampliaSpazioComportamentale(BehSpace * b, StatoRete * precedente, StatoRete * nuovo, Transizione * mezzo) {
    int i;
    bool giaPresente = false;
    StatoRete *s;
    if (b->nStates>0) {
        for (s=b->states[i=0]; i<b->nStates; s=(i<b->nStates ? b->states[++i] : s)) { // Per ogni stato dello spazio comportamentale
            if (memcmp(nuovo->statoComponenti, s->statoComponenti, ncomp*sizeof(int)) == 0
            && memcmp(nuovo->contenutoLink, s->contenutoLink, nlink*sizeof(int)) == 0) {
                giaPresente = (loss>0 ? nuovo->indiceOsservazione == s->indiceOsservazione : true);
                if (giaPresente) {
                    freeStatoRete(nuovo); // Questa istruzione previene la duplicazione in memoria di stati con stessa semantica
                    nuovo = s;  // Non è solo una questione di prestazioni, ma di mantenimento relazione biunivoca ptr <-> id
                    break;
                }
            }
        }
    }
    if (giaPresente) { // Già c'era lo stato, ma la transizione non è detto
        struct ltrans * trans = precedente->transizioni;
        nuovo->id = s->id;
        while (trans != NULL) {
            if (trans->t->a == nuovo && trans->t->t->id == mezzo->id) return false;
            trans = trans->prossima;
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        alloc1(b, 's');
        nuovo->id = b->nStates;
        b->states[b->nStates++] = nuovo;
    }
    if (mezzo != NULL) { // Lo stato iniziale, ad esempio ha NULL
        TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
        nuovaTransRete->a = nuovo;
        nuovaTransRete->da = precedente;
        nuovaTransRete->t = mezzo;
        nuovaTransRete->regex = NULL;
        nuovaTransRete->dimRegex = 0;

        struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
        nuovaLTr->t = nuovaTransRete;
        nuovaLTr->prossima = nuovo->transizioni;//statiSpazio[nuovo->id]->transizioni;
        nuovo->transizioni = nuovaLTr; //statiSpazio[nuovo->id]->transizioni = nuovaLTr;
        
        if (nuovo != precedente) {
            nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->prossima = precedente->transizioni; //statiSpazio[precedente->id]->transizioni;
            precedente->transizioni = nuovaLTr; //statiSpazio[precedente->id]->transizioni = nuovaLTr;
        }

        b->nTrans++;
    }
    return !giaPresente;
}

void generaSpazioComportamentale(BehSpace * b, StatoRete * attuale) {
    Componente *c;
    int i, j, k;
    for (c=componenti[i=0]; i<ncomp;  c=componenti[++i]){
        Transizione *t;
        int nT = c->nTransizioni;
        if (nT == 0) continue;
        for (t=c->transizioni[j=0]; j<nT; t=(j<nT ? c->transizioni[++j] : t)) {
            if ((attuale->statoComponenti[c->intId] == t->da) &&                          // Transizione abilitata se stato attuale = da
            (t->idEventoIn == VUOTO || t->idEventoIn == attuale->contenutoLink[t->linkIn->intId])) { // Ok eventi in ingresso
                if (loss>0 && ((attuale->indiceOsservazione>=loss && t->oss > 0) ||    // Osservate tutte, non se ne possono più vedere
                (attuale->indiceOsservazione<loss && t->oss > 0 && t->oss != osservazione[attuale->indiceOsservazione])))
                    continue; // Transizione non compatibile con l'osservazione lineare
                bool ok = true;
                for (k=0; k<t->nEventiU; k++) {                                        // I link di uscita sono vuoti
                    if (attuale->contenutoLink[t->linkU[k]->intId] != VUOTO
                    && !(t->idEventoIn != VUOTO && t->linkIn->intId == t->linkU[k]->intId)) { // L'evento in ingresso svuoterà questo link
                        ok = false;
                        break;
                    }
                }
                if (ok) { // La transizione è abilitata: esecuzione
                    int nuoviStatiAttivi[ncomp], nuovoStatoLink[nlink];
                    memcpy(nuovoStatoLink, attuale->contenutoLink, nlink*sizeof(int));
                    memcpy(nuoviStatiAttivi, attuale->statoComponenti, ncomp*sizeof(int));
                    if (t->idEventoIn != VUOTO) // Consumo link in ingresso
                        nuovoStatoLink[t->linkIn->intId] = VUOTO;
                    nuoviStatiAttivi[c->intId] = t->a; // Nuovo stato attivo
                    for (k=0; k<t->nEventiU; k++) // Riempimento link in uscita
                        nuovoStatoLink[t->linkU[k]->intId] = t->idEventiU[k];
                    StatoRete * nuovoStato = generaStato(nuovoStatoLink, nuoviStatiAttivi);
                    nuovoStato->indiceOsservazione = attuale->indiceOsservazione;
                    if (loss>0) {
                        if (attuale->indiceOsservazione<loss && t->oss > 0 && t->oss == osservazione[attuale->indiceOsservazione])
                            nuovoStato->indiceOsservazione = attuale->indiceOsservazione+1;
                        nuovoStato->finale &= nuovoStato->indiceOsservazione == loss;
                    }
                    // Ora bisogna inserire il nuovo stato e la nuova transizione nello spazio
                    bool avanzamento = ampliaSpazioComportamentale(b, attuale, nuovoStato, t);
                    //Se non è stato aggiunto un nuovo stato allora stiamo come prima: taglio
                    if (avanzamento) generaSpazioComportamentale(b, nuovoStato);
                }
            }
        }
    }
}

void potsSC(BehSpace *b, StatoRete *s) { // Invocata esternamente solo a partire dagli stati finali
    ok[s->id] = 1;
    struct ltrans * trans = s->transizioni;
    for (; trans != NULL; trans=trans->prossima) {
        if (trans->t->a == s && !ok[trans->t->da->id])
            potsSC(b, b->states[trans->t->da->id]);
    }
}

void potatura(BehSpace *b) {
    StatoRete *s;
    int i;
    ok = calloc(b->nStates, sizeof(int));
    ok[0] = 1;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        if (s->finale) potsSC(b, s);
    }
    
    for (i=0; i<b->nStates; i++) {
        if (!ok[i]) {
            eliminaStato(b, i);
            memcpy(ok+i, ok+i+1, (b->nStates-i)*sizeof(int));
            i--;
        }
    }
    free(ok);
}