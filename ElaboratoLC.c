#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define LOGO "    ______                     __                     ___         __                  _\n   / ____/_______  _______  __/ /_____  ________     /   | __  __/ /_____  ____ ___  (_)\n  / __/ / ___/ _ \\/ ___/ / / / __/ __ \\/ ___/ _ \\   / /| |/ / / / __/ __ \\/ __ `__ \\/ /\n / /___(__  )  __/ /__/ /_/ / /_/ /_/ / /  /  __/  / ___ / /_/ / /_/ /_/ / / / / / / /\n/_____/____/\\___/\\___/\\__,_/\\__/\\____/_/   \\___/  /_/  |_\\__,_/\\__/\\____/_/ /_/ /_/_/\n"
                                                                                        
#define VUOTO -1
#define ACAPO -10
#define REGEX 10

#define ottieniComando(com) while (!isalpha(*com=getchar()));

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
    struct ltrans *transizioni;
} StatoRete;

typedef struct {
    int da, a, dimRegex;
    struct transizione * t;
    char *regex;
    bool parentesizzata;
} TransizioneRete;

struct ltrans {
    TransizioneRete *t;
    struct ltrans * prossima;
};

StatoRete ** statiSpazio;
int nStatiS=0, nTransSp=0;
int * ok, *osservazione, loss=0;

// UTILI
// void copiaArray(int* da, int* in, int lunghezza){                            // Applicazioni sostituite con memcpy e memmove
// 	int i;
// 	for (i=0; i<lunghezza; i++)
// 		*in++ = *da++;
// }

// void copiaArrayTR(TransizioneRete ** da, TransizioneRete ** in, int lunghezza){
// 	int i;
// 	for (i=0; i<lunghezza; i++)
// 		*in++ = *da++;
// }

// void copiaArraySR(StatoRete** da, StatoRete** in, int lunghezza, bool desc) {
// 	int i;
// 	if (desc) {
//         StatoRete** s = &da[lunghezza-1], **d = &in[lunghezza-1];
//         for (i=lunghezza; i>0; i--)
//             *d-- = *s--;
//     } else
//         for (i=0; i<lunghezza; i++)
//             *in++ = *da++;
// }

// void stampaArray(int* a, int lunghezza){             // Inutilizzata
// 	int i=0;
// 	int* arr = a;
// 	for (; i<lunghezza; i++)
// 		printf("%d ", *a++);
// 	printf("\n");
// 	a=arr;
// }

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

void aggiungiTransizioneInCoda(StatoRete *s, TransizioneRete *t) {
    struct ltrans *ptr = s->transizioni;
    while (ptr != NULL) ptr = ptr->prossima;
    struct ltrans *nuovoElem = calloc(1, sizeof(struct ltrans));
    ptr->prossima = nuovoElem;
    nuovoElem->t = t;
}

// Rendo bufferizzata la gestione della memoria heap per (tutte) le struttre dati (strutture di puntatori e strutture a contatori)
int *sizeofTrComp;
int sizeofSR=5, sizeofCOMP=2, sizeofLINK=2;                    // Dimensioni di partenza

void allocamentoIniziale(void){
    componenti = malloc(sizeofCOMP* sizeof(Componente*));                   // Puntatori ai componenti
    sizeofTrComp = malloc(sizeofCOMP*sizeof(int));                          // Dimensioni degli array puntatori a transizioni nei componenti
    links = malloc(sizeofLINK* sizeof(Link*));                              // Puntatori ai link
    statiSpazio = malloc(sizeofSR* sizeof(StatoRete*));                     // Puntatori agli stati rete
}

void alloc1(char o) {   // Gestione strutture globali (livello zero)
    if (o=='s') {
        if (nStatiS+1 > sizeofSR) {
            sizeofSR += 10;
            StatoRete ** spazio = realloc(statiSpazio, sizeofSR*sizeof(StatoRete*));
            if (spazio == NULL) printf("Errore di allocazione memoria!\n");
            else statiSpazio = spazio;
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

StatoRete * generaStato(int *contenutoLink, int *statiAttivi) {
    StatoRete *s = calloc(1, sizeof(StatoRete));
    s->id = VUOTO;
    if (contenutoLink != NULL) {
        s->contenutoLink = malloc(nlink*sizeof(int));
        memcpy((*s).contenutoLink, contenutoLink, nlink*sizeof(int));
    } else s->contenutoLink = NULL;
    if (statiAttivi != NULL) {
        s->statoComponenti = malloc(ncomp*sizeof(int));
        memcpy((*s).statoComponenti, statiAttivi, ncomp*sizeof(int));
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

            TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
            nuovaTransRete->a = idA;
            nuovaTransRete->da = idDa;
            nuovaTransRete->t = t;
            nuovaTransRete->regex = NULL;
            nuovaTransRete->dimRegex = 0;
            struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->prossima = statiSpazio[idDa]->transizioni;
            statiSpazio[idDa]->transizioni = nuovaLTr;
            if (idA != idDa) {
                nuovaLTr = calloc(1, sizeof(struct ltrans));
                nuovaLTr->t = nuovaTransRete;
                nuovaLTr->prossima = statiSpazio[idA]->transizioni;
                statiSpazio[idA]->transizioni = nuovaLTr;
            }
            nTransSp++;

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
            
            TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
            nuovaTransRete->regex = NULL;
            nuovaTransRete->dimRegex = 0;
            nuovaTransRete->a = idA;
            nuovaTransRete->da = idDa;
            nuovaTransRete->t = t;
            struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
            nuovaLTr->t = nuovaTransRete;
            nuovaLTr->prossima = statiSpazio[idDa]->transizioni;
            statiSpazio[idDa]->transizioni = nuovaLTr;
            if (idA != idDa) {
                nuovaLTr = calloc(1, sizeof(struct ltrans));
                nuovaLTr->t = nuovaTransRete;
                nuovaLTr->prossima = statiSpazio[idA]->transizioni;
                statiSpazio[idA]->transizioni = nuovaLTr;
            }
            nTransSp++;

            fscanf(file, "%s", buffer);
            if (strcmp(buffer, "}") == 0) break;
        }
    }
}

void stampaStruttureAttuali(StatoRete * attuale, bool testuale) {
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
            if (memcmp(nuovo->statoComponenti, s->statoComponenti, ncomp*sizeof(int)) == 0
            && memcmp(nuovo->contenutoLink, s->contenutoLink, nlink*sizeof(int)) == 0) {
                giaPresente = (loss>0 ? nuovo->indiceOsservazione == s->indiceOsservazione : true);
                if (giaPresente) break;
            }
        }
    }
    if (giaPresente) { // Già c'era lo stato, ma la transizione non è detto
        struct ltrans * trans = precedente->transizioni;
        nuovo->id = s->id;
        while (trans != NULL) {
            if (trans->t->a == nuovo->id && trans->t->t->id == mezzo->id) return false;
            trans = trans->prossima;
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        alloc1('s');
        nuovo->id = nStatiS;
        statiSpazio[nStatiS++] = nuovo;
    }
    if (mezzo != NULL){
        TransizioneRete * nuovaTransRete = calloc(1, sizeof(TransizioneRete));
        nuovaTransRete->a = nuovo->id;
        nuovaTransRete->da = precedente->id;
        nuovaTransRete->t = mezzo;
        nuovaTransRete->regex = NULL;
        nuovaTransRete->dimRegex = 0;

        struct ltrans *nuovaLTr = calloc(1, sizeof(struct ltrans));
        nuovaLTr->t = nuovaTransRete;
        nuovaLTr->prossima = statiSpazio[nuovo->id]->transizioni;
        statiSpazio[nuovo->id]->transizioni = nuovaLTr;
        
        nuovaLTr = calloc(1, sizeof(struct ltrans));
        nuovaLTr->t = nuovaTransRete;
        nuovaLTr->prossima = statiSpazio[precedente->id]->transizioni;
        statiSpazio[precedente->id]->transizioni = nuovaLTr;
        nTransSp++;
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
                    //copiaArray(attuale->contenutoLink, nuovoStatoLink, nlink);
                    memcpy(nuovoStatoLink, attuale->contenutoLink, nlink*sizeof(int));
                    //copiaArray(attuale->statoComponenti, nuoviStatiAttivi, ncomp);
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
    StatoRete *s;
    for (s=statiSpazio[i=0]; i<nStatiS; s=statiSpazio[++i]) {
        struct ltrans * trans = s->transizioni;
        while (trans != NULL) {
            if (trans->t->da == i) {
                if (rinomina) fprintf(file, "S%d -> S%d [label=t%d", i, trans->t->a, trans->t->t->id);
                else fprintf(file, "%s -> %s [label=t%d", nomeSpazi[i], nomeSpazi[trans->t->a], trans->t->t->id);
                if (trans->t->t->oss>0) fprintf(file, "O%d", trans->t->t->oss);
                if (trans->t->t->ril>0) fprintf(file, "R%d", trans->t->t->ril);
                fprintf(file, "]\n");
            }
            trans = trans->prossima;
        }
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

void eliminaStato(int i) {
    int j;
    StatoRete *r, *s = statiSpazio[i];
    for (; s->transizioni != NULL; s->transizioni = s->transizioni->prossima) {
        if (s->transizioni->t->da != i && s->transizioni->t->a != i) continue;
        nTransSp--;
        int eliminaAncheDa = -1;
        if (s->transizioni->t->da != i) eliminaAncheDa = s->transizioni->t->da;
        if (s->transizioni->t->a != i) eliminaAncheDa = s->transizioni->t->a;
        if (eliminaAncheDa != -1) {
            struct ltrans *trans = statiSpazio[eliminaAncheDa]->transizioni, *transPrima = NULL;
            for (; trans != NULL; trans = trans->prossima) {
                if (trans->t->da == i || trans->t->a == i) {
                    if (transPrima == NULL) statiSpazio[eliminaAncheDa]->transizioni = trans->prossima;
                    else transPrima->prossima = trans->prossima;
                } else transPrima = trans;
            }
        }
    }
    for (r=statiSpazio[j=0]; j<nStatiS; r=statiSpazio[++j]) {
        struct ltrans *trans = r->transizioni;
        for (; trans != NULL; trans = trans->prossima) {
            if (trans->t->da >i && trans->t->da == j) trans->t->da--;
            if (trans->t->a >i && trans->t->a == j) trans->t->a--;
        }
    }
    if (s->contenutoLink != NULL) free(statiSpazio[i]->contenutoLink);
    if (s->statoComponenti != NULL) free(statiSpazio[i]->statoComponenti);
    free(s);
    nStatiS--;
    memcpy(statiSpazio+i, statiSpazio+i+1, (nStatiS-i)*sizeof(StatoRete*));
    for (j=0; j<nStatiS; j++) {                                         //  Abbasso l'id degli stati successivi
        if (statiSpazio[j]->id>=i) statiSpazio[j]->id--;
    }
}

void potsSC(StatoRete *s) { // Invocata esternamente solo a partire dagli stati finali
    ok[s->id] = 1;
    struct ltrans * trans = s->transizioni;
    for (; trans != NULL; trans=trans->prossima) {
        if (trans->t->a == s->id && !ok[trans->t->da])
            potsSC(statiSpazio[trans->t->da]);
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
            memcpy(ok+i, ok+i+1, (nStatiS-i)*sizeof(int));
            i--;
        }
    }
    free(ok);
}

void diagnostica(void) {
    int i=0, j=0;
    char *temp = malloc(REGEX*50*nTransSp);     // Sfondare il buffer implica probabile crash del programma. N transizioni -> buffer di N/2 KB
    Transizione *tvuota = calloc(1, sizeof(Transizione));
    StatoRete *r;
    for (r=statiSpazio[j=nStatiS-1]; j>=0; r=statiSpazio[--j]) {
        struct ltrans *trans = r->transizioni;
        while (trans != NULL) {
            if (trans->t->da == j) {
                trans->t->da++;
                trans->t->a++;
            }
            trans = trans->prossima;
        }
        r->id++;
    }
    alloc1('s');
    memmove(statiSpazio+1, statiSpazio, nStatiS*sizeof(StatoRete*));
    nStatiS++;
    statiSpazio[0] = calloc(1, sizeof(StatoRete));
    statiSpazio[0]->id = 0;
    TransizioneRete * nuovaTr = calloc(1, sizeof(TransizioneRete));
    nuovaTr->da = 0;
    nuovaTr->a = 1;
    nuovaTr->t = tvuota;
    statiSpazio[0]->transizioni = calloc(1, sizeof(struct ltrans));
    statiSpazio[0]->transizioni->t = nuovaTr;
    struct ltrans *vecchiaTesta = statiSpazio[1]->transizioni;
    statiSpazio[1]->transizioni = calloc(1, sizeof(struct ltrans));
    statiSpazio[1]->transizioni->t = nuovaTr;
    statiSpazio[1]->transizioni->prossima = vecchiaTesta;
    nTransSp++;

    alloc1('s');
    StatoRete * fine = calloc(1, sizeof(StatoRete));                           // Generazione nuovo stato finale
    fine->id = nStatiS;
    fine->finale = true;
    for (i=1; i<nStatiS; i++) {                                                 // Collegamento ex stati finali col nuovo
        if (statiSpazio[i]->finale) {
            TransizioneRete * trFinale = calloc(1, sizeof(TransizioneRete));
            trFinale->da = i;
            trFinale->a = nStatiS;
            trFinale->t = tvuota;
            struct ltrans *vecchiaTesta = statiSpazio[i]->transizioni;
            statiSpazio[i]->transizioni = calloc(1, sizeof(struct ltrans));
            statiSpazio[i]->transizioni->t = trFinale;
            statiSpazio[i]->transizioni->prossima = vecchiaTesta;
            vecchiaTesta = fine->transizioni;
            fine->transizioni = calloc(1, sizeof(struct ltrans));
            fine->transizioni->t = trFinale;
            fine->transizioni->prossima = vecchiaTesta;
            nTransSp++;
            statiSpazio[i]->finale = false;
        }
    }
    statiSpazio[nStatiS++] = fine;
    for (r=statiSpazio[j=0]; j<nStatiS; r=statiSpazio[++j]) {
        struct ltrans *trans = r->transizioni;
        while (trans != NULL) {
            if (trans->t->da == j) {
                trans->t->regex = calloc(REGEX, 1);
                trans->t->dimRegex = REGEX;
                trans->t->parentesizzata = true;
                if (trans->t->t->ril >0) sprintf(trans->t->regex, "r%d", trans->t->t->ril);
            }
            trans = trans->prossima;
        }
    }
    
    while (nTransSp>1) {
        bool continua = false;
        for (i=0; i<nStatiS; i++) {                                         // Semplificazione serie -> unità
            TransizioneRete *tentra, *tesce;
            if (statiSpazio[i]->transizioni != NULL && statiSpazio[i]->transizioni->prossima != NULL && statiSpazio[i]->transizioni->prossima->prossima == NULL &&
            (((tesce = statiSpazio[i]->transizioni->t)->da == i && (tentra = statiSpazio[i]->transizioni->prossima->t)->a == i)
            || ((tentra = statiSpazio[i]->transizioni->t)->a == i && (tesce = statiSpazio[i]->transizioni->prossima->t)->da == i)) && tesce->da != tesce->a && tentra->a != tentra->da)  {       // Elemento interno alla sequenza
                int strl1 = strlen(tentra->regex), strl2 = strlen(tesce->regex);
                TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                nt->da = tentra->da;
                nt->a = tesce->a;
                nt->t = tvuota;
                if (strl1>0 & strl2>0) nt->parentesizzata = false;
                else nt->parentesizzata = tentra->parentesizzata;
                int dimNt = strl1+strl2+20 < REGEX ? REGEX : strl1+strl2+20;
                nt->regex = calloc(dimNt, 1);
                nt->dimRegex = dimNt;
                sprintf(nt->regex, "%s%s", tentra->regex, tesce->regex);
                nTransSp++;
                struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans));
                nuovaTr->t = nt;
                struct ltrans *templtrans = statiSpazio[tentra->da]->transizioni;
                statiSpazio[tentra->da]->transizioni = nuovaTr;
                statiSpazio[tentra->da]->transizioni->prossima = templtrans;
                if (tentra->da != tesce->a) {
                    templtrans = statiSpazio[tesce->a]->transizioni;
                    nuovaTr = calloc(1, sizeof(struct ltrans));
                    nuovaTr->t = nt;
                    statiSpazio[tesce->a]->transizioni = nuovaTr;
                    statiSpazio[tesce->a]->transizioni->prossima = templtrans;
                }
                eliminaStato(i);
                continua = true;
            }
        }
        if (continua) continue;
        for (i=0; i<nStatiS; i++) {                                           // Collasso gruppi di transizioni che condividono partenze e arrivi in una
            struct ltrans *trans1 = statiSpazio[i]->transizioni;
            while (trans1 != NULL) {
                struct ltrans *trans2 = statiSpazio[i]->transizioni, *nodoPrecedente = NULL;
                while (trans2 != NULL) {
                    if (trans1->t != trans2->t && trans1->t->da == trans2->t->da && trans1->t->a == trans2->t->a) {
                        int strl1 = strlen(trans1->t->regex), strl2 = strlen(trans2->t->regex);
                        if (strl1 > 0 && strl1==strl2 && strcmp(trans1->t->regex, trans2->t->regex)==0);
                        else if (strl1 > 0 && strl2 > 0) {
                            if (trans1->t->dimRegex < strl1 + strl2 + 4) {
                                char * nuovaReg = realloc(trans1->t->regex, (strl1+strl2)*2+4);
                                trans1->t->dimRegex = (strl1+strl2)*2+4;
                                trans1->t->regex = nuovaReg;
                            }
                            sprintf(temp, "(%s|%s)", trans1->t->regex, trans2->t->regex);
                            strcpy(trans1->t->regex, temp);
                            trans1->t->parentesizzata = true;
                        } else if (strl1 > 0 && strl2==0) {
                            int deltaCaratteri;
                            deltaCaratteri = trans1->t->parentesizzata? 1 : 3;
                            if (trans1->t->dimRegex < strl1 + deltaCaratteri) {
                                char * nuovaReg = realloc(trans2->t->regex, strl1*2+deltaCaratteri);
                                trans1->t->dimRegex = strl1*2+deltaCaratteri;
                                trans1->t->regex = nuovaReg;
                                trans1->t->parentesizzata = true;
                            }
                            if (trans1->t->parentesizzata) sprintf(temp, "%s?", trans1->t->regex);
                            else sprintf(temp, "(%s)?", trans1->t->regex);
                            strcpy(trans1->t->regex, temp);
                            trans1->t->parentesizzata = true;  // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
                        } else if (strl2 > 0) {
                            int deltaCaratteri;
                            deltaCaratteri = trans2->t->parentesizzata? 1 : 3;
                            if (trans1->t->dimRegex < strl2 + deltaCaratteri) {
                                char * nuovaReg = realloc(trans1->t->regex, (strl2)*2+deltaCaratteri);
                                trans1->t->dimRegex = strl2*2+deltaCaratteri;
                                trans1->t->regex = nuovaReg;
                            }
                            if (trans2->t->parentesizzata) sprintf(trans1->t->regex, "%s?", trans2->t->regex);
                            else sprintf(trans1->t->regex, "(%s)?", trans2->t->regex);
                            trans1->t->parentesizzata = true;  // Anche se non termina con ), non ha senso aggiungere ulteriori parentesi
                        }
                        //free(t2->regex);
                        nodoPrecedente->prossima = trans2->prossima;
                        int idnodopartenza = trans2->t->da == i? trans2->t->a : trans2->t->da;
                        struct ltrans *listaNelNodoDiPartenza = statiSpazio[idnodopartenza]->transizioni, *precedenteNelNodoDiPartenza = NULL;
                        while (listaNelNodoDiPartenza != NULL) {
                            if (listaNelNodoDiPartenza->t == trans2->t) {
                                if (precedenteNelNodoDiPartenza == NULL) statiSpazio[idnodopartenza]->transizioni = listaNelNodoDiPartenza->prossima;
                                else precedenteNelNodoDiPartenza->prossima = listaNelNodoDiPartenza->prossima;
                                break;
                            }
                            precedenteNelNodoDiPartenza = listaNelNodoDiPartenza;
                            listaNelNodoDiPartenza = listaNelNodoDiPartenza->prossima;
                        }
                        nTransSp--;
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
        for (i=1; i<nStatiS-1; i++) {
            struct ltrans *trans1 = statiSpazio[i]->transizioni;
            while (trans1 != NULL) {
                if (trans1->t->a == i && trans1->t->da != i) {              // Transizione entrante
                    struct ltrans *trans2 = statiSpazio[i]->transizioni;
                    while (trans2 != NULL) {
                        if (trans2->t->da == i && trans2->t->a != i) {      // Transizione uscente
                            bool esisteAutoTransizioneN = false;
                            struct ltrans *trans3 = statiSpazio[i]->transizioni;
                            while (trans3 != NULL) {
                                if (trans3->t->a == trans3->t->da) {
                                    azioneEffettuataSuQuestoStato = esisteAutoTransizioneN = true;
                                    int strl1 = strlen(trans1->t->regex), strl2 = strlen(trans2->t->regex), strl3 = strlen(trans3->t->regex),
                                    dimNt = strl1+strl2+strl3+20 < REGEX ? REGEX : strl1+strl2+strl3+20;
                                    TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                                    nt->da = trans1->t->da;
                                    nt->a = trans2->t->a;
                                    nt->t = tvuota;
                                    nt->regex = calloc(dimNt, 1);
                                    nt->dimRegex = dimNt;
                                    if (strl3 == 0 ) {
                                        if (strl1>0 & strl2>0) {
                                            sprintf(nt->regex, "(%s%s)", trans1->t->regex, trans2->t->regex);
                                            nt->parentesizzata = true;
                                        }
                                        else if (strl1 == 0 & strl2>0) {
                                            strcpy(nt->regex, trans2->t->regex);
                                            nt->parentesizzata = trans2->t->parentesizzata;
                                        }
                                        else if (strl2 == 0 & strl1>0) {
                                            strcpy(nt->regex, trans1->t->regex);
                                            nt->parentesizzata = trans1->t->parentesizzata;
                                        }
                                    } else {
                                        if (strl1 + strl2 >0) {
                                            if (trans3->t->parentesizzata)  sprintf(nt->regex, "(%s%s*%s)", trans1->t->regex, trans3->t->regex, trans2->t->regex);
                                            else sprintf(nt->regex, "(%s(%s)*%s)", trans1->t->regex, trans3->t->regex, trans2->t->regex);
                                        }
                                        else if (trans3->t->parentesizzata) sprintf(nt->regex, "%s*", trans3->t->regex);
                                        else sprintf(nt->regex, "(%s)*", trans3->t->regex);
                                        nt->parentesizzata = true; // Se anche termina con *, non importa
                                    }
                                    struct ltrans *statoDa = statiSpazio[nt->da]->transizioni, *statoA = statiSpazio[nt->a]->transizioni;
                                    struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans));
                                    nuovaTr->t = nt;
                                    nuovaTr->prossima = statoDa;
                                    statiSpazio[nt->da]->transizioni = nuovaTr;
                                    nuovaTr = calloc(1, sizeof(struct ltrans));
                                    nuovaTr->t = nt;
                                    nuovaTr->prossima = statoA;
                                    statiSpazio[nt->a]->transizioni = nuovaTr;
                                    nTransSp++;
                                }
                                trans3 = trans3->prossima;
                            }
                            if (!esisteAutoTransizioneN) {
                                azioneEffettuataSuQuestoStato = true;
                                TransizioneRete *nt = calloc(1, sizeof(TransizioneRete));
                                nt->da = trans1->t->da;
                                nt->a = trans2->t->a;
                                nt->t = tvuota;
                                int strl1 = strlen(trans1->t->regex), strl2 = strlen(trans2->t->regex),
                                dimNt = strl1+strl2+20 < REGEX ? REGEX : strl1+strl2+20;
                                nt->regex = calloc(dimNt, 1);
                                nt->dimRegex = dimNt;
                                if (strl1>0 & strl2>0) {
                                    sprintf(nt->regex, "(%s%s)", trans1->t->regex, trans2->t->regex);
                                    nt->parentesizzata = true;
                                }
                                else if (strl1 == 0 & strl2>0) {
                                    strcpy(nt->regex, trans2->t->regex);
                                    nt->parentesizzata = trans2->t->parentesizzata;
                                } else if (strl2 == 0 & strl1>0) {
                                    strcpy(nt->regex, trans1->t->regex);
                                    nt->parentesizzata = trans1->t->parentesizzata;
                                }
                                struct ltrans *nuovaTr = calloc(1, sizeof(struct ltrans)), *nuovaTr2 = calloc(1, sizeof(struct ltrans));
                                nuovaTr->t = nt;
                                nuovaTr->prossima = statiSpazio[nt->da]->transizioni;
                                statiSpazio[nt->da]->transizioni = nuovaTr;
                                nuovaTr2->t = nt;
                                nuovaTr2->prossima = statiSpazio[nt->a]->transizioni;
                                statiSpazio[nt->a]->transizioni = nuovaTr2;
                                nTransSp++;
                            }
                        }
                        trans2 = trans2->prossima;
                    }
                }
                trans1 = trans1->prossima;
            }
            if (azioneEffettuataSuQuestoStato) {
                eliminaStato(i);
                break;
            }
        }
    }
    //free(temp);  Errore su Windows
    printf("REGEX: %s\n", statiSpazio[0]->transizioni->t->regex);
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

int main(int argc, char *argv[]) {
    printf(LOGO);
    char sceltaDot, sceltaOperazione, pota, nomeFileSC[100], sceltaDiag, sceltaRinomina;
    if (argc >1) strcpy(nomeFile, argv[1]);
    else {
        printf("Indicare il file che contiene la definizione dell'automa: ");
        fflush(stdout);
        scanf("%99s", nomeFile);
    }
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
    ottieniComando(&sceltaDot);
    stampaStruttureAttuali(iniziale, sceltaDot != 's');
    printf("Generare spazio comportamentale (c), fornire un'osservazione lineare (o), o caricare uno spazio da file (f, se gli stati sono rinominati: g)? ");
    ottieniComando(&sceltaOperazione);
    if (sceltaOperazione=='o') {
        impostaDatiOsservazione();
        iniziale->finale = false;
    } else if (sceltaOperazione=='f' || sceltaOperazione=='g') {
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
        parseDot(fileSC, sceltaOperazione=='g');
        fclose(fileSC);
        if (loss==0 & sceltaOperazione=='f') printf("Lo stato non corrisponde ad un'osservazione lineare, pertanto non si consiglia un suo utilizzo per diagnosi\n");
        if (sceltaOperazione=='g') printf("Non e' possibile stabilire se lo spazio importato sia derivante da un'osservazione lineare: eseguire una diagnosi solo in caso affermativo\n");
    }
    if (sceltaOperazione != 'f' && sceltaOperazione!='g') {
        printf("Generazione spazio comportamentale...\n");
        ampliaSpazioComportamentale(NULL, iniziale, NULL);
        generaSpazioComportamentale(iniziale);
        printf("Effettuare potatura (s/n)? ");
        ottieniComando(&pota);
        if (pota=='s'& nTransSp>0) potatura();
        printf("Generato lo spazio: conta %d stati e %d transizioni\n", nStatiS, nTransSp);
        if (sceltaDot=='s') {
            printf("Rinominare gli spazi col loro id (s/n)? ");
            ottieniComando(&sceltaRinomina);
            stampaSpazioComportamentale(sceltaRinomina=='s');
        }
    }
    if (sceltaOperazione=='f' || sceltaOperazione=='g' || (sceltaOperazione=='o' & pota=='s')) {
        printf("Eseguire una diagnosi su questa osservazione (s/n)? ");
        fflush(stdout);
        ottieniComando(&sceltaDiag);
        if (sceltaDiag=='s') {
            printf("Eseguo diagnostica... \n");
            diagnostica();
        }
    }
	return(0);
}