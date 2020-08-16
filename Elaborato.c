#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define VUOTO -1
#define ACAPO -10

char nomeFile[FILENAME_MAX];

// DEFINIZIONE STATICA DI UN MODELLO COMPORTAMENTALE
typedef struct link {
    struct componente *da, *a;
    int id, intId;
} Link;

typedef struct transizione {
    int oss, ril, da, a, id;
    int idEventoIn, *idEventiU, nEventiU;
    struct link *linkIn, **linkU;
} Transizione;

typedef struct componente {
    int /**stati,*/ nStati, id, intId, nTransizioni;
    struct transizione **transizioni;
} Componente;

Componente **componenti;
Link **links;
int nlink=0, ncomp=0;

// COMPORTAMENTO DINAMICO DELLA RETE
typedef struct statorete {
    int *statoComponenti, *contenutoLink;
    int id;
    bool finale;
} StatoRete;

typedef struct {
    int da, a;
    struct transizione * t;
} TransizioneRete;

StatoRete ** statiSpazio;
TransizioneRete ** transizioniSpazio;
int nStatiS=0, nTransSp=0;
// int *contenutoLink, *statiAttivi; // Da essere intesi come dei temporanei




void copiaArray(int* da, int* in, int lunghezza){
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
    char sezione[15], c;
    int i, idComponenteAttuale=0, idLinkAttuale=0;
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
                Componente *nuovo = malloc(sizeof(Componente));
                fscanf(file, "%d", &(nuovo->nStati));
                match(',', file);
                fscanf(file, "%d", &(nuovo->id));
                nuovo->intId = idComponenteAttuale++;   // Rappresentazione intera sequenziale, da zero
                /*int *stati = malloc(nstati*sizeof(int));
                for (i=0; i<nstati; i++)
                    stati[i]=i;
                nuovo->stati = stati;*/
                //statiAttivi = realloc(statiAttivi, (ncomp+1)*sizeof(int));  // Aggiungere un nuovo componente implica
                //statiAttivi[ncomp] = 0;                                     // allargare l'elenco degli stati attivi
                nuovo->transizioni = NULL;
                nuovo->nTransizioni = 0;
                componenti = realloc(componenti, (ncomp+1)*sizeof(Componente*)); // e la lista dei componenti stessi
                componenti[ncomp++] = nuovo;

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
                Transizione *nuovo = malloc(sizeof(Transizione));
                int temp, componentePadrone;
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
                if (isdigit(c = fgetc(file))) { // Se dopo la virgola c'è un numero, allora c'è un evento in ingresso
                    ungetc(c, file);
                    fscanf(file, "%d", &(nuovo->idEventoIn));
                    match(':', file); //idEvento:idLink
                    fscanf(file, "%d", &temp);
                    nuovo->linkIn = linkDaId(temp);
                } else {
                    nuovo->idEventoIn = VUOTO;
                    nuovo->linkIn = NULL;
                    ungetc(c, file);
                }
                match('|', file); // Altrimenti se c'è | allora l'evento in ingresso è nullo
                nuovo->nEventiU = 0;
                nuovo->linkU = NULL;
                while (!feof(file) && (c=fgetc(file)) != '\n') { //Creazione lista di eventi in uscita
                    if (feof(file)) break;
                    ungetc(c, file);
                    int l = nuovo->nEventiU;

                    fscanf(file, "%d", &temp);
                    nuovo->idEventiU = realloc(nuovo->idEventiU, (l+1)*sizeof(int));
                    nuovo->idEventiU[l] = temp;

                    match(':', file); //idEvento:idLink
                    fscanf(file, "%d", &temp);
                    Link *linkNuovo = linkDaId(temp);
                    nuovo->linkU = realloc(nuovo->linkU, (l+1)*sizeof(Link*));
                    nuovo->linkU[l] = linkNuovo;
                    nuovo->nEventiU++;
                    match(',', file);
                }
                if (!feof(file)) ungetc(c, file);
                // Aggiunta della neonata transizione al relativo componente
                Componente *padrone = compDaId(componentePadrone);
                padrone->transizioni = realloc(padrone->transizioni, (padrone->nTransizioni+1)*sizeof(Transizione*));
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
                Link *nuovo = malloc(sizeof(Link));
                int idComp1, idComp2;
                fscanf(file, "%d", &idComp1);
                match(',', file);
                fscanf(file, "%d", &idComp2);
                match(',', file);
                fscanf(file, "%d", &(nuovo->id));
                nuovo->intId = idLinkAttuale++;
                nuovo->da = compDaId(idComp1);
                nuovo->a = compDaId(idComp2);
                
                //contenutoLink = realloc(contenutoLink, (nlink+1)*sizeof(int));  // Aggiungere un nuovo link implica
                //contenutoLink[nlink] = VUOTO;                                   // allargare l'elenco dei loro contenuti
                links = realloc(links, (nlink+1)*sizeof(Link*));                // e la loro lista
                links[nlink++] = nuovo;

                if (!feof(file)) match(ACAPO, file); 
            }
        } else if (!feof(file))
            printf("Riga non corretta: %s\n", sezione);
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
    // for (i=0; i<ncomp; i++)
    //     s->statoComponenti[i] = componenti[i]->statoAttivo;
    // for (i=0; i<nlink; i++)
    //     s->contenutoLink[i] = links[i]->idEventoContenuto;
    s->finale = true;
    for (i=0; i<nlink; i++)
       s->finale &= (s->contenutoLink[i] == VUOTO);
    return s;
}

bool ampliaSpazioComportamentale(StatoRete * precedente, StatoRete * nuovo, Transizione * nuovaTrans){
    int i, j;
    bool giaPresente = false;
    StatoRete *s;
    if (nStatiS>0) {
        for (s=statiSpazio[i=0]; i<nStatiS; s=(i<nStatiS ? statiSpazio[++i] : s)) { // Per ogni stato dello spazio comportamentale
            bool stessiAttivi = true;
            for (j=0; j<ncomp; j++) {                                               // Uguaglianza liste stati attivi
                stessiAttivi &= (nuovo->statoComponenti[j] == s->statoComponenti[j]);
            }
            if (stessiAttivi) {
                bool stessiEventi = true;
                for (j=0; j<nlink; j++)                                             // Uguaglianza stato link
                    stessiEventi &= (nuovo->contenutoLink[j] == s->contenutoLink[j]);
                if (stessiEventi) {
                    giaPresente = true;
                    break;
                }
            }
        }
    }
    if (giaPresente) { // Già c'era lo stato, ma la transizione non è detto
        if (nTransSp>0) {
            nuovo->id = s->id;
            TransizioneRete * trans;
            bool giaPresenteTr = false;
            for (trans=transizioniSpazio[i=0]; i<nTransSp; trans=(i<nTransSp ? transizioniSpazio[++i] : trans)) { // Controllare se la transizione già esisteva
                if ((trans->da == precedente->id) && (trans->a == s->id) && (trans->t->id == nuovaTrans->id)) {
                    giaPresenteTr = true;
                    break;
                }
            }
            if (giaPresenteTr) return false;
        }
    } else { // Se lo stato è nuovo, ovviamente lo è anche la transizione che ci arriva
        statiSpazio = realloc(statiSpazio, (nStatiS+1)*sizeof(StatoRete*));
        nuovo->id = nStatiS;
        statiSpazio[nStatiS++] = nuovo;
    }
    if (nuovaTrans != NULL){
        transizioniSpazio = realloc(transizioniSpazio, (nTransSp+1)*sizeof(TransizioneRete*));
        TransizioneRete * nuovaTransRete = malloc(sizeof(TransizioneRete));
        nuovaTransRete->a = nuovo->id;
        nuovaTransRete->da = precedente->id;
        nuovaTransRete->t = nuovaTrans;
        transizioniSpazio[nTransSp++] = nuovaTransRete;
    }
    return !giaPresente;
}

void stampaStruttureAttuali(StatoRete * attuale, bool testuale){
    int i, j, k;
    Link *l;
    Componente *c;
    FILE * file;
    char nomeFileDot[strlen(nomeFile)+7], nomeFilePDF[strlen(nomeFile)+7], *comando;
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
            else fprintf(file, "C%d_%d -> C%d_%d [label=\"t%d\"];\n", c->id, t->da, c->id, t->a, t->id);
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

void generaSpazioComportamentale(StatoRete * attuale){
    Componente *c;
    int i, j, k;
    for (c=componenti[i=0]; i<ncomp;  c=componenti[++i]){
        Transizione *t;
        int nT = c->nTransizioni;
        if (nT == 0) continue;
        for (t=c->transizioni[j=0]; j<nT; t=(j<nT ? c->transizioni[++j] : t)) {
            if (attuale->statoComponenti[c->intId] == t->da &&            // Transizione abilitata se stato attuale = da
            (t->idEventoIn == VUOTO || t->idEventoIn == attuale->contenutoLink[t->linkIn->intId])) { // Ok eventi in ingresso
                bool ok = true;                                          // I link di uscita sono vuoti
                for (k=0; k<t->nEventiU; k++)
                    if (attuale->contenutoLink[t->linkU[k]->intId] != VUOTO) {
                        ok = false;
                        break;
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
                    // Ora bisogna inserire il nuovo stato e la nuova transizione nello spazio
                    bool avanzamento = ampliaSpazioComportamentale(attuale, nuovoStato, t);
                    //Se non è stato aggiunto un nuovo stato allora stiamo come prima: taglio
                    if (avanzamento) generaSpazioComportamentale(nuovoStato);
                }
            }
        }
    }
    //printf("Attenzione! Superato il tetto al numero di iterazioni per il calcolo dello spazio comportamentale. Terminazione.\n");
}

void stampaSpazioComportamentale() {
    int i, j;
    char* nomeSpazi[nStatiS], nomeFileDot[strlen(nomeFile)+7], nomeFilePDF[strlen(nomeFile)+7], *comando;
    sprintf(nomeFileDot, "SC%s.dot", nomeFile);
    FILE * file = fopen(nomeFileDot, "w");
    fprintf(file ,"digraph SC%s {\n", nomeFile);
    for (i=0; i<nStatiS; i++) {
        if (statiSpazio[i]->finale) fprintf(file, "node [shape=doublecircle]; ");
        else  fprintf(file, "node [shape=circle]; ");
        char* puntatore = nomeSpazi[i] = malloc(20*sizeof(char));
        sprintf(puntatore,"SR_");
        puntatore += 3;
        for (j=0; j<ncomp; j++)
            sprintf(puntatore++,"%d", statiSpazio[i]->statoComponenti[j]);
        sprintf(puntatore,"_SL_");
        puntatore+=4;
        for (j=0; j<nlink; j++)
            if (statiSpazio[i]->contenutoLink[j] == VUOTO) sprintf(puntatore++,"e");
            else sprintf(puntatore++, "%d", statiSpazio[i]->contenutoLink[j]);
        fprintf(file, "%s ;\n", nomeSpazi[i]);
    }
    for (i=0; i<nTransSp; i++)
        fprintf(file, "%s -> %s [label=t%d]\n", nomeSpazi[transizioniSpazio[i]->da], nomeSpazi[transizioniSpazio[i]->a], transizioniSpazio[i]->t->id);
    fprintf(file, "}");
    fclose(file);
    sprintf(nomeFilePDF, "SC%s.pdf", nomeFile);
    sprintf(comando, "dot -Tpdf -o %s %s", nomeFilePDF, nomeFileDot);
    system(comando);
}

int main(void) {
    printf("Benvenuto!\nIndicare il file da analizzare: ");
	scanf("%s", nomeFile);
	FILE* file = fopen(nomeFile, "rb+");
	if (file == NULL) {
		printf("File inesistente!\n");
		return -1;
	}
	parse(file);
	fclose(file);
	printf("Parsing effettuato...\n");
    int * statiAttivi = calloc(ncomp, sizeof(int));
    int statoLink[nlink], i;
    for (i=0; i<nlink; i++)
        statoLink[i] = VUOTO;
    StatoRete * iniziale = generaStato(statoLink, statiAttivi);

    char *sceltaDot;
    printf("Salvare i grafi come .dot (s/n)? ");
    scanf("%s", sceltaDot);
    if (sceltaDot[0]=='s') stampaStruttureAttuali(iniziale, false);
    else stampaStruttureAttuali(iniziale, true);
    printf("Generazione spazio comportamentale...\n");
    ampliaSpazioComportamentale(NULL, iniziale, NULL);
    generaSpazioComportamentale(iniziale);
    if (sceltaDot[0]=='s') stampaSpazioComportamentale();
	//system("pause");
	return(0);
}