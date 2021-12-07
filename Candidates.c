#include "header.h"

Trans *stubTrans;

void bt(BehSpace *b){
    BehState *s;
    unsigned int bucketId;
    foreachstb(b)
        s = sl->s;
        if (!s->transitions && b->nStates>1) printf(MSG_MEMTEST17, s->id);
        foreachdecl(lt, s->transitions) {
            if (lt->t->to->id != (int)s->id && lt->t->from->id != (int)s->id)
                printf(MSG_MEMTEST3, s->id, lt->t->from->id, lt->t->to->id);
            if (lt->t->to != s && lt->t->from != s) {
                printf(MSG_MEMTEST4, s->id, s);
                printf(MSG_MEMTEST5, lt->t->from, lt->t->from->id, lt->t->to, lt->t->to->id);
                printf(MSG_MEMTEST6, memcmp(lt->t->to, s, sizeof(BehState))==0);
                printf(MSG_MEMTEST7, memcmp(lt->t->from, s, sizeof(BehState))==0);
            }
        }
    foreachstc
}

INLINEO3(bool exitCondition(BehSpace *RESTRICT b, char mode)) {
    if (mode) {
        if (b->nStates>2) return false;
        unsigned int bucketId;
        foreachst(b,
            if (sl->s->id == 0)
                foreachdecl(lt, sl->s->transitions) {
                    debugif(DEBUG_DIAG, printlog("from %d to %d mark %d\n",lt->t->from->id, lt->t->to->id, lt->t->marker))
                    foreachdecl(lt2, lt->next)
                        if (lt->t->marker != 0 && lt->t->marker == lt2->t->marker)
                            return false;
            }
        )
        return true;
    }
    else return b->nTrans<=1;
}

INLINEO3(bool collapseSeries(BehSpace * b, unsigned int nMarker, char mode)) {
    bool ret = false;
    unsigned int bucketId;
    foreachstb(b)
        BehState * stemp = sl->s;
        BehTrans *tentra, *tesce;
        if (stemp->transitions != NULL && stemp->transitions->next != NULL && stemp->transitions->next->next == NULL &&
        (((tesce = stemp->transitions->t)->from == stemp && (tentra = stemp->transitions->next->t)->to == stemp)
        || ((tentra = stemp->transitions->t)->to == stemp && (tesce = stemp->transitions->next->t)->from == stemp))
        && tesce->from != tesce->to && tentra->to != tentra->from)  {             // stemp is the mid-sequence state
            BehTrans *nt = calloc(1, sizeof(BehTrans));
            nt->from = tentra->from;
            nt->to = tesce->to;
            nt->t = stubTrans;
            nt->regex = tentra->regex;  // To save CPU I'm making the new regex right into tentra's
            regexMake(tentra->regex, tesce->regex, tentra->regex, 'c', NULL);
            if (mode && nt->to->id == (int)nMarker-1) {
                if (tesce->marker != 0) nt->marker = tesce->marker;
                else nt->marker = stemp->id;
            }

            b->nTrans++;
            struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans));
            nuovaTr->t = nt;
            struct ltrans *templtrans = tentra->from->transitions;
            tentra->from->transitions = nuovaTr;
            tentra->from->transitions->next = templtrans;
            if (tentra->from != tesce->to) {
                templtrans = tesce->to->transitions;
                nuovaTr = calloc(1, sizeof(struct ltrans));
                nuovaTr->t = nt;
                tesce->to->transitions = nuovaTr;
                tesce->to->transitions->next = templtrans;
            }
            freeRegex(tesce->regex);
            removeBehState(b, stemp, false);
            ret = true;
            sl = b->sMap[bucketId];
            continue;
        }
    foreachstc
    return ret;
}

INLINEO3(bool collapseParallels(BehSpace * b, char mode)) {
    unsigned int bucketId;
    bool ret = false;
    foreachstb(b)                              // Collasso gruppi di transitions che condividono partenze e arrivi in una
        BehState* stemp = sl->s;
        foreachdecl(t1, stemp->transitions) {
            struct ltrans *t2 = t1->next, *beforeT2 = t1;
            while (t2) {
                if (t1->t->from == t2->t->from && t1->t->to == t2->t->to
                && (!mode || (mode && t2->t->marker == t1->t->marker))) {
                    debugif(DEBUG_DIAG, if (mode) printlog("Removed t with mark %d\n", t2->t->marker))
                    if (t1->t->regex->strlen >= t2->t->regex->strlen) {
                        regexMake(t1->t->regex, t2->t->regex, t1->t->regex, 'a', NULL);
                        freeRegex(t2->t->regex);
                    }
                    else {
                        regexMake(t2->t->regex, t1->t->regex, t2->t->regex, 'a', NULL);
                        freeRegex(t1->t->regex);
                        t1->t->regex = t2->t->regex;
                    }

                    if (t1->t->from != t1->t->to) {
                        BehState * state2 = t2->t->from == stemp ? t2->t->to : t2->t->from;
                        struct ltrans *s2t = state2->transitions, *beforeS2t = NULL;
                        while (s2t) {
                            if (s2t->t == t2->t) {
                                if (beforeS2t == NULL) state2->transitions = s2t->next;
                                else beforeS2t->next = s2t->next;
                                free(s2t);
                                break;
                            }
                            beforeS2t = s2t;
                            s2t = s2t->next;
                        }
                    }
                    free(t2->t);
                    b->nTrans--;
                    ret = true;
                    beforeT2->next = t2->next;
                    free(t2);
                    t2 = beforeT2->next;
                    continue;
                }
                beforeT2 = t2;
                t2 = t2->next;
            }
        }
    foreachstc
    return ret;
}

INLINEO3(bool closeLoop(BehSpace * b, unsigned int nMarker, char mode)) {
    unsigned int bucketId;
    bool ret = false;
    foreachstb(b)
        BehState * stemp = sl->s;
        BehTrans * autoTransizione = NULL;
        if (stemp->id != 0 && (unsigned) stemp->id+1 != nMarker) {
            foreachdecl(trans, stemp->transitions)
                if (trans->t->to == trans->t->from) {
                    autoTransizione = trans->t;
                    break;
                }
        }
        foreachdecl(trans1, stemp->transitions) {                               // Transizione entrante
            if (trans1->t->to == stemp && trans1->t->from != stemp) {           // In trans1 c'è una entrante nel nodo      
                foreachdecl(trans2, stemp->transitions) {                       // Transizione uscente
                    if (trans2->t->from == stemp && trans2->t->to != stemp) {   // In trans2 c'è una uscente dal nodo
                        ret = true;                  
                        BehTrans *nt = calloc(1, sizeof(BehTrans));
                        nt->from = trans1->t->from;
                        nt->to = trans2->t->to;
                        nt->t = stubTrans;
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

                        if (mode && nt->to->id == (int)nMarker-1) {
                            if (trans2->t->marker != 0) nt->marker = trans2->t->marker;
                            else nt->marker = stemp->id;
                        }
                    }
                }
            }
        }
        if (ret) {
            foreachdecl(trans, stemp->transitions)
                freeRegex(trans->t->regex);
            removeBehState(b, stemp, false);
            return ret;
        }
    foreachstc
    return ret;
}

Regex** diagnostics(BehSpace *RESTRICT b, char mode) {
    if (mode != 2 && !b->containsFinalStates) return NULL;
    unsigned int nMarker = b->nStates+2, bucketId;
    Regex ** ret = calloc(mode? nMarker-2 : 1, sizeof(Regex*));
    BehState * stemp = NULL;
    
    stubTrans = calloc(1, sizeof(Trans));
    foreachst(b, sl->s->id++)                                            // New initial state α
    stemp = calloc(1, sizeof(BehState));
    insertState(b, stemp, false);
    stemp->id = 0;
    BehTrans * nuovaTr = calloc(1, sizeof(BehTrans));
    nuovaTr->from = stemp;
    BehState * exInitial=NULL;
    foreachst(b, if (sl->s->id == 1) exInitial = sl->s;)
    nuovaTr->to = exInitial;
    nuovaTr->t = stubTrans;
    nuovaTr->regex = emptyRegex(0);
    stemp->transitions = calloc(1, sizeof(struct ltrans));
    stemp->transitions->t = nuovaTr;
    struct ltrans *vecchiaTesta = exInitial->transitions;
    exInitial->transitions = calloc(1, sizeof(struct ltrans));
    exInitial->transitions->t = nuovaTr;
    exInitial->transitions->next = vecchiaTesta;
    b->nTrans++;

    BehState * fine = calloc(1, sizeof(BehState));                              // New final state ω
    fine->id = b->nStates;
    fine->flags = FLAG_FINAL;
    foreachst(b,                                                  // Transitions from ex final states/all states to ω
        if (sl->s->id != 0 && (mode==2 || (sl->s->flags & FLAG_FINAL))) {
            BehTrans * trFinale = calloc(1, sizeof(BehTrans));
            trFinale->from = sl->s;
            trFinale->to = fine;
            trFinale->t = stubTrans;
            trFinale->regex = emptyRegex(0);
            struct ltrans *vecchiaTesta = sl->s->transitions;
            sl->s->transitions = calloc(1, sizeof(struct ltrans));
            sl->s->transitions->t = trFinale;
            sl->s->transitions->next = vecchiaTesta;
            vecchiaTesta = fine->transitions;
            fine->transitions = calloc(1, sizeof(struct ltrans));
            fine->transitions->t = trFinale;
            fine->transitions->next = vecchiaTesta;
            b->nTrans++;
            sl->s->flags &= !FLAG_FINAL;
        }
    )
    insertState(b, fine, false);

    foreachst(b,                                                        // Singleton regex making
        foreachdecl(trans, sl->s->transitions) {
            if (trans->t->from == sl->s) {
                trans->t->regex = emptyRegex(0);
                if (trans->t->t->fault >0) {
                    regexCompile(trans->t->regex, trans->t->t->fault);
                    trans->t->regex->concrete = true;
                }
            }
        }
    )
    
    debugif(DEBUG_DIAG, printlog("Entering reduction cycle\n"));
    while (!exitCondition(b, mode)) {
        debugif(DEBUG_DIAG, printlog("NS: %d NT: %d\n", b->nStates, b->nTrans));
        if (!collapseSeries(b, nMarker, mode))
            if (!collapseParallels(b, mode))
                closeLoop(b, nMarker, mode);
    }
    foreachst(b,
            if (sl->s->id == 0) {exInitial = sl->s; goto breakfor;})
    breakfor:
    if (mode) {
        foreachdecl(lt, exInitial->transitions) {
            if (lt->t->marker != 0) ret[lt->t->marker-1] = lt->t->regex;
        }
    }
    else ret[0] = exInitial->transitions->t->regex;
    return ret;
}