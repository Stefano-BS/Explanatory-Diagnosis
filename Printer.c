#include "header.h"

char* nomeStato(StatoRete *s){
    int j, v;
    char* nome = calloc(30, sizeof(char)), *puntatore = nome;
    sprintf(puntatore++, "R");
    for (j=0; j<ncomp; j++) {
        v = s->statoComponenti[j];
        sprintf(puntatore,"%d", v);
        puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
    }
    sprintf(puntatore,"_L");
    puntatore+=2;
    for (j=0; j<nlink; j++)
        if (s->contenutoLink[j] == VUOTO)
            sprintf(puntatore++,"e");
        else {
            v = s->contenutoLink[j];
            sprintf(puntatore, "%d", v);
            puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
        }
    if (loss>0) {
        sprintf(puntatore,"_O");
        puntatore+=2;
        v = s->indiceOsservazione;
        sprintf(puntatore, "%d", v);
        puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
    }
    return nome;
}

void stampaStruttureAttuali(StatoRete * attuale, bool testuale) {
    int i, j, k, strlenNomeFile = strlen(nomeFile);
    Link *l;
    Componente *c;
    FILE * file;
    
    char nomeFileDot[strlenNomeFile+7], nomeFilePDF[strlenNomeFile+7], comando[30+strlenNomeFile*2];
    if (testuale) printf(MSG_ACTUAL_STRUCTURES);
    else {
        sprintf(nomeFileDot, "%s.dot", nomeFile);
        file = fopen(nomeFileDot, "w");
        fprintf(file, "digraph \"%s\" {\nsize=\"8,5\"\ncompound=true\n", nomeFile);
    }
    for (c=componenti[i=0]; i<ncomp; c=(i<ncomp ? componenti[++i] : c)){
        int nT = c->nTransizioni;
        if (testuale) printf(MSG_COMP_DESCRIPTION, c->id, c->nStati, attuale->statoComponenti[c->intId], nT);
        else fprintf(file, "subgraph cluster%d {node [shape=doublecircle]; C%d_%d;\nnode [shape=circle];\n", c->id, c->id, attuale->statoComponenti[c->intId]);
        Transizione *t;
        if (nT == 0) continue;
        for (t=c->transizioni[j=0]; j<nT; t=(j<nT ? c->transizioni[++j] : t)) {
            if (testuale) printf(MSG_TRANS_DESCRIPTION, t->id, t->da, t->a, t->oss, t->ril);
            else {
                fprintf(file, "C%d_%d -> C%d_%d [label=\"t%d", c->id, t->da, c->id, t->a, t->id);
                if (t->oss>0) fprintf(file, "O%d", t->oss);
                if (t->ril>0) fprintf(file, "R%d", t->ril);
                fprintf(file, "\"];\n");
            }
            if (testuale & (t->idEventoIn == VUOTO)) printf(MSG_TRANS_DESCRIPTION2);
            else if (testuale) printf(MSG_TRANS_DESCRIPTION3, t->idEventoIn, t->linkIn->id);
            Link *uscita;
            int nUscite = t->nEventiU;
            if (nUscite == 0) {
                if (testuale) printf(MSG_TRANS_DESCRIPTION4);
                continue;
            } else
                if (testuale) printf(MSG_TRANS_DESCRIPTION5);
            for (uscita=t->linkU[k=0]; k<nUscite; uscita=(k<nUscite ? t->linkU[++k] : uscita))
                if (testuale) printf(MSG_TRANS_DESCRIPTION6, t->idEventiU[k], uscita->id);
        }
        if (!testuale) fprintf(file, "}\n");
    }
    for (l=links[i=0]; i<nlink; l=(i<nlink ? links[++i] : l))
        if (testuale)
            if (attuale->contenutoLink[l->intId] == VUOTO) printf(MSG_LINK_DESCRIPTION1, l->id, l->da->id, l->a->id);
            else printf(MSG_LINK_DESCRIPTION2, l->id, attuale->contenutoLink[l->intId], l->da->id, l->a->id);
        else
            if (attuale->contenutoLink[l->intId] == VUOTO) fprintf(file, "C%d_0 -> C%d_0 [ltail=cluster%d lhead=cluster%d label=\"L%d{}\"];\n", l->da->id, l->a->id, l->da->id, l->a->id, l->id);
            else if (attuale->contenutoLink[l->intId] == VUOTO) fprintf(file, "C%d_0 -> C%d_0 [ltail=cluster%d lhead=cluster%d label=\"L%d{e%d}\"];\n", l->da->id, l->a->id, l->da->id, l->a->id, l->id, attuale->contenutoLink[l->intId]);
    if (testuale) printf(MSG_END_STATE_DESC, attuale->finale == true ? MSG_YES : MSG_NO);
    else {
        fprintf(file, "}\n");
        fclose(file);
        sprintf(nomeFilePDF, "%s.pdf", nomeFile);
        sprintf(comando, "dot -Tpdf -o %s %s", nomeFilePDF, nomeFileDot);
        system(comando);
    }
}

void stampaSpazioComportamentale(BehSpace *b, bool rinomina) {
    int i, strlenNomeFile = strlen(nomeFile);
    char* nomeSpazi[b->nStates], nomeFileDot[strlenNomeFile+7], nomeFilePDF[strlenNomeFile+7], comando[30+strlenNomeFile*2];
    sprintf(nomeFileDot, "%s_SC.dot", nomeFile);
    FILE * file = fopen(nomeFileDot, "w");
    fprintf(file ,"digraph \"SC%s\" {\n", nomeFile);
    for (i=0; i<b->nStates; i++) {
        if (b->states[i]->finale) fprintf(file, "node [shape=doublecircle]; ");
        else fprintf(file, "node [shape=circle]; ");
        nomeSpazi[i] = nomeStato(b->states[i]);
        if (rinomina) fprintf(file, "S%d ;\n", i);
        else fprintf(file, "%s ;\n", nomeSpazi[i]);
    }
    StatoRete *s;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        struct ltrans * trans = s->transizioni;
        while (trans != NULL) {
            if (trans->t->da == s) {
                if (rinomina) fprintf(file, "S%d -> S%d [label=t%d", i, trans->t->a->id, trans->t->t->id);
                else fprintf(file, "%s -> %s [label=t%d", nomeSpazi[i], nomeSpazi[trans->t->a->id], trans->t->t->id);
                if (trans->t->t->oss>0) fprintf(file, "O%d", trans->t->t->oss);
                if (trans->t->t->ril>0) fprintf(file, "R%d", trans->t->t->ril);
                fprintf(file, "]\n");
            }
            trans = trans->prossima;
        }
    }
    fprintf(file, "}");
    fclose(file);
    sprintf(nomeFilePDF, "%s_SC.pdf", nomeFile);
    sprintf(comando, "dot -Tpdf -o \"%s\" \"%s\"", nomeFilePDF, nomeFileDot);
    system(comando);
    if (rinomina) {
        printf(MSG_SOBSTITUTION_LIST);
        for (i=0; i<b->nStates; i++)
            printf("%d: %s\n", i, nomeSpazi[i]);
    }
    for (i=0; i<b->nStates; i++)
        free(nomeSpazi[i]);
}