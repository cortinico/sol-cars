/**
  \file 
  \brief test su grafo stradale diretto

 \author lso11 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <mcheck.h>
#include "dgraph.h"

/** file dati mappa toscana: file nodi */
#define NODESFILE "./CITY.txt"
/** file dati mappa toscana: file archi */
#define ARCSFILE  "./TUSCANY.map"
/** file scrittura mappa toscana: file nodi */
#define NODESFILE1 "./outcitta.txt"
/** file scrittura mappa toscana: file archi */
#define ARCSFILE1  "./outstrade.map"
/** terminatore file di controllo test degree() */
#define END  (-1)

/** nodi primo grafo di test */
static char* citta[] = {
    "PISA",
    "LUCCA",
    "SIENA",
    "LIVORNO",
    "LA SPEZIA",
    "GROSSETO",
    "AREZZO",
    "PRATO",
    "FIRENZE",
    NULL,
};

/** archi corretti primo grafo di test */
static char* strade[] = {
  "PISA:LUCCA:24.8",
  "LUCCA:PISA:24.8",
  "PISA:LIVORNO:22.3",
  "LIVORNO:PISA:22.3",
  "LA SPEZIA:PISA:80.0",
  "SIENA:GROSSETO:90.6",
  "FIRENZE:AREZZO:77.9",
  "AREZZO:FIRENZE:77.9",
  "PRATO:LUCCA:57.9",
  "SIENA:FIRENZE:56.1",
  "FIRENZE:PRATO:24.9",
  "PRATO:FIRENZE:24.9",
  NULL,
};

/** archi errati primo grafo di test */
static char* stradeerr[] = {
  "PISA:COSENZA:24.8",
  "PISA:COSENZA:CICCIOBELLO",
  "LUCCA:PISA:",
  "PISA",
  NULL,
};

/** degree nodi corretto primo grafo di test */
static int deg[] = {
    2,
    1,
    2,
    1,
    1,
    0,
    1,
    2,
    2,
    END,
};

/** main function */
int main (void) {

  graph_t *gmap,*gmap1;
    FILE* fnode, *farc; 
    int i;
    int n1,n2;
    char** p;

    mtrace();

    /* test creazione */
    /* creazione grafo con una sola citta*/
    if ( ( gmap = new_graph(1,citta) ) == NULL )  {
      perror("new_graph 1");
      exit(EXIT_FAILURE);
    }

    /* stampa */
    print_graph(gmap);

    /* deallocazione grafo */
    free_graph(&gmap);

    /* puntatore annullato correttamente ? */
    if ( gmap != NULL )  {
      perror("free_graph");
      exit(EXIT_FAILURE);
    }


    /* alcune invocazioni errate */
    if ( new_graph(0,citta) != NULL )  {
      fprintf(stderr,"new_graph 2: errore\n");
      exit(EXIT_FAILURE);
    }
    if ( new_graph(2,NULL) != NULL )  {
      fprintf(stderr,"new_graph 3: errore\n");
      exit(EXIT_FAILURE);
    }

    free_graph(&gmap);
    free_graph(NULL);

    print_graph(NULL);

    /* creazione grafo con label da citta' */
    /* contiamo le citta*/
    for(p=citta,i=0; *p!=NULL; p++,i++);

    /* i e' il numero delle citta */
    gmap = new_graph(i,citta);

    /* stampa del grafo creato */
    print_graph(gmap);

    /* il numero di nodi e' corretto nel grafo creato ? */
    if ( n_size(gmap) != i ) {
      fprintf(stderr, "n_size 1: errore\n");
      exit(EXIT_FAILURE);
    }

    /* invocazione errata */
    if ( n_size(NULL) != -1 ) {
      fprintf(stderr, "n_size 2: errore\n");
      exit(EXIT_FAILURE);
    }

    /* inseriamo gli archi in strade */
    for(p=strade; *p!=NULL; p++){
      if (  add_edge(gmap,*p) == -1 ) {
	fprintf(stderr,"adding edge --%s--",*p);
	perror(" ");
	exit(EXIT_FAILURE);
      }
    }

    /* inseriamo archi duplicati ... */
    for(p=strade; *p!=NULL; p++){
      if (  add_edge(gmap,*p) == 0 ) {
	fprintf(stderr,"adding duplicate edge --%s--",*p);
	perror(" ");
	exit(EXIT_FAILURE);
      }
    }

    /* controlliamo il degree ... */
    for(p=citta,i=0; *p!=NULL; p++,i++){
      if (  degree(gmap,*p) != deg[i] ) {
	fprintf(stderr,"adding duplicate edge --%s--",*p);
	perror(" ");
	exit(EXIT_FAILURE);
      }
    }

    /* tento di inserire alcuni archi malformati */
    for(p=stradeerr; *p!=NULL; p++){
      if (  add_edge(gmap,*p) == 0 ) {
	fprintf(stderr,"add_edge : sto inserendo l'arco malformato --%s--",*p);
	perror(" ");
	exit(EXIT_FAILURE);
      }
    }
    

    /* alcune invocazioni errate */
    if (  add_edge(NULL,*p) == 0 ) {
      fprintf(stderr,"add_edge : errore\n");
      exit(EXIT_FAILURE);
    }

    if ( degree(NULL,NULL) != -1 ) {
      fprintf(stderr,"degree : errore\n");
      exit(EXIT_FAILURE);
    }

    /* contiamo le strade*/
    for(p=strade,i=0; *p!=NULL; p++,i++);

    /* i e' il numero delle strade presenti */
    /* controllimo se coincidono */
    if ( e_size(gmap) != i ) {
      perror("e_size");
      exit(EXIT_FAILURE);
    }

    /* invocazione errata */
    if ( e_size(NULL) != -1 ) {
      fprintf(stderr, "e_size : errore\n");
      exit(EXIT_FAILURE);
    }
    
    /* controlliamo is_edge e is_node */
    n1 = is_node(gmap,*citta);
    n2 = is_node(gmap,*(citta+1));
    if ( n1 < 0 || n2 < 0 ) {
      perror("is_node");
      exit(EXIT_FAILURE);
    }
    
    if ( ! is_edge(gmap,n1,n2) ) {
      perror("is_edge");
      exit(EXIT_FAILURE);
    }
    

    /* invocazione errata */
    if ( is_node(NULL,*citta) != -1 ) {
      fprintf(stderr, "n_size 2: errore\n");
      exit(EXIT_FAILURE);
    }

    /* stampa del grafo */
    print_graph(gmap);

    /* controlliamo la copia ... */
    if ( ( gmap1 = copy_graph(gmap) ) == NULL ) {
      fprintf(stderr, "copy_graph : errore\n");
      exit(EXIT_FAILURE);
    }

    /* libera la memoria */
    free_graph(&gmap);

    /* stampa della copia del grafo */
    print_graph(gmap1);

    /* libera la memoria della copia*/
    free_graph(&gmap1);


    /* test della load/store su file */
    /* apertura dei file */
    if ( (fnode = fopen(NODESFILE,"r") ) == NULL ) {
      perror("fopen");
      exit(EXIT_FAILURE);
    }
    if ( (farc = fopen(ARCSFILE,"r") ) == NULL ) {
      perror("fopen");
      exit(EXIT_FAILURE);
    }
    
    /* invocazione errata */
    if ( load_graph(NULL,farc) != NULL ) {
      fprintf(stderr, "load_graph: errore\n");
      exit(EXIT_FAILURE);
    }

    /* crea il grafo leggendo da file  */
    if ( ( gmap=load_graph(fnode,farc) ) == NULL ) {
      perror("load_graph");
      exit(EXIT_FAILURE);
    }

    /* chiusura dei file */
    (void)fclose(fnode);
    (void)fclose(farc);
    
    /* stampa del grafo letto */
    print_graph(gmap);

    /* apertura dei file per la scrittura */
    if ( (fnode = fopen(NODESFILE1,"w") ) == NULL ) {
      perror("fopen");
      exit(EXIT_FAILURE);
    }
    if ( (farc = fopen(ARCSFILE1,"w") ) == NULL ) {
      perror("fopen");
      exit(EXIT_FAILURE);
    }

    /* scrive il grafo sui file  */
    if ( save_graph(fnode,farc,gmap) == -1 ) {
      perror("load_graph");
      exit(EXIT_FAILURE);
    }

    /* invocazione errata */
    if ( save_graph(fnode,NULL,gmap) != -1 || save_graph(NULL,farc,NULL) != -1  ){
      fprintf(stderr, "save_graph: errore\n");
      exit(EXIT_FAILURE);
    }


    /* chiusura dei file */
    (void)fclose(fnode);
    (void)fclose(farc);

    free_graph(&gmap);

    muntrace();

    exit(EXIT_SUCCESS);
}
