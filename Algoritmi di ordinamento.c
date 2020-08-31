#include <stdbool.h>
#include <stdlib.h>

typedef struct slista {
	int n;
	struct slista *prima, *dopo;
} Node;

void isort(int* a, int lunghezza){
	int i, j, chiave;
	for (j = 1; j<lunghezza; j++) {
		chiave = a[j];
		i = j - 1;
		while (i >= 0 & a[i] > chiave)
			a[i+1] = a[i--];
		a[i + 1] = chiave; 
	}
}

void merge(int*a, int p, int q, int r) {
	int i = p, j = q+1, k = 0;
    int b[r-p+1];  //Array esterno in cui accumulare il risultato

    while (i <= q & j <= r)
		b[k++] = a[i] <= a[j] ? a[i++] : a[j++];
    while (i <= q)
		b[k++] = a[i++];
    while (j <= r)
       b[k++] = a[j++];
    for (k=p; k<=r; k++)
       a[k] = b[k-p];
}

void msort(int* a, int p, int r){
	if (p<r){
		int q = (p+r)/2;
		msort(a, p, q);
		msort(a, q+1, r);
		merge(a, p, q, r);
	}
}

void heapify(int a[], int i, int heap) {
	int sx = 2*i+1, dx = 2*(i+1);  //padre = (i-1)/2
	int largest = (sx < heap && a[sx] > a[i]) ? sx : i;
	largest = (dx < heap && a[dx] > a[largest]) ? dx : largest;
	//In largest è ora memorizzato l’indice del maggiore fra A[i], A[LEFT(i)] e A[RIGHT(i)]
	if (largest != i) {
		int temp = a[i];
		a[i] = a[largest];
		a[largest] = temp;
		heapify(a, largest, heap);
	}
}

void buildHeap(int a[], int lunghezza){
	int i=(lunghezza-1)/2; //Ultimo padre
	for (; i>=0; i--)
		heapify(a, i, lunghezza); // gli elementi del sottoarray A[length[A]/2+1..length[A]] sono tutte foglie → ciascuno di essi è uno heap di un solo elemento
}

void heapsort(int a[], int lunghezza){
	buildHeap(a, lunghezza);
	int i=lunghezza-1, temp;
	for (; i>0; i--) {
		temp = a[0];
		a[0] = a[i];
		a[i] = temp; //Messo l'elemento massimo (in radice) in fondo
		heapify(a, 0, --lunghezza);
	}
}

int partitionQS(int* a, int p, int r){
	int x=a[p]; //x è l’elemento perno
	r++; p--;
	while (true) {
		while (a[--r] > x);
		while (a[++p] < x);
		if (p<r) {
			int temp = a[p];
			a[p] = a[r];
			a[r] = temp;
		}
		else return r;
	}
}

void quicksort(int* a, int p, int r){
	if (p < r) {
		int q = partitionQS(a,p,r);
		quicksort(a,p,q);
		quicksort(a,q+1,r);
	}
}

int quickSelect(int *a, int da, int fino, int iesimo){ // 1-esimo: a[0] in a ordinato
	while (fino-da>1){
		int q = partitionQS(a, da, fino);
		int k = q-da+1; // Numero elementi nella parte bassa
		if (iesimo+1<=k) fino = q+1;
		else {da = q+1; iesimo = iesimo-k;}
	}
	return a[da];
}

void countingSort(int* a, int lunghezza, int k) {
	int* b = calloc(lunghezza, sizeof(int)), *c = calloc(k, sizeof(int));
	int i, j;
	for (i=0; i<lunghezza; i++)
		c[a[i]]++; // C[i] contiene ora il numero di elementi uguali a i
	for (i=1; i<k; i++)
		c[i] += c[i-1]; // C[i] contiene ora il numero di elementi <= i
	for (i=lunghezza-1; i>-1; i--)
		b[--c[j=a[i]]] = j;
	copiaArray(b, a, lunghezza);
	free(c); free(b);
}

void bucketSort(int *a, int lunghezza, int min, int max) {
	Node *buckets = malloc(lunghezza*sizeof(Node));
	int i, j=0;
	for (i=0; i<lunghezza; i++) {
		buckets[i].n = -1;
		buckets[i].dopo = NULL;
	}
	for (i=0; i<lunghezza; i++) { // inserisci A[i] nella lista B[nA[i]]
		int indice = lunghezza*(a[i]-min)/(max-min);
		//Node *bucket = &(buckets[indice]);
		if (buckets[indice].n == -1)
			buckets[indice].n = a[i];
		else {
			Node *nuovo = malloc(sizeof(Node));	// Per inserire in coda, effetturare un ciclo while
			nuovo->n = buckets[indice].n;		// Questo codice aggiunge in seconda posizione un nodo
			nuovo->dopo = buckets[indice].dopo; // Vi copia i valori della testa e sovrascrive la testa
			buckets[indice].n = a[i];			// Ciò è necessario perché buckets è puntatore a nodi
			buckets[indice].dopo = nuovo;		// Non puntatore a puntatori a nodi
		}
	}
	for (i=0; i<lunghezza; i++) { // Ordina la lista b[i] e scrivi in a il risultato
		Node bucket = buckets[i];
		if (bucket.n == -1) continue;
		int lSottoLista=0;
		while (true) {
			a[j++] = bucket.n;
			lSottoLista++;
			if (bucket.dopo == NULL) break;
			else bucket = *(bucket.dopo);
		}
		isort(a+j-lSottoLista, lSottoLista);
	}
}

void copiaArray(int* da, int* in, int lunghezza){
	int i;
	for (i=0; i<lunghezza; i++)
		*in++ = *da++;
}

bool verificaOrdinamento(int* a, int lunghezza){
	int* arr = a, i;
	for (i=0; i<lunghezza-1; i++)
		if (*a > *(++a)) return false;
	return true;
}

void stampaArray(int* a, int lunghezza){
	int i=0;
	int* arr = a;
	for (; i<lunghezza; i++)
		printf("%d ", *a++);
	printf("\n");
	a=arr;
}