#include "header.h"

inline char* stateName(BehState *, bool) __attribute__((always_inline));
inline char* stateName(BehState *s, bool showObs) {
    unsigned int j, v;
    char* nome = malloc(30), *puntatore = nome;
    sprintf(puntatore++, "R");
    for (j=0; j<ncomp; j++) {
        v = s->componentStatus[j];
        sprintf(puntatore,"%d", v);
        puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
    }
    sprintf(puntatore,"_L");
    puntatore+=2;
    for (j=0; j<nlink; j++)
        if (s->linkContent[j] == VUOTO)
            sprintf(puntatore++,"e");
        else {
            v = s->linkContent[j];
            sprintf(puntatore, "%d", v);
            puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
        }
    if (showObs) {
        sprintf(puntatore,"_O");
        puntatore+=2;
        v = s->obsIndex;
        sprintf(puntatore, "%d", v);
        puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
    }
    return nome;
}

int thJob(void * command) {
    system(command);
    free(command);
    return 0;
}

void launchDot(char * command) {
    doC11(
        thrd_t thr;
        thrd_create(&thr, thJob, command);
        thrd_detach(thr);
    )
    doC99(thJob(command);)
}

void printDES(BehState * attuale, bool testuale) {
    unsigned short i, j, k;
    Link *l;
    Component *c;
    FILE * file;
    
    char nomeFileDot[strlenInputDES+7], nomeFileSvg[strlenInputDES+7];
    if (testuale) printf(MSG_ACTUAL_STRUCTURES);
    else {
        sprintf(nomeFileDot, "%s.dot", inputDES);
        file = fopen(nomeFileDot, "w");
        fprintf(file, "digraph \"%s\" {\nnode [style=filled fillcolor=white] compound=true\n", inputDES);
    }
    for (c=components[i=0]; i<ncomp; c=(i<ncomp ? components[++i] : c)){
        int nT = c->nTrans;
        if (testuale) printf(MSG_COMP_DESCRIPTION, c->id, c->nStates, attuale->componentStatus[c->intId], nT);
        else fprintf(file, "subgraph cluster%d {\nstyle=\"rounded,filled\" color=\"#FFEEEE\"node [shape=doublecircle]; C%d_%d;\nnode [shape=circle];\n", c->id, c->id, attuale->componentStatus[c->intId]);
        Trans *t;
        if (nT == 0) continue;
        for (t=c->transitions[j=0]; j<nT; t=(j<nT ? c->transitions[++j] : t)) {
            if (testuale) printf(MSG_TRANS_DESCRIPTION, t->id, t->from, t->to, t->obs, t->fault);
            else {
                fprintf(file, "C%d_%d -> C%d_%d [label=\"t%d", c->id, t->from, c->id, t->to, t->id);
                if (t->obs>0) fprintf(file, "O%hu", t->obs);
                if (t->fault>0 && t->fault<=25) fprintf(file, "%c", 96 + t->fault);
                else if (t->fault>25) fprintf(file, "R%hu", t->fault);
                fprintf(file, "\"];\n");
            }
            if (testuale & (t->idIncomingEvent == VUOTO)) printf(MSG_TRANS_DESCRIPTION2);
            else if (testuale) printf(MSG_TRANS_DESCRIPTION3, t->idIncomingEvent, t->linkIn->id);
            Link *uscita;
            int nUscite = t->nOutgoingEvents;
            if (nUscite == 0) {
                if (testuale) printf(MSG_TRANS_DESCRIPTION4);
                continue;
            } else
                if (testuale) printf(MSG_TRANS_DESCRIPTION5);
            for (uscita=t->linkOut[k=0]; k<nUscite; uscita=(k<nUscite ? t->linkOut[++k] : uscita))
                if (testuale) printf(MSG_TRANS_DESCRIPTION6, t->idOutgoingEvents[k], uscita->id);
        }
        if (!testuale) fprintf(file, "}\n");
    }
    for (l=links[i=0]; i<nlink; l=(i<nlink ? links[++i] : l))
        if (testuale)
            if (attuale->linkContent[l->intId] == VUOTO) printf(MSG_LINK_DESCRIPTION1, l->id, l->from->id, l->to->id);
            else printf(MSG_LINK_DESCRIPTION2, l->id, attuale->linkContent[l->intId], l->from->id, l->to->id);
        else {
            fprintf(file, "C%d_0 -> C%d_0 [", l->from->id, l->to->id);
            if (l->from != l->to) fprintf(file, "ltail=cluster%d lhead=cluster%d ", l->from->id, l->to->id);
            if (attuale->linkContent[l->intId] == VUOTO) fprintf(file, "label=\"L%d{}\"];\n", l->id);
            else fprintf(file, "label=\"L%d{e%d}\"];\n", l->id, attuale->linkContent[l->intId]);
        }
    if (testuale)
        printf(MSG_END_STATE_DESC, (attuale->flags & FLAG_FINAL) == FLAG_FINAL ? MSG_YES : MSG_NO);
    else {
        fprintf(file, "}\n");
        fclose(file);
        sprintf(nomeFileSvg, "%s.svg", inputDES);
        char * command = malloc(30+strlenInputDES*2);
        sprintf(command, "dot -Tsvg -o %s %s", nomeFileSvg, nomeFileDot);
        launchDot(command);
    }
}

char* printBehSpace(BehSpace *b, bool rename, bool showObs, int toString) {
    unsigned int i, position=0;
    char* nomeSpazi[b->nStates], nomeFileDot[strlenInputDES+8], nomeFileSvg[strlenInputDES+8], *ret;
    FILE * file;
    if (!toString) {
        sprintf(nomeFileDot, "%s_SC.dot", inputDES);
        file = fopen(nomeFileDot, "w");
        fprintf(file ,"digraph \"SC%s\" {\n", inputDES);
    }
    else ret = malloc((b->nStates+b->nTrans)*100);  // ARBITRARY LIMIT
    for (i=0; i<b->nStates; i++) {
        nomeSpazi[i] = stateName(b->states[i], showObs);
        if (toString) {
            if (rename) sprintf(ret+position, "C%dS%d ;\n", toString, i);
            else sprintf(ret+position, "%s ;\n", nomeSpazi[i]);
            position += strlen(ret+position);
        } else {
            if (b->states[i]->flags & FLAG_FINAL) fprintf(file, "node [style=filled fillcolor=\"#FFEEEE\"]; ");
            else fprintf(file, "node [fillcolor=\"#FFFFFF\"]; ");
            if (rename) fprintf(file, "S%d ;\n", i);
            else fprintf(file, "%s ;\n", nomeSpazi[i]);
        }
    }
    BehState *s;
    for (s=b->states[i=0]; i<b->nStates; s=b->states[++i]) {
        foreachdecl(trans, s->transitions) {
            if (trans->t->from == s) {
                if (toString) {
                    if (rename) sprintf(ret+position, "C%dS%d -> C%dS%d [label=t%d", toString, i, toString, trans->t->to->id, trans->t->t->id);
                    else sprintf(ret+position, "%s -> %s [label=t%d", nomeSpazi[i], nomeSpazi[trans->t->to->id], trans->t->t->id);
                    position += strlen(ret+position);
                    if (trans->t->t->obs>0) sprintf(ret+position, "O%hu", trans->t->t->obs);
                    position += strlen(ret+position);
                    if (trans->t->t->fault>0 && trans->t->t->fault<=25) sprintf(ret+position, "%c", 96 + trans->t->t->fault);
                    else if (trans->t->t->fault>25) sprintf(ret+position, "R%hu", trans->t->t->fault);
                    position += strlen(ret+position);
                    sprintf(ret+position, "]\n");
                    position += strlen(ret+position);
                } else {
                    if (rename) fprintf(file, "S%d -> S%d [label=t%d", i, trans->t->to->id, trans->t->t->id);
                    else fprintf(file, "%s -> %s [label=t%d", nomeSpazi[i], nomeSpazi[trans->t->to->id], trans->t->t->id);
                    if (trans->t->t->obs>0) fprintf(file, "O%hu", trans->t->t->obs);
                    if (trans->t->t->fault>0 && trans->t->t->fault<=25) fprintf(file, "%c", 96 + trans->t->t->fault);
                    else if (trans->t->t->fault>25) fprintf(file, "R%hu", trans->t->t->fault);
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
        sprintf(nomeFileSvg, "%s_SC.svg", inputDES);
        char * command = malloc(35+strlenInputDES*2);
        sprintf(command, "dot -Tsvg -o \"%s\" \"%s\"", nomeFileSvg, nomeFileDot);
        launchDot(command);
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
    unsigned int i, j;
    char nomeFileDot[strlenInputDES+9], nomeFileSvg[strlenInputDES+9];
    char * command = malloc(44+strlenInputDES*2);
    if (exp->maps) sprintf(nomeFileDot, "%s_EXP.dot", inputDES);
    else sprintf(nomeFileDot, "%s_PEX.dot", inputDES);
    if (exp->maps) sprintf(nomeFileSvg, "%s_EXP.svg", inputDES);
    else sprintf(nomeFileSvg, "%s_PEX.svg", inputDES);
    sprintf(command, "dot -Tsvg -o \"%s\" \"%s\"", nomeFileSvg, nomeFileDot);
    FILE * file = fopen(nomeFileDot, "w");
    fprintf(file ,"digraph \"EXP%s\" {\nnode [style=filled fillcolor=white]\n", inputDES);
    FaultSpace * fault;
    for (fault=exp->faults[i=0]; i<exp->nFaultSpaces; fault=exp->faults[++i]) {
        fprintf(file, "subgraph cluster%d {\nstyle=\"rounded,filled\" label=\"C%d\" fontcolor=\"#B2CCBB\" color=\"#EAFFEE\"\nedge[color=darkgray fontcolor=darkgray]\n", i, i);
        //char * descBehSpace = stampaSpazioComportamentale(exp->faults[i]->b, true, i+1);
        //fprintf(file, "%s", descBehSpace);
        for (j=0; j<fault->b->nStates; j++) {
            char flags = fault->b->states[j]->flags;
            if (exp->maps) {
                if ((flags & (FLAG_SILENT_FINAL | FLAG_FINAL)) == FLAG_SILENT_FINAL) fprintf(file, "node [shape=octagon]; ");
                else if ((flags & FLAG_FINAL) == FLAG_FINAL) fprintf(file, "node [shape=doubleoctagon]; ");
                else fprintf(file, "node [shape=oval]; ");
            }
            else {
                if ((flags & (FLAG_SILENT_FINAL | FLAG_FINAL)) == FLAG_SILENT_FINAL) fprintf(file, "node [shape=octagon width=0.3 height=0.3]; ");
                else if ((flags & FLAG_FINAL) == FLAG_FINAL) fprintf(file, "node [shape=doubleoctagon width=0.3 height=0.3]; ");
                else fprintf(file, "node [shape=circle width=0.3 height=0.3]; ");
            }
            if (exp->maps) fprintf(file, "C%dS%d [label=%d];\n", i, exp->maps[i]->idMapToOrigin[j], exp->maps[i]->idMapToOrigin[j]);
            else fprintf(file, "C%dS%d [label=\"\"];\n", i, j);
        }
        BehState *s;
        for (s=fault->b->states[j=0]; j<fault->b->nStates; s=fault->b->states[++j]) {
            foreachdecl(trans, s->transitions) {
                if (trans->t->from == s) {
                    if (exp->maps) fprintf(file, "C%dS%d -> C%dS%d [label=t%d", i, exp->maps[i]->idMapToOrigin[j], i, exp->maps[i]->idMapToOrigin[trans->t->to->id], trans->t->t->id);
                    else fprintf(file, "C%dS%d -> C%dS%d [label=t%d", i, j, i, trans->t->to->id, trans->t->t->id);
                    if (trans->t->t->obs>0) fprintf(file, "O%hu", trans->t->t->obs);
                    if (trans->t->t->fault>0 && trans->t->t->fault<=25) fprintf(file, "%c", 96 + trans->t->t->fault);
                    else if (trans->t->t->fault>25) fprintf(file, "R%hu", trans->t->t->fault);
                    fprintf(file, "]\n");
                }
            }
        }
        fprintf(file, "}\n");
    }
    
    ExplTrans *t;
    for (t=exp->trans[i=0]; i<exp->nTrans; t=exp->trans[++i]) {
        int fromId, toId;
        for (j=0; j<exp->nFaultSpaces; j++) {
            if (t->from == exp->faults[j]) fromId=j;
            if (t->to == exp->faults[j]) toId=j;
        }
        if (exp->maps) fprintf(file, "C%dS%d -> C%dS%d [style=dashed arrowhead=vee label=\"O%d",
            fromId, exp->maps[fromId]->idMapToOrigin[t->fromStateId], toId, t->toStateId, t->obs);
        else fprintf(file, "C%dS%d -> C%dS%d [style=dashed arrowhead=vee label=\"O%hu",
            fromId, t->fromStateId, toId, t->toStateId, t->obs);
        if (t->fault>0 && t->fault<=25) fprintf(file, "%c", 96 + t->fault);
        else if (t->fault>25) fprintf(file, "R%hu", t->fault);
        if (t->regex->regex[0] == '\0') fprintf(file, " %lc\"]\n", eps);
        else fprintf(file, " %s\"]\n", t->regex->regex);
    }
    fprintf(file, "}\n");
    fclose(file);
    launchDot(command);
}

unsigned int findInExplainer(Explainer *exp, FaultSpace *f) {
    unsigned int i=0;
    for (; i<exp->nFaultSpaces; i++)
        if (exp->faults[i] == f) break;
    return i;
}

void printMonitoring(Monitoring * m, Explainer *exp) {
    unsigned int i, j, temp;
    char nomeFileDot[strlenInputDES+9], nomeFileSvg[strlenInputDES+9];
    char * command = malloc(44+strlenInputDES*2);
    sprintf(nomeFileDot, "%s_MON.dot", inputDES);
    sprintf(nomeFileSvg, "%s_MON.svg", inputDES);
    sprintf(command, "dot -Tsvg -o \"%s\" \"%s\"", nomeFileSvg, nomeFileDot);
    FILE * file = fopen(nomeFileDot, "w");
    fprintf(file ,"digraph \"MON%s\" {\nrankdir=LR\nnode [style=filled fillcolor=white]\n", inputDES);
    MonitorState * mu;
    for (mu=m->mu[i=0]; i<m->length; mu=m->mu[++i]) {
        fprintf(file, "subgraph cluster%d {\nstyle=\"rounded,filled\" color=\"#FFF9DD\" node [style=\"rounded,filled\" shape=box fillcolor=\"#FFFFFF\"]\n", i);
        if (mu->lmu->regex[0] == '\0') fprintf(file, "label=ε\n");
        else fprintf(file, "label=\"%s\"\n", mu->lmu->regex);
        for (j=0; j<mu->nExpStates; j++) {
            temp = findInExplainer(exp, mu->expStates[j]);
            fprintf(file, "M%dS%d [label=%d];\n", i, temp, temp);
        }
        fprintf(file, "}\n");
    }
    for (mu=m->mu[i=0]; i+1<m->length; mu=m->mu[++i]) {
        MonitorTrans *arc;
        if (mu->nArcs>0) {
            for (arc=mu->arcs[j=0]; j<mu->nArcs; arc=mu->arcs[++j]) {
                char *l = arc->l->regex, *lp = arc->lp->regex;
                if (l[0] == '\0') l = "ε";
                if (lp[0] == '\0') lp = "ε";
                fprintf(file, "M%dS%d -> M%dS%d [label=\"(%s, %s)\"]\n", i, findInExplainer(exp, arc->from), i+1, findInExplainer(exp, arc->to), l, lp);
            }
        }
    }
    fprintf(file, "}\n");
    fclose(file);
    launchDot(command);
}