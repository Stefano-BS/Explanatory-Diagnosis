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
        sprintf(nomeFilePDF, "%s.svg", nomeFile);
        sprintf(comando, "dot -Tsvg -o %s %s", nomeFilePDF, nomeFileDot);
        system(comando);
    }
}

char* stampaSpazioComportamentale(BehSpace *b, bool rename, int toString) {
    int i, strlenNomeFile = strlen(nomeFile), position=0;
    char* nomeSpazi[b->nStates], nomeFileDot[strlenNomeFile+8], nomeFilePDF[strlenNomeFile+8], comando[35+strlenNomeFile*2], *ret;
    FILE * file;
    if (!toString) {
        sprintf(nomeFileDot, "%s_SC.dot", nomeFile);
        file = fopen(nomeFileDot, "w");
        fprintf(file ,"digraph \"SC%s\" {\n", nomeFile);
    }
    else ret = malloc((b->nStates+b->nTrans)*100);  // ARBITRARY LIMIT
    for (i=0; i<b->nStates; i++) {
        nomeSpazi[i] = nomeStato(b->states[i]);
        if (toString) {
            if (rename) sprintf(ret+position, "C%dS%d ;\n", toString, i);
            else sprintf(ret+position, "%s ;\n", nomeSpazi[i]);
            position += strlen(ret+position);
        } else {
            if (b->states[i]->finale) fprintf(file, "node [shape=doublecircle]; ");
            else fprintf(file, "node [shape=circle]; ");
            if (rename) fprintf(file, "S%d ;\n", i);
            else fprintf(file, "%s ;\n", nomeSpazi[i]);
        }
    }
    StatoRete *s;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        foreachdecl(trans, s->transizioni) {
            if (trans->t->da == s) {
                if (toString) {
                    if (rename) sprintf(ret+position, "C%dS%d -> C%dS%d [label=t%d", toString, i, toString, trans->t->a->id, trans->t->t->id);
                    else sprintf(ret+position, "%s -> %s [label=t%d", nomeSpazi[i], nomeSpazi[trans->t->a->id], trans->t->t->id);
                    position += strlen(ret+position);
                    if (trans->t->t->oss>0) sprintf(ret+position, "O%d", trans->t->t->oss);
                    position += strlen(ret+position);
                    if (trans->t->t->ril>0) sprintf(ret+position, "R%d", trans->t->t->ril);
                    position += strlen(ret+position);
                    sprintf(ret+position, "]\n");
                    position += strlen(ret+position);
                } else {
                    if (rename) fprintf(file, "S%d -> S%d [label=t%d", i, trans->t->a->id, trans->t->t->id);
                    else fprintf(file, "%s -> %s [label=t%d", nomeSpazi[i], nomeSpazi[trans->t->a->id], trans->t->t->id);
                    if (trans->t->t->oss>0) fprintf(file, "O%d", trans->t->t->oss);
                    if (trans->t->t->ril>0) fprintf(file, "R%d", trans->t->t->ril);
                    fprintf(file, "]\n");
                }
            }
        }
    }
    if (toString)
        sprintf(ret+position, "}\n");
    else {
        fprintf(file, "}");
        fclose(file);
        sprintf(nomeFilePDF, "%s_SC.svg", nomeFile);
        sprintf(comando, "dot -Tsvg -o \"%s\" \"%s\"", nomeFilePDF, nomeFileDot);
        system(comando);
        if (rename) {
            printf(MSG_SOBSTITUTION_LIST);
            for (i=0; i<b->nStates; i++)
                printf("%d: %s\n", i, nomeSpazi[i]);
        }
    }
    for (i=0; i<b->nStates; i++)
        free(nomeSpazi[i]);
    return ret;
}

void printExplainer(Explainer * exp) {
    int i, j, strlenNomeFile = strlen(nomeFile);
    char nomeFileDot[strlenNomeFile+9], nomeFilePDF[strlenNomeFile+9], comando[44+strlenNomeFile*2];
    sprintf(nomeFileDot, "%s_EXP.dot", nomeFile);
    sprintf(nomeFilePDF, "%s_EXP.svg", nomeFile);
    sprintf(comando, "dot -Tsvg -o \"%s\" \"%s\"", nomeFilePDF, nomeFileDot);
    FILE * file = fopen(nomeFileDot, "w");
    fprintf(file ,"digraph \"EXP%s\" {\nnode [style=filled fillcolor=white]\n", nomeFile);
    FaultSpace * fault;
    for (fault=exp->faults[i=0]; i<exp->nFaultSpaces; fault=exp->faults[++i]) {
        fprintf(file, "subgraph cluster%d {\nstyle=\"rounded,filled\" color=\"#EAFFEE\"\nedge[color=darkgray fontcolor=darkgray]\n", i+1);
        //char * descBehSpace = stampaSpazioComportamentale(exp->faults[i]->b, true, i+1);
        //fprintf(file, "%s", descBehSpace);
        for (j=0; j<fault->b->nStates; j++) {
            fprintf(file, "C%dS%d ;\n", i+1, fault->idMapToOrigin[j]);
        }
        StatoRete *s;
        for (s=fault->b->states[j=0]; j<fault->b->nStates; s=fault->b->states[++j]) {
            foreachdecl(trans, s->transizioni) {
                if (trans->t->da == s) {
                    fprintf(file, "C%dS%d -> C%dS%d [label=t%d", i+1, fault->idMapToOrigin[j], i+1, fault->idMapToOrigin[trans->t->a->id], trans->t->t->id);
                    if (trans->t->t->oss>0) fprintf(file, "O%d", trans->t->t->oss);
                    if (trans->t->t->ril>0) fprintf(file, "R%d", trans->t->t->ril);
                    fprintf(file, "]\n");
                }
            }
        }
        fprintf(file, "}\n");
    }
    
    TransExpl *t;
    for (t=exp->trans[i=0]; i<exp->nTrans; t=exp->trans[++i]) {
        int fromId, toId;
        for (j=0; j<exp->nFaultSpaces; j++) {
            if (t->from == exp->faults[j]) fromId=j;
            if (t->to == exp->faults[j]) toId=j;
        }
        fprintf(file, "C%dS%d -> C%dS%d [style=dashed arrowhead=vee ltail=cluster%d lhead=cluster%d label=\"O%d",
            fromId+1, exp->faults[fromId]->idMapToOrigin[t->fromStateId], toId+1, t->toStateId, fromId+1, toId+1, t->obs);
        if (t->ril>0) fprintf(file, "R%d", t->ril);
        fprintf(file, " %s\"]\n", t->regex);
    }
    fprintf(file, "}\n");
    fclose(file);
    system(comando);
}