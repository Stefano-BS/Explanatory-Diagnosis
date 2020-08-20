#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define VUOTO -1
#define ACAPO -10

char nomeFile[100];

// DEFINIZIONE STATICA DI UN MODELLO COMPORTAMENTALE
typedef struct link {
    struct componente *da, *a;
    int id, intId;
} Link;

typedef struct transizione {
    int oss, ril, da, a, id;
    int idEventoIn, *idEventiU, nEventiU, sizeofEvU;
    struct link *linkIn, **linkU;
} Transizione;

typedef struct componente {
    int nStati, id, intId, nTransizioni;
    struct transizione **transizioni;
} Componente;

Componente **componenti;
Link **links;
int nlink=0, ncomp=0;

// COMPORTAMENTO DINAMICO DELLA RETE
typedef struct statorete {
    int *statoComponenti, *contenutoLink;
    int id, indiceOsservazione;
    bool finale;
} StatoRete;

typedef struct {
    int da, a;
    struct transizione * t;
    char *regex;
    bool concreta;
} TransizioneRete;

StatoRete ** statiSpazio;
TransizioneRete ** transizioniSpazio;
int nStatiS=0, nTransSp=0;
int * ok, *osservazione, loss=0;

// UTILI
void copiaArray(int* da, int* in, int lunghezza){
	int i;
	for (i=0; i<lunghezza; i++)
		*in++ = *da++;
}

void copiaArraySR(StatoRete** da, StatoRete** in, int lunghezza, bool desc) {
	int i;
	if (desc) {
        StatoRete** s = &da[lunghezza-1], **d = &in[lunghezza-1];
        for (i=lunghezza; i>0; i--)
            *d-- = *s--;
    } else
        for (i=0; i<lunghezza; i++)
            *in++ = *da++;
}

void copiaArrayTR(TransizioneRete ** da, TransizioneRete ** in, int lunghezza){
	int i;
	for (i=0; i<lunghezza; i++)
		*in++ = *da++;
}

void stampaArray(int* a, int lunghezza){
	int i=0;
	int* arr = a;
	for (; i<lunghezza; i++)
		printf("%d ", *a++);
	printf("\n");
	a=arr;
}

Componente* compDaId(int id){
    int i=0;
    for (; i<ncomp; i++)
        if (componenti[i]->id == id) return componenti[i];
    printf("Errore: componente con id %d non trovato\n", id);
    return NULL;
}

Link* linkDaId(int id){
    int i=0;
    for (; i<nlink; i++)
        if (links[i]->id == id) return links[i];
    printf("Errore: link con id %d non trovato\n", id);
    return NULL;
}

// Rendo bufferizzata la gestione della memoria heap per (tutte) le struttre dati (strutture di puntatori e strutture a contatori)
int *sizeofTrComp;
int sizeofSR=5, sizeofTR=20, sizeofCOMP=2, sizeofLINK=2;

void allocamentoIniziale(void){
    componenti = malloc(sizeofCOMP* sizeof(Componente*));                   // Puntatori ai componenti
    sizeofTrComp = malloc(sizeofCOMP*sizeof(int));                          // Dimensioni degli array puntatori a transizioni nei componenti
    links = malloc(sizeofLINK* sizeof(Link*));                              // Puntatori ai link
    statiSpazio = malloc(sizeofSR* sizeof(StatoRete*));                     // Puntatori agli stati rete
    transizioniSpazio = malloc(sizeofTR* sizeof(TransizioneRete*));         // Puntatori alle transizioni rete
}

void alloc1(char o) {   // Gestione strutture globali (livello zero)
    if (o=='s') {
        if (nStatiS+1 > sizeofSR) {
            sizeofSR += 10;
            StatoRete ** spazio = realloc(statiSpazio, sizeofSR*sizeof(StatoRete*));
            if (spazio == NULL) printf("Errore di allocazione memoria!\n");
            else statiSpazio = spazio;
        }
    } else if (o=='t') {
        if (nTransSp+1 > sizeofTR) {
            sizeofTR += 50;
            TransizioneRete ** spazio = realloc(transizioniSpazio, sizeofTR*sizeof(TransizioneRete*));
            if (spazio == NULL) printf("Errore di allocazione memoria!\n");
            else transizioniSpazio = spazio;
        }
    } else if (o=='c') {
        if (ncomp+1 > sizeofCOMP) {
            sizeofCOMP += 10;
            Componente ** spazio = realloc(componenti, sizeofCOMP*sizeof(Componente*));
            int* sottoAlloc = realloc(sizeofTrComp, sizeofCOMP*sizeof(int));
            if (spazio == NULL | sottoAlloc == NULL) printf("Errore di allocazione memoria!\n");
            else {componenti = spazio; sizeofTrComp = sottoAlloc;}
        }
    } else if (o=='l') {
        if (nlink+1 > sizeofLINK) {
            sizeofLINK += 10;
            Link ** spazio = realloc(links, sizeofLINK*sizeof(Link*));
            if (spazio == NULL) printf("Errore di allocazione memoria!\n");
            else links = spazio;
        }
    }
}

Componente * nuovoComponente(void) {
    alloc1('c');
    Componente *nuovo = calloc(1, sizeof(Componente));
    nuovo->transizioni = calloc(5, sizeof(Transizione*));
    nuovo->intId = ncomp;
    sizeofTrComp[ncomp] = 5;
    componenti[ncomp++] = nuovo;
    return nuovo;
}

void alloc1trC(Componente * comp) {
    if (comp->nTransizioni+1 > sizeofTrComp[comp->intId]) {
        sizeofTrComp[comp->intId] += 10;
        Transizione ** spazio = realloc(comp->transizioni, sizeofTrComp[comp->intId]*sizeof(Transizione*));
        if (spazio == NULL) printf("Errore di allocazione memoria!\n");
        else comp->transizioni = spazio;
    }
}


// ANALISI LESSICALE FILE IN INGRESSO
int linea = 0;

void match(int c, FILE* input) {
    if (c==ACAPO) {
        char c1 = fgetc(input);
        //if (c1 == '\r') printf("trovato\\r "); else if (c1 == '\n') printf("trovato\\n "); else printf("trovato%c ", c1);
        if (c1 == '\n') return;
        else if (c1 == '\r') 
            if (fgetc(input) != '\n') printf("Linea %d - Atteso fine linea\n", linea);
    } else {
        char cl = fgetc(input);
        if (cl != c) printf("Linea %d - Carattere inaspettato: %c (atteso: %c)\n", linea, cl, c);
    }
}

void parse(FILE* file){
    // NOTA: prima definizione componenti
    // Segue la definizione dei link
    // Infine, la definizione delle transizioni
    char sezione[16], c;
    int i;
    while (fgets(sezione, 15, file) != NULL) {
        linea++;
        if (strcmp(sezione,"//COMPONENTI\n")==0 || strcmp(sezione,"//COMPONENTI\r\n")==0) {
            while (!feof(file)) { //Per ogni componente
                if  ((c=fgetc(file)) == '/') { //Vuol dire che siamo sforati nella sezione dopo
                    ungetc(c, file);
                    break;
                }
                ungetc(c, file); //Rimettiamo dentro il pezzo di intero
                
                linea++;
                Componente *nuovo = nuovoComponente();
                fscanf(file, "%d", &(nuovo->nStati));
                match(',', file);
                fscanf(file, "%d", &(nuovo->id));
                if (!feof(file)) match(ACAPO, file);
            }
        } else if (strcmp(sezione,"//TRANSIZIONI\n")==0 || strcmp(sezione,"//TRANSIZIONI\r\n")==0) {
            while (!feof(file)) {
                if  ((c=fgetc(file)) == '/') {
                    ungetc(c, file);
                    break;
                }
                ungetc(c, file);

                linea++;
                Transizione *nuovo = calloc(1, sizeof(Transizione));
                int temp, componentePadrone;
                nuovo->idEventiU = calloc(2, sizeof(int));
                nuovo->linkU = calloc(2, sizeof(Link*));
                nuovo->sizeofEvU = 2;
                fscanf(file, "%d", &(nuovo->id));
                match(',', file);
                fscanf(file, "%d", &componentePadrone);
                match(',', file);
                fscanf(file, "%d", &(nuovo->da));
                match(',', file);
                fscanf(file, "%d", &(nuovo->a));
                match(',', file);
                fscanf(file, "%d", &(nuovo->oss));
                match(',', file);
                fscanf(file, "%d", &(nuovo->ril));
                match(',', file);
                if (isdigit(c = fgetc(file))) {                     // Se dopo la virgola c'è un numero, allora c'è un evento in ingresso
                    ungetc(c, file);
                    fscanf(file, "%d", &(nuovo->idEventoIn));
                    match(':', file);                               //idEvento:idLink
                    fscanf(file, "%d", &temp);
                    nuovo->linkIn = linkDaId(temp);
                } else {
                    nuovo->idEventoIn = VUOTO;
                    nuovo->linkIn = NULL;
                    ungetc(c, file);
                }
                match('|', file);                                   // Altrimenti se c'è | allora l'evento in ingresso è nullo
                
                while (!feof(file) && (c=fgetc(file)) != '\n') {    //Creazione lista di eventi in uscita
                    if (feof(file)) break;
                    ungetc(c, file);
                    
                    fscanf(file, "%d", &temp);
                    if (nuovo->nEventiU +1 > nuovo->sizeofEvU) {
                        nuovo->sizeofEvU += 5;
                        nuovo->idEventiU = realloc(nuovo->idEventiU, nuovo->sizeofEvU*sizeof(int));
                        nuovo->idEventiU = realloc(nuovo->idEventiU, nuovo->sizeofEvU*sizeof(int));
                    }
                    nuovo->idEventiU[nuovo->nEventiU] = temp;

                    match(':', file); //idEvento:idLink
                    fscanf(file, "%d", &temp);
                    Link *linkNuovo = linkDaId(temp);
                    nuovo->linkU[nuovo->nEventiU] = linkNuovo;
                    nuovo->nEventiU++;
                    match(',', file);
                }
                if (!feof(file)) ungetc(c, file);
                
                Componente *padrone = compDaId(componentePadrone);  // Aggiunta della neonata transizione al relativo componente
                alloc1trC(padrone);
                padrone->transizioni[padrone->nTransizioni] = nuovo;
                padrone->nTransizioni++;

                if (!feof(file)) match(ACAPO, file);
            }
        } else if (strcmp(sezione,"//LINK\n")==0 || strcmp(sezione,"//LINK\r\n")==0) {
            while (!feof(file)) {
                if  ((c=fgetc(file)) == '/') {
                    ungetc(c, file);
                    break;
                }
                ungetc(c, file);
                
                linea++;
                Link *nuovo = calloc(1, sizeof(Link));
                int idComp1, idComp2;
                fscanf(file, "%d", &idComp1);
                match(',', file);
                fscanf(file, "%d", &idComp2);
                match(',', file);
                fscanf(file, "%d", &(nuovo->id));
                nuovo->intId = nlink;
                nuovo->da = compDaId(idComp1);
                nuovo->a = compDaId(idComp2);
                alloc1('l');
                links[nlink++] = nuovo;

                if (!feof(file)) match(ACAPO, file); 
            }
        } else if (!feof(file))
            printf("Riga non corretta: %s\n", sezione);
    }
}

void stampaStruttureAttuali(StatoRete * attuale, bool testuale){
    int i, j, k;
    Link *l;
    Componente *c;
    FILE * file;
    char nomeFileDot[strlen(nomeFile)+7], nomeFilePDF[strlen(nomeFile)+7], comando[30+strlen(nomeFile)*2];
    if (testuale) printf("\nStrutture dati attuali:\n");
    else {
        sprintf(nomeFileDot, "%s.dot", nomeFile);
        file = fopen(nomeFileDot, "w");
        fprintf(file, "digraph %s {\nsize=\"8,5\"\ncompound=true\n", nomeFile);
    }
    for (c=componenti[i=0]; i<ncomp; c=(i<ncomp ? componenti[++i] : c)){
        int nT = c->nTransizioni;
        if (testuale) printf("Componente id:%d, ha %d stati (attivo: %d) e %d transizioni:\n", c->id, c->nStati, attuale->statoComponenti[c->intId], nT);
        else fprintf(file, "subgraph cluster%d {node [shape=doublecircle]; C%d_%d;\nnode [shape=circle];\n", c->id, c->id, attuale->statoComponenti[c->intId]);
        Transizione *t;
        if (nT == 0) continue;
        for (t=c->transizioni[j=0]; j<nT; t=(j<nT ? c->transizioni[++j] : t)) {
            if (testuale) printf("\tTransizione id:%d va da %d a %d, osservabile: %d, rilevante: %d,", t->id, t->da, t->a, t->oss, t->ril);
            else {
                fprintf(file, "C%d_%d -> C%d_%d [label=\"t%d", c->id, t->da, c->id, t->a, t->id);
                if (t->oss>0) fprintf(file, "O%d", t->oss);
                if (t->ril>0) fprintf(file, "R%d", t->ril);
                fprintf(file, "\"];\n");
            }
            if (testuale & t->idEventoIn == VUOTO) printf(" nessun evento entrante,");
            else if (testuale) printf(" evento in ingresso: %d sul link %d,", t->idEventoIn, t->linkIn->id);
            Link *uscita;
            int nUscite = t->nEventiU;
            if (nUscite == 0) {
                if (testuale) printf(" nessun evento in uscita.\n");
                continue;
            } else
                if (testuale) printf(" eventi in uscita:\n");
            for (uscita=t->linkU[k=0]; k<nUscite; uscita=(k<nUscite ? t->linkU[++k] : uscita))
                if (testuale) printf("\t\tEvento %d sul link id:%d\n", t->idEventiU[k], uscita->id);
        }
        if (!testuale) fprintf(file, "}\n");
    }
    for (l=links[i=0]; i<nlink; l=(i<nlink ? links[++i] : l))
        if (testuale)
            if (attuale->contenutoLink[l->intId] == VUOTO) printf("Link id:%d, vuoto, collega %d a %d\n", l->id, l->da->id, l->a->id);
            else printf("Link id:%d, contiene evento %d, collega %d a %d\n", l->id, attuale->contenutoLink[l->intId], l->da->id, l->a->id);
        else
            if (attuale->contenutoLink[l->intId] == VUOTO) fprintf(file, "C%d_0 -> C%d_0 [ltail=cluster%d lhead=cluster%d label=\"L%d{}\"];\n", l->da->id, l->a->id, l->da->id, l->a->id, l->id);
            else if (attuale->contenutoLink[l->intId] == VUOTO) fprintf(file, "C%d_0 -> C%d_0 [ltail=cluster%d lhead=cluster%d label=\"L%d{e%d}\"];\n", l->da->id, l->a->id, l->da->id, l->a->id, l->id, attuale->contenutoLink[l->intId]);
    if (testuale) printf("Fine contenuto. Stato finale: %s\n", attuale->finale == true ? "si" : "no");
    else {
        fprintf(file, "}\n");
        fclose(file);
        sprintf(nomeFilePDF, "%s.pdf", nomeFile);
        sprintf(comando, "dot -Tpdf -o %s %s", nomeFilePDF, nomeFileDot);
        system(comando);
    }
}

StatoRete * generaStato(int *contenutoLink, int *statiAttivi){
    StatoRete *s = malloc(sizeof(StatoRete));
    s->id = VUOTO;
    s->statoComponenti = malloc(ncomp*sizeof(int));
    s->contenutoLink = malloc(nlink*sizeof(int));
    copiaArray(contenutoLink, (*s).contenutoLink, nlink);
    copiaArray(statiAttivi, (*s).statoComponenti, ncomp);
    int i;
    s->indiceOsservazione = 0;
    s->finale = true;
    for (i=0; i<nlink; i++)
       s->finale &= (s->contenutoLink[i] == VUOTO);
    return s;
}

char* nomeStato(int n){
    int j;
    char* nome = calloc(30, sizeof(char)), *puntatore = nome;
    sprintf(puntatore++, "R");
    for (j=0; j<ncomp; j++) {
        sprintf(puntatore,"%d", statiSpazio[n]->statoComponenti[j]);
        puntatore += strlen(puntatore);
    }
    sprintf(puntatore,"_L");
    puntatore+=2;
    for (j=0; j<nlink; j++)
        if (statiSpazio[n]->contenutoLink[j] == VUOTO)
            sprintf(puntatore++,"e");
        else {
            sprintf(puntatore, "%d", statiSpazio[n]->contenutoLink[j]);
            puntatore += strlen(puntatore);
        }
    if (loss>0) {
        sprintf(puntatore,"_O");
        puntatore+=2;
        sprintf(puntatore, "%d", statiSpazio[n]->indiceOsservazione);
        puntatore += strlen(puntatore);
    }
    return nome;
}

bool ampliaSpazioComportamentale(StatoRete * precedente, StatoRete * nuovo, Transizione * nuovaTrans){
    int i, j;
    bool giaPresente = false;
    StatoRete *s;
    if (nStatiS>0) {
        for (s=statiSpazio[i=0]; i<nStatiS; s=(i<nStatiS ? statiSpazio[++i] : s)) { // Per ogni stato dello spazio comportamentale
            bool stessiAttivi = true;
            for (j=0; j<ncomp; j++)                                                 // Uguaglianza liste stati attivi
                stessiAttivi &= (nuovo->statoComponenti[j] == s->statoComponenti[j]);
            if (stessiAttivi) {
                bool stessiEventi = true;
                for (j=0; j<nlink; j++)                                             // Uguaglianza stato link
                    stessiEventi &= (nuovo->contenutoLink[j] == s->contenutoLink[j]);
                if (stessiEventi) {
                    giaPresente = (loss>0 ? nuovo->indiceOsservazione == s->indiceOsservazione : true);
                    if (giaPresente) break;
                }
            }
        }
    }
    if (giaPresente) { // Già c'era lo stato, ma la transizione non è detto
        if (nTransSp>0) {
            nuovo->id = s->id;
            TransizioneRete * trans;
            for (trans=transizioniSpazio[i=0]; i<nTransSp; trans=(i<nTransSp ? transizioniSpazio[++i] : trans)) { // Controllare se la transizione già esisteva
                if ((trans->da == precedente->id) && (trans->a == s->id) && (trans->t->id == nuovaTrans->id)) 
                    return false;
            }
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        alloc1('s');
        nuovo->id = nStatiS;
        statiSpazio[nStatiS++] = nuovo;
    }
    if (nuovaTrans != NULL){
        alloc1('t');
        TransizioneRete * nuovaTransRete = malloc(sizeof(TransizioneRete));
        nuovaTransRete->a = nuovo->id;
        nuovaTransRete->da = precedente->id;
        nuovaTransRete->t = nuovaTrans;
        transizioniSpazio[nTransSp++] = nuovaTransRete;
    }
    return !giaPresente;
}

void generaSpazioComportamentale(StatoRete * attuale) {
    Componente *c;
    int i, j, k;
    for (c=componenti[i=0]; i<ncomp;  c=componenti[++i]){
        Transizione *t;
        int nT = c->nTransizioni;
        if (nT == 0) continue;
        for (t=c->transizioni[j=0]; j<nT; t=(j<nT ? c->transizioni[++j] : t)) {
            if (attuale->statoComponenti[c->intId] == t->da &&                          // Transizione abilitata se stato attuale = da
            (t->idEventoIn == VUOTO || t->idEventoIn == attuale->contenutoLink[t->linkIn->intId])) { // Ok eventi in ingresso
                if (loss>0 && ((attuale->indiceOsservazione>=loss && t->oss > 0) ||    // Osservate tutte, non se ne possono più vedere
                (attuale->indiceOsservazione<loss && t->oss > 0 && t->oss != osservazione[attuale->indiceOsservazione])))
                    continue; // Transizione non compatibile con l'osservazione lineare
                bool ok = true;
                for (k=0; k<t->nEventiU; k++) {                                        // I link di uscita sono vuoti
                    if (attuale->contenutoLink[t->linkU[k]->intId] != VUOTO) {
                        ok = false;
                        break;
                    }
                }
                if (ok) { // La transizione è abilitata: esecuzione
                    int nuoviStatiAttivi[ncomp], nuovoStatoLink[nlink];
                    copiaArray(attuale->contenutoLink, nuovoStatoLink, nlink);
                    copiaArray(attuale->statoComponenti, nuoviStatiAttivi, nlink);
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
                    bool avanzamento = ampliaSpazioComportamentale(attuale, nuovoStato, t);
                    //Se non è stato aggiunto un nuovo stato allora stiamo come prima: taglio
                    if (avanzamento) generaSpazioComportamentale(nuovoStato);
                }
            }
        }
    } //printf("Attenzione! Superato il tetto al numero di iterazioni per il calcolo dello spazio comportamentale. Terminazione.\n");
}

void stampaSpazioComportamentale(void) {
    int i, j;
    char* nomeSpazi[nStatiS], nomeFileDot[strlen(nomeFile)+7], nomeFilePDF[strlen(nomeFile)+7], comando[30+strlen(nomeFile)*2];
    sprintf(nomeFileDot, "SC%s.dot", nomeFile);
    FILE * file = fopen(nomeFileDot, "w");
    fprintf(file ,"digraph SC%s {\n", nomeFile);
    for (i=0; i<nStatiS; i++) {
        if (statiSpazio[i]->finale) fprintf(file, "node [shape=doublecircle]; ");
        else fprintf(file, "node [shape=circle]; ");
        fprintf(file, "%s ;\n", nomeSpazi[i] = nomeStato(i));
    }
    for (i=0; i<nTransSp; i++) {
        fprintf(file, "%s -> %s [label=t%d", nomeSpazi[transizioniSpazio[i]->da], nomeSpazi[transizioniSpazio[i]->a], transizioniSpazio[i]->t->id);
        if (transizioniSpazio[i]->t->oss>0) fprintf(file, "O%d", transizioniSpazio[i]->t->oss);
        if (transizioniSpazio[i]->t->ril>0) fprintf(file, "R%d", transizioniSpazio[i]->t->ril);
        fprintf(file, "]\n");
    }
    fprintf(file, "}");
    fclose(file);
    sprintf(nomeFilePDF, "SC%s.pdf", nomeFile);
    sprintf(comando, "dot -Tpdf -o %s %s", nomeFilePDF, nomeFileDot);
    system(comando);
}

void potsSC(StatoRete *s) { // Invocata esternamente solo a partire dagli stati finali
    TransizioneRete *tr;
    int j;
    ok[s->id] = 1;
    for (tr=transizioniSpazio[j=0]; j<nTransSp; tr=transizioniSpazio[++j]) {
        if (tr->a == s->id && !ok[tr->da]) // Transizioni che giungono in questo stato (+ prevenzione ciclicità)
            potsSC(statiSpazio[tr->da]);
    }
}

void eliminaTransizione(int j) {
    copiaArrayTR(transizioniSpazio+j+1, transizioniSpazio+j, (--nTransSp)-j);
}

void eliminaStato(int i) {
    int j;
    for (j=0; j<nTransSp; j++) {                                // Elimino le transizioni senza più lo stato
        if (transizioniSpazio[j]->da == i || transizioniSpazio[j]->a == i) {
            eliminaTransizione(j);
            j--;
        }
    }
    for (j=0; j<nTransSp; j++) {                                // Abbasso l'id degli stati successivi in tutte le transizioni
        if (transizioniSpazio[j]->da>i) transizioniSpazio[j]->da--;
        if (transizioniSpazio[j]->a>i) transizioniSpazio[j]->a--;
    }
    copiaArraySR(statiSpazio+i+1, statiSpazio+i, (--nStatiS)-i, false);  // Elimino dal contenitore globale
    for (j=0; j<nStatiS; j++) {                                         //  Abbasso l'id degli stati successivi
        if (statiSpazio[j]->id>=i) statiSpazio[j]->id--;
    }
}

void potatura(void) {
    StatoRete *s;
    TransizioneRete *tr;
    int i;
    ok = calloc(nStatiS, sizeof(int));
    ok[0] = 1;
    for (s=statiSpazio[i=0]; i<nStatiS; s=statiSpazio[++i]) {
        if (s->finale) potsSC(s);
    }
    
    for (i=0; i<nStatiS; i++) {
        if (!ok[i]) {
            eliminaStato(i);                                           // Elimino lo stato i
            copiaArray(ok+i+1, ok+i, nStatiS-i);                      //  Elimino dalla lista Ok
            i--;
        }
    }
}

void diagnostica(void) {
    int i=0, j=0, k=0, h=0;
    char *temp = malloc(1000);
    Transizione *tvuota = calloc(1, sizeof(Transizione));
    alloc1('s');                                                        // Generazione nuovo stato iniziale
    for (i=0; i<nStatiS; i++)
        statiSpazio[i]->id++;
    copiaArraySR(statiSpazio, statiSpazio+1, nStatiS, true);
    nStatiS++;
    for (i=0; i<nTransSp; i++) {
        transizioniSpazio[i]->da++;
        transizioniSpazio[i]->a++;
    }
    statiSpazio[0] = calloc(1, sizeof(StatoRete));
    statiSpazio[0]->id = 0;
    alloc1('t');
    transizioniSpazio[nTransSp] = calloc(1, sizeof(TransizioneRete));
    transizioniSpazio[nTransSp]->da = 0;
    transizioniSpazio[nTransSp]->a = 1;
    transizioniSpazio[nTransSp]->t = tvuota;
    nTransSp++;

    alloc1('s');
    StatoRete * fine = calloc(1, sizeof(StatoRete));                           // Generazione nuovo stato finale
    fine->id = nStatiS;
    fine->finale = true;
    for (i=1; i<nStatiS; i++) {                                                 // Collegamento ex stati finali col nuovo
        if (statiSpazio[i]->finale) {
            TransizioneRete * trFinale = calloc(1, sizeof(TransizioneRete));
            trFinale->da = statiSpazio[i]->id;
            trFinale->a = nStatiS;
            trFinale->t = tvuota;
            alloc1('t');
            transizioniSpazio[nTransSp] = trFinale;
            nTransSp++;
            statiSpazio[i]->finale = false;
        }
    }
    statiSpazio[nStatiS] = fine;
    nStatiS++;
    for (i=0; i<nTransSp; i++) {                                            // Generazione Regex iniziali (etichette ril)
        transizioniSpazio[i]->regex = calloc(1000, 1);
        if (transizioniSpazio[i]->t->ril >0)
            sprintf(transizioniSpazio[i]->regex, "r%d", transizioniSpazio[i]->t->ril);
    }
    
    int transizioniDa [nStatiS], idTda[nStatiS], idTin[nStatiS], transizioniIn [nStatiS];
    while (nTransSp>1) {
        bool continua = false;
        // Trovo una catena di stati
        for (i=0; i<nStatiS; i++)
            transizioniDa[i] = idTda[i] = transizioniIn[i] = idTin[i] = 0;
        for (i=0; i<nTransSp; i++) {                                       // Individuazione di stati "solo di passaggio"
            transizioniDa[transizioniSpazio[i]->da]++;  // n°t che partono da i
            idTda[transizioniSpazio[i]->da] = i;        // indice t che parte da i (se è solo una)
            transizioniIn[transizioniSpazio[i]->a]++;   // n°t che giungono in i
            idTin[transizioniSpazio[i]->a] = i;         // indice t che giunge in i (se è solo una)
        }
        for (i=0; i<nStatiS; i++) {                                         // Semplificazione serie -> unità
            if ((1 == transizioniDa[i]) && (1 == transizioniIn[i])) {       // Elemento interno alla sequenza
// printf("At%d aveva %s, t%d aveva %s, ", idTin[i], transizioniSpazio[idTin[i]]->regex, idTda[i], transizioniSpazio[idTda[i]]->regex);
                strcat(transizioniSpazio[idTin[i]]->regex, transizioniSpazio[idTda[i]]->regex);
// printf(" ora ha %s\n", transizioniSpazio[idTin[i]]->regex);
                transizioniSpazio[idTin[i]]->a = transizioniSpazio[idTda[i]]->a;
                eliminaStato(i);
                continua = true;
                break;
            }
        }
        if (continua) continue;
        TransizioneRete *t1, *t2;                                           // Collasso gruppi di transizioni che condividono partenze e arrivi in una
        for (t1=transizioniSpazio[i=0]; i<nTransSp; t1 = i<nTransSp ? transizioniSpazio[++i] : t1) {
            for (t2=transizioniSpazio[j=0]; j<nTransSp; t2 = j<nTransSp ? transizioniSpazio[++j] : t2) {
                if ((i != j) && (t1->da == t2->da) && (t1->a == t2->a)) {   // Nota: i diverso da j
// printf("Bt%d aveva %s, t%d aveva %s, ", i, t1->regex, j, t2->regex);
                    if (strlen(t1->regex) > 0 && strlen(t2->regex) > 0) {
                        sprintf(temp, "(%s|%s)", t1->regex, t2->regex);
                        strcpy(t1->regex, temp);
                        t1->concreta &= t2->concreta;
                    } else if (strlen(t1->regex) > 0 && strlen(t2->regex)==0) {
                        sprintf(temp, "(%s)?", t1->regex);
                        strcpy(t1->regex, temp);
                        t1->concreta = false;
                    } else if (strlen(t2->regex) > 0) {
                        sprintf(t1->regex, "(%s)?", t2->regex);
                        t1->concreta = false;
                    }
// printf(" ora ha %s\n", t1->regex);
                    eliminaTransizione(j);
                    continua = true;
                    if (j>=i) i--;
                    break;
                }
            }
        }
        if (continua) continue;
        bool azioneEffettuataSuQuestoStato = false;
        for (i=1; i<nStatiS-1; i++) {                           // Per ogni stato, ma l'azione viene eseguita solo al primo trovato
            for (t1=transizioniSpazio[j=0]; j<nTransSp; t1 = j<nTransSp ? transizioniSpazio[++j] : t1) { // Per ogni transizione
                if ((t1->a == i) && (t1->a != t1->da)) {                                                //  Che giunge nello stato
                    for (t2=transizioniSpazio[k=0]; k<nTransSp; t2 = k<nTransSp ? transizioniSpazio[++k] : t2) {    // Per ogni altra transizione
                        if ((t2->da == i) && (t2->a != t2->da)) {                                        // Che parte dallo stato
                            bool esisteAutoTransizioneN = false;
                            for (h=0; h<nTransSp; h++) {                                      // Per ogni transizione (controllo auto-trans.)
                                if ((transizioniSpazio[h]->a == transizioniSpazio[h]->da) && (transizioniSpazio[h]->da == i)) {
                                    azioneEffettuataSuQuestoStato = esisteAutoTransizioneN = true;
                                    TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                                    nt->regex = calloc(1000, 1);
                                    nt->da = t1->da;
                                    nt->a = t2->a;
                                    nt->t = tvuota;
// printf("Ct%d aveva %s, t%d aveva %s, t%d aveva %s, ", i, t1->regex, j, t1->regex, h, transizioniSpazio[h]->regex) ;
                                    if (strlen(transizioniSpazio[h]->regex) == 0 ) {
                                        if (t1->regex[0] == '(' && t2->regex[strlen(t2->regex)-1]== ')') sprintf(nt->regex, "%s%s", t1->regex, t2->regex);
                                        else if (strlen(t1->regex) + strlen(t2->regex) >0) sprintf(nt->regex, "(%s%s)", t1->regex, t2->regex);
                                    } else {
                                        if (t1->regex[0] == '(' && t2->regex[strlen(t2->regex)-1]== ')') sprintf(nt->regex, "%s(%s)*%s", t1->regex, transizioniSpazio[h]->regex, t2->regex);
                                        else if (strlen(t1->regex) + strlen(t2->regex) >0) sprintf(nt->regex, "(%s(%s)*%s)", t1->regex, transizioniSpazio[h]->regex, t2->regex);
                                        else sprintf(nt->regex, "(%s)*", transizioniSpazio[h]->regex);
                                    }
// printf(" ora ha %s\n", nt->regex);
                                    alloc1('t');
                                    transizioniSpazio[nTransSp] = nt;
                                    nTransSp++;
                                }
                            }
                            if (!esisteAutoTransizioneN) {
                                azioneEffettuataSuQuestoStato = true;
                                TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                                nt->regex = calloc(1000, 1);
                                nt->da = t1->da;
                                nt->a = t2->a;
                                nt->t = tvuota;
// printf("Dt%d aveva %s, t%d aveva %s, ", i, t1->regex, j, t2->regex);
                                if (strlen(t1->regex) + strlen(t2->regex) > 0) {
                                    if (t1->regex[0] == '(' && t2->regex[strlen(t2->regex)-1]== ')') sprintf(nt->regex, "%s%s", t1->regex, t2->regex);
                                    else sprintf(nt->regex, "(%s%s)", t1->regex, t2->regex);
                                }
// printf(" ora ha %s\n", nt->regex);
                                alloc1('t');
                                transizioniSpazio[nTransSp] = nt;
                                nTransSp++;
                            }
                        }
                    }
                }
            }
            if (azioneEffettuataSuQuestoStato) {
                eliminaStato(i);
                break;
            }
        }
    }
    printf("REGEX: %s\n", transizioniSpazio[0]->regex);
}

int main(void) {
    char sceltaDot[100], sceltaOperazione[100], pota[100];
    printf("Benvenuto!\nIndicare il file da analizzare: ");
    fflush(stdout);
	scanf("%99s", nomeFile);
	FILE* file = fopen(nomeFile, "rb+");
	if (file == NULL) {
		printf("File \"%s\" inesistente!\n", nomeFile);
		return -1;
	}
    allocamentoIniziale();
	parse(file);
	fclose(file);
	printf("Parsing effettuato...\n");
    int *statiAttivi = calloc(ncomp, sizeof(int));
    int statoLink[nlink], i;
    for (i=0; i<nlink; i++)
        statoLink[i] = VUOTO;
    StatoRete * iniziale = generaStato(statoLink, statiAttivi);
    iniziale->indiceOsservazione = 0;

    printf("Salvare i grafi come .dot (s/n)? ");
    scanf("%99s", sceltaDot);
    printf("Effettuare potatura (s/n)? ");
    scanf("%99s", pota);
    if (sceltaDot[0]=='s') stampaStruttureAttuali(iniziale, false);
    else stampaStruttureAttuali(iniziale, true);
    printf("Generare spazio comportamentale (c) o fornire un'osservazione lineare (o)? ");
    scanf("%99s", sceltaOperazione);
    if (sceltaOperazione[0]=='o') {
        int oss, sizeofBuf = 10;
        osservazione = calloc(10, sizeof(int));
        printf("Fornire la sequenza di etichette. Ogni numero, a capo, per terminare usa un carattere non numerico\n");
        while (true) {
            char digitazione[10];
            scanf("%9s", digitazione);
            oss = atoi(digitazione);
            if (oss <= 0) break;
            if (loss+1 > sizeofBuf) {
                sizeofBuf += 10;
                osservazione = realloc(osservazione, sizeofBuf*sizeof(int));
            }
            osservazione[loss++] = oss;
        }
        iniziale->finale = false;
    }
    printf("Generazione spazio comportamentale...\n");
    ampliaSpazioComportamentale(NULL, iniziale, NULL);
    generaSpazioComportamentale(iniziale);
    if (pota[0]=='s' && nTransSp>0) potatura();
    if (sceltaDot[0]=='s') stampaSpazioComportamentale();
    if (sceltaOperazione[0]=='o' & pota[0]=='s') {
        printf("Eseguo diagnostica... \n");
        diagnostica();
    }
	return(0);
}