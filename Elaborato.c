#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define VUOTO -1
#define ACAPO -10
#define REGEX 10

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
    int da, a, dimRegex;
    struct transizione * t;
    char *regex;
    bool concreta, parentesizzata;
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
int sizeofSR=5, sizeofTR=20, sizeofCOMP=2, sizeofLINK=2;                    // Dimensioni di partenza

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
                    match(':', file);                               // idEvento:idLink
                    fscanf(file, "%d", &temp);
                    nuovo->linkIn = linkDaId(temp);
                } else {
                    nuovo->idEventoIn = VUOTO;
                    nuovo->linkIn = NULL;
                    ungetc(c, file);
                }
                match('|', file);                                   // Altrimenti se c'è | allora l'evento in ingresso è nullo
                
                while (!feof(file) && (c=fgetc(file)) != '\n') {    // Creazione lista di eventi in uscita
                    if (feof(file)) break;
                    ungetc(c, file);
                    
                    fscanf(file, "%d", &temp);
                    if (nuovo->nEventiU +1 > nuovo->sizeofEvU) {    // Allocazione bufferizzata della memoria anche in questo caso
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

StatoRete * generaStato(int *contenutoLink, int *statiAttivi){
    StatoRete *s = calloc(1, sizeof(StatoRete));
    s->id = VUOTO;
    if (contenutoLink != NULL) {
        s->contenutoLink = malloc(nlink*sizeof(int));
        copiaArray(contenutoLink, (*s).contenutoLink, nlink);
    } else s->contenutoLink = NULL;
    if (statiAttivi != NULL) {
        s->statoComponenti = malloc(ncomp*sizeof(int));
        copiaArray(statiAttivi, (*s).statoComponenti, ncomp);
    } else s->statoComponenti = NULL;
    s->indiceOsservazione = 0;
    s->finale = true;
    if (contenutoLink != NULL) {
        int i;
        for (i=0; i<nlink; i++)
            s->finale &= (s->contenutoLink[i] == VUOTO);
    }
    return s;
}

char* nomeStato(int n){
    int j, v;
    char* nome = calloc(30, sizeof(char)), *puntatore = nome;
    sprintf(puntatore++, "R");
    for (j=0; j<ncomp; j++) {
        v = statiSpazio[n]->statoComponenti[j];
        sprintf(puntatore,"%d", v);
        puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
    }
    sprintf(puntatore,"_L");
    puntatore+=2;
    for (j=0; j<nlink; j++)
        if (statiSpazio[n]->contenutoLink[j] == VUOTO)
            sprintf(puntatore++,"e");
        else {
            v = statiSpazio[n]->contenutoLink[j];
            sprintf(puntatore, "%d", v);
            puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
        }
    if (loss>0) {
        sprintf(puntatore,"_O");
        puntatore+=2;
        v = statiSpazio[n]->indiceOsservazione;
        sprintf(puntatore, "%d", v);
        puntatore += v == 0 ? 1 : (int)ceilf(log10f(v+1));
    }
    return nome;
}

void parseDot(FILE * file, bool semplificata) {
    int i;
    char buffer[50];
    fgets(buffer, 49, file);   // Intestazione
    if (!semplificata) {
        char nomeStatiTrovati[1000][50];        // Attenzione, limite arbitrario
        while (true) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "node") != 0) break;
            fscanf(file, "%s", buffer);
            bool dbc = strcmp(buffer, "[shape=doublecircle];")==0;
            fscanf(file, "%s", buffer);
            strcpy(nomeStatiTrovati[nStatiS], buffer);
            int strl = strlen(buffer), attivi[ncomp], clink[nlink], oss=-1, sez=0, j=1;
            for (i=1; i<strl; i++) {
                if (buffer[i] == '_') {
                    sez++;
                    j=i+2;
                    continue;
                }
                if (buffer[i] == 'L') continue;
                if (buffer[i] == 'O') continue;
                if (sez==0) attivi[i-j] = buffer[i] - '0';
                if (sez==1) clink[i-j] = buffer[i] == 'e' ? VUOTO : buffer[i] - '0';
                if (sez==2) {
                    oss = buffer[i] - '0';
                    loss = (loss>oss? loss : oss);
                }
            }
            StatoRete * nuovo = generaStato(clink, attivi);
            nuovo->id = nStatiS;
            nuovo->finale = dbc;
            nuovo->indiceOsservazione = oss;
            alloc1('s');
            statiSpazio[nStatiS++] = nuovo;
            fscanf(file, "%s", buffer); // Simbolo ;
        }
        while (true) {
            int idDa, idA, j;
            for (i=0; i<nStatiS; i++)
                if (strcmp(nomeStatiTrovati[i], buffer) == 0) {
                    idDa = i; 
                    break;
                }
            fscanf(file, "%s", buffer); // Simbolo ->
            fscanf(file, "%s", buffer);
            for (i=0; i<nStatiS; i++)
                if (strcmp(nomeStatiTrovati[i], buffer) == 0) {
                    idA = i; 
                    break;
                }
            
            fscanf(file, "%s", buffer); // Label
            int idTransizione = atoi(buffer+8);
            Transizione *t = NULL;
            for (i=0; i<ncomp; i++) {
                for (j=0; j<componenti[i]->nTransizioni; j++) {
                    if (componenti[i]->transizioni[j]->id == idTransizione) {
                        t = componenti[i]->transizioni[j];
                        break;
                    }
                }
                if (t != NULL) break;
            }

            alloc1('t');
            TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
            nuovaTransRete->a = idA;
            nuovaTransRete->da = idDa;
            nuovaTransRete->t = t;
            nuovaTransRete->regex = NULL;
            nuovaTransRete->dimRegex = REGEX;
            transizioniSpazio[nTransSp++] = nuovaTransRete;

            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "}") == 0) break;
        }
    } else {
        while (true) {
            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "node") != 0) break;
            fscanf(file, "%s", buffer);
            bool dbc = strcmp(buffer, "[shape=doublecircle];")==0;
            fscanf(file, "%s", buffer); // Trattandosi di una sequenza S0 S1 ... Sn il contenuto è prevedibile
            
            StatoRete * nuovo = generaStato(NULL, NULL);
            nuovo->id = nStatiS;
            nuovo->finale = dbc;
            alloc1('s');
            statiSpazio[nStatiS++] = nuovo;
            fscanf(file, "%s", buffer); // Simbolo ;
        }
        while (true) {
            int idDa = atoi(buffer+1), j;
            fscanf(file, "%s", buffer); // Simbolo ->
            fscanf(file, "%s", buffer);
            int idA = atoi(buffer+1);
            fscanf(file, "%s", buffer); // Label
            int idTransizione = atoi(buffer+8);
            Transizione *t = NULL;
            for (i=0; i<ncomp; i++) {
                for (j=0; j<componenti[i]->nTransizioni; j++) {
                    if (componenti[i]->transizioni[j]->id == idTransizione) {
                        t = componenti[i]->transizioni[j];
                        break;
                    }
                }
                if (t != NULL) break;
            }
            if (t==NULL) printf("Errore: transizione %d non trovata\n", idTransizione);
            
            alloc1('t');
            TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
            nuovaTransRete->regex = NULL;
            nuovaTransRete->dimRegex = REGEX;
            nuovaTransRete->a = idA;
            nuovaTransRete->da = idDa;
            nuovaTransRete->t = t;
            transizioniSpazio[nTransSp++] = nuovaTransRete;

            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "}") == 0) break;
        }
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

bool ampliaSpazioComportamentale(StatoRete * precedente, StatoRete * nuovo, Transizione * mezzo){
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
                if ((trans->da == precedente->id) && (trans->a == s->id) && (trans->t->id == mezzo->id)) 
                    return false;
            }
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        alloc1('s');
        nuovo->id = nStatiS;
        statiSpazio[nStatiS++] = nuovo;
    }
    if (mezzo != NULL){
        alloc1('t');
        TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
        nuovaTransRete->a = nuovo->id;
        nuovaTransRete->da = precedente->id;
        nuovaTransRete->t = mezzo;
        nuovaTransRete->regex = NULL;
        nuovaTransRete->dimRegex = REGEX;
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
            if ((attuale->statoComponenti[c->intId] == t->da) &&                          // Transizione abilitata se stato attuale = da
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
                    copiaArray(attuale->statoComponenti, nuoviStatiAttivi, ncomp);
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

void stampaSpazioComportamentale(bool rinomina) {
    int i, j;
    char* nomeSpazi[nStatiS], nomeFileDot[strlen(nomeFile)+7], nomeFilePDF[strlen(nomeFile)+7], comando[30+strlen(nomeFile)*2];
    sprintf(nomeFileDot, "SC%s.dot", nomeFile);
    FILE * file = fopen(nomeFileDot, "w");
    fprintf(file ,"digraph SC%s {\n", nomeFile);
    for (i=0; i<nStatiS; i++) {
        if (statiSpazio[i]->finale) fprintf(file, "node [shape=doublecircle]; ");
        else fprintf(file, "node [shape=circle]; ");
        nomeSpazi[i] = nomeStato(i);
        if (rinomina) fprintf(file, "S%d ;\n", i);
        else fprintf(file, "%s ;\n", nomeSpazi[i]);
    }
    for (i=0; i<nTransSp; i++) {
        if (rinomina) fprintf(file, "S%d -> S%d [label=t%d", transizioniSpazio[i]->da, transizioniSpazio[i]->a, transizioniSpazio[i]->t->id);
        else fprintf(file, "%s -> %s [label=t%d", nomeSpazi[transizioniSpazio[i]->da], nomeSpazi[transizioniSpazio[i]->a], transizioniSpazio[i]->t->id);
        if (transizioniSpazio[i]->t->oss>0) fprintf(file, "O%d", transizioniSpazio[i]->t->oss);
        if (transizioniSpazio[i]->t->ril>0) fprintf(file, "R%d", transizioniSpazio[i]->t->ril);
        fprintf(file, "]\n");
    }
    fprintf(file, "}");
    fclose(file);
    sprintf(nomeFilePDF, "SC%s.pdf", nomeFile);
    sprintf(comando, "dot -Tpdf -o %s %s", nomeFilePDF, nomeFileDot);
    system(comando);
    if (rinomina) {
        printf("Elenco sostituzioni nomi degli stati:\n");
        for (i=0; i<nStatiS; i++)
            printf("%d: %s\n", i, nomeSpazi[i]);
    }
    // for (i=0; i<nStatiS; i++)
    //     free(nomeSpazi[i]); Errore sia win che linux
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
    free(transizioniSpazio[j]);     // Free di Regex per qualche ragione non va
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
    if (statiSpazio[i]->contenutoLink != NULL) free(statiSpazio[i]->contenutoLink);
    if (statiSpazio[i]->statoComponenti != NULL) free(statiSpazio[i]->statoComponenti);
    free(statiSpazio[i]);
    copiaArraySR(statiSpazio+i+1, statiSpazio+i, (--nStatiS)-i, false);  // Elimino dal contenitore globale
    for (j=0; j<nStatiS; j++) {                                         //  Abbasso l'id degli stati successivi
        if (statiSpazio[j]->id>=i) statiSpazio[j]->id--;
    }
}

void potatura(void) {
    StatoRete *s;
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
    free(ok);
}

void diagnostica(void) {
    int i=0, j=0, k=0, h=0;
    char *temp = malloc(REGEX*50*nTransSp); // Sfondare il buffer implica probabile crash del programma. N transizioni -> buffer di N/2 KB
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
        transizioniSpazio[i]->regex = calloc(REGEX, 1);
        transizioniSpazio[i]->dimRegex = REGEX;
        transizioniSpazio[i]->parentesizzata = true;
        if (transizioniSpazio[i]->t->ril >0) sprintf(transizioniSpazio[i]->regex, "r%d", transizioniSpazio[i]->t->ril);
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
                int strl1 =strlen(transizioniSpazio[idTin[i]]->regex), strl2 = strlen(transizioniSpazio[idTda[i]]->regex);
                if (transizioniSpazio[idTin[i]]->dimRegex < strl1 + strl2 + 1) {
                    char * nuovaReg = realloc(transizioniSpazio[idTin[i]]->regex, (strl1+strl2+1)*2);
                    transizioniSpazio[idTin[i]]->dimRegex = (strl1+strl2+1)*2;
                    transizioniSpazio[idTin[i]]->regex = nuovaReg;
                }
                strcat(transizioniSpazio[idTin[i]]->regex, transizioniSpazio[idTda[i]]->regex);
                free(transizioniSpazio[idTda[i]]->regex);
                transizioniSpazio[idTin[i]]->concreta &= transizioniSpazio[idTda[i]]->concreta;
                if (strl1>0 & strl2>0) transizioniSpazio[idTin[i]]->parentesizzata = false;
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
                    int strl1 = strlen(t1->regex), strl2 = strlen(t2->regex);
                    if (strl1 > 0 && strl1==strl2 && strcmp(t1->regex, t2->regex)==0);
                    else if (strl1 > 0 && strl2 > 0) {
                        if (t1->dimRegex < strl1 + strl2 + 4) {
                            char * nuovaReg = realloc(t1->regex, (strl1+strl2)*2+4);
                            t1->dimRegex = (strl1+strl2)*2+4;
                            t1->regex = nuovaReg;
                        }
                        sprintf(temp, "(%s|%s)", t1->regex, t2->regex);
                        strcpy(t1->regex, temp);
                        t1->concreta &= t2->concreta;
                        t1->parentesizzata = true;
                    } else if (strl1 > 0 && strl2==0) {
                        int deltaCaratteri;
                        deltaCaratteri = t1->parentesizzata? 1 : 3;
                        if (t1->dimRegex < strl1 + deltaCaratteri) {
                            char * nuovaReg = realloc(t1->regex, strl1*2+deltaCaratteri);
                            t1->dimRegex = strl1*2+deltaCaratteri;
                            t1->regex = nuovaReg;
                            t1->parentesizzata = true;
                        }
                        if (t1->parentesizzata) sprintf(temp, "%s?", t1->regex);
                        else sprintf(temp, "(%s)?", t1->regex);
                        strcpy(t1->regex, temp);
                        t1->concreta = false;
                        t1->parentesizzata = true;  // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
                    } else if (strl2 > 0) {
                        int deltaCaratteri;
                        deltaCaratteri = t2->parentesizzata? 1 : 3;
                        if (t1->dimRegex < strl2 + deltaCaratteri) {
                            char * nuovaReg = realloc(t1->regex, (strl2)*2+deltaCaratteri);
                            t1->dimRegex = strl2*2+deltaCaratteri;
                            t1->regex = nuovaReg;
                        }
                        if (t2->parentesizzata) sprintf(t1->regex, "%s?", t2->regex);
                        else sprintf(t1->regex, "(%s)?", t2->regex);
                        t1->concreta = false;
                        t1->parentesizzata = true;  // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
                    }
                    free(t2->regex);
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
                                    int strl1 = strlen(t1->regex), strl2 = strlen(t2->regex), strl3 = strlen(transizioniSpazio[h]->regex),
                                    dimNt = strl1+strl2+strl3+20 < REGEX ? REGEX : strl1+strl2+strl3+20;
                                    TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                                    nt->da = t1->da;
                                    nt->a = t2->a;
                                    nt->t = tvuota;
                                    nt->regex = calloc(dimNt, 1);
                                    nt->dimRegex = dimNt;
                                    if (strl3 == 0 ) {
                                        //if (t1->regex[0] == '(' && t2->regex[strlen(t2->regex)-1]== ')') sprintf(nt->regex, "%s%s", t1->regex, t2->regex);
                                        if (strl1>0 & strl2>0) {
                                            sprintf(nt->regex, "(%s%s)", t1->regex, t2->regex);
                                            nt->parentesizzata = true;
                                        }
                                        else if (strl1 == 0 & strl2>0) {
                                            strcpy(nt->regex, t2->regex);
                                            nt->parentesizzata = t2->parentesizzata;
                                        }
                                        else if (strl2 == 0 & strl1>0) {
                                            strcpy(nt->regex, t1->regex);
                                            nt->parentesizzata = t1->parentesizzata;
                                        }
                                    } else {
                                        //if (t1->regex[0] == '(' && t2->regex[strlen(t2->regex)-1]== ')') sprintf(nt->regex, "%s(%s)*%s", t1->regex, transizioniSpazio[h]->regex, t2->regex);
                                        if (strl1 + strl2 >0) {
                                            if (transizioniSpazio[h]->parentesizzata)  sprintf(nt->regex, "(%s%s*%s)", t1->regex, transizioniSpazio[h]->regex, t2->regex);
                                            else sprintf(nt->regex, "(%s(%s)*%s)", t1->regex, transizioniSpazio[h]->regex, t2->regex);
                                        }
                                        else if (transizioniSpazio[h]->parentesizzata) sprintf(nt->regex, "%s*", transizioniSpazio[h]->regex);
                                        else sprintf(nt->regex, "(%s)*", transizioniSpazio[h]->regex);
                                        nt->parentesizzata = true; // Se anche termina con *, non importa
                                    }
                                    alloc1('t');
                                    transizioniSpazio[nTransSp] = nt;
                                    nTransSp++;
                                }
                            }
                            if (!esisteAutoTransizioneN) {
                                azioneEffettuataSuQuestoStato = true;
                                TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                                nt->da = t1->da;
                                nt->a = t2->a;
                                nt->t = tvuota;
                                int strl1 = strlen(t1->regex), strl2 = strlen(t2->regex),
                                dimNt = strl1+strl2+20 < REGEX ? REGEX : strl1+strl2+20;
                                nt->regex = calloc(dimNt, 1);
                                nt->dimRegex = dimNt;
                                if (strl1>0 & strl2>0) {
                                    sprintf(nt->regex, "(%s%s)", t1->regex, t2->regex);
                                    nt->parentesizzata = true;
                                }
                                else if (strl1 == 0 & strl2>0) {
                                    strcpy(nt->regex, t2->regex);
                                    nt->parentesizzata = t2->parentesizzata;
                                } else if (strl2 == 0 & strl1>0) {
                                    strcpy(nt->regex, t1->regex);
                                    nt->parentesizzata = t1->parentesizzata;
                                }
                                alloc1('t');
                                transizioniSpazio[nTransSp] = nt;
                                nTransSp++;
                            }
                        }
                    }
                }
            }
            if (azioneEffettuataSuQuestoStato) {
                for (j=0; j<nTransSp; j++) {
                    if (transizioniSpazio[j]->da == i || transizioniSpazio[j]->a == i) free(transizioniSpazio[j]->regex);
                }
                eliminaStato(i);
                break;
            }
        }
    }
    //free(temp);  Errore su Windows
    printf("REGEX: %s\n", transizioniSpazio[0]->regex);
}

void impostaDatiOsservazione(void) {
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
}

int main(void) {
    char sceltaDot[100], sceltaOperazione[20], pota[20], nomeFileSC[100], sceltaDiag[20], sceltaRinomina[20];
    printf("Benvenuto!\nIndicare il file che contiene la definizione dell'automa: ");
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
    scanf("%19s", sceltaDot);
    stampaStruttureAttuali(iniziale, sceltaDot[0] != 's');
    printf("Generare spazio comportamentale (c), fornire un'osservazione lineare (o), o caricare uno spazio da file (f, se gli stati sono rinominati: g)? ");
    scanf("%19s", sceltaOperazione);
    if (sceltaOperazione[0]=='o') {
        impostaDatiOsservazione();
        iniziale->finale = false;
    } else if (sceltaOperazione[0]=='f' || sceltaOperazione[0]=='g') {
        printf("Indicare il file dot generato contenete lo spazio comportamentale: ");
        fflush(stdout);
        fflush(stdin);
        scanf("%99s", nomeFileSC);
        fflush(stdin);
        FILE* fileSC = fopen(nomeFileSC, "rb+");
        if (fileSC == NULL) {
            printf("File \"%s\" inesistente!\n", nomeFile);
            return -1;
        }
        parseDot(fileSC, sceltaOperazione[0]=='g');
        fclose(fileSC);
        if (loss==0 & sceltaOperazione[0]=='f') printf("Lo stato non corrisponde ad un'osservazione lineare, pertanto non si consiglia un suo utilizzo per diagnosi\n");
        if (sceltaOperazione[0]=='g') printf("Non e' possibile stabilire se lo spazio importato sia derivante da un'osservazione lineare: eseguire una diagnosi solo in caso affermativo\n");
    }
    if (sceltaOperazione[0] != 'f' && sceltaOperazione[0]!='g') {
        printf("Generazione spazio comportamentale...\n");
        ampliaSpazioComportamentale(NULL, iniziale, NULL);
        generaSpazioComportamentale(iniziale);
        printf("Effettuare potatura (s/n)? ");
        scanf("%19s", pota);
        if (pota[0]=='s' && nTransSp>0) potatura();
        printf("Generato lo spazio: conta %d stati e %d transizioni\n", nStatiS, nTransSp);
        if (sceltaDot[0]=='s') {
            printf("Rinominare gli spazi col loro id (s/n)? ");
            scanf("%19s", sceltaRinomina);
            stampaSpazioComportamentale(sceltaRinomina[0]=='s');
        }
    }
    if (sceltaOperazione[0]=='f' || sceltaOperazione[0]=='g' || (sceltaOperazione[0]=='o' & pota[0]=='s')) {
        printf("Eseguire una diagnosi su questa osservazione (s/n)? ");
        fflush(stdout);
        scanf("%19s", sceltaDiag);
        if (sceltaDiag[0]=='s') {
            printf("Eseguo diagnostica... \n");
            diagnostica();
        }
    }
	return(0);
}