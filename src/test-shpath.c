/**
  \file 
  \brief test shortest path

 \author lso11 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <mcheck.h>
#include "dgraph.h"
#include "shortestpath.h"

/** file dati mappa toscana: file nodi */
#define NODESFILE "./CITY.txt"
/** file dati mappa toscana: file archi */
#define ARCSFILE  "./TUSCANY.map"


/** nodi primo grafo di test */
static char* citta[] = {
    "PISA",
    "LUCCA",
    "LIVORNO",
    "LA SPEZIA",
    "AREZZO",
    "PRATO",
    "FIRENZE",
    NULL,
};

/** archi primo grafo di test */
static char* strade[] = {
  "PRATO:FIRENZE:24.9",
  "PISA:LUCCA:24.8",
  "LUCCA:PISA:24.8",
  "PISA:LIVORNO:22.3",
  "LIVORNO:PISA:22.3",
  "LA SPEZIA:PISA:80.0",
  "PISA:LA SPEZIA:80.0",
  "FIRENZE:AREZZO:77.9",
  "AREZZO:FIRENZE:77.9",
  "PRATO:LUCCA:57.9",
  "LUCCA:PRATO:57.9",
  "FIRENZE:PRATO:24.9",
  NULL,
};

/** stampa formattata del vettore delle distanze calcolato da dijkstra()
   \param g grafo di riferimento
   \param n nodo di partenza
   \param dist vettore delle distanze in km
*/
static void stampadist (graph_t* g, int n, double* dist) {
  int i;

  if ( g == NULL || dist == NULL || n >= g -> size ) {
    fprintf(stderr,"stampadist: Parametri non validi.\n");
    return;
  }
  fprintf(stdout,"dist %d ::",n);
  for(i=0;i<g->size;i++){
    if ( dist[i] != INFINITY) fprintf(stdout," %6.2f ",dist[i]);
    else fprintf(stdout,"  INFIN ");
  }
  putchar('\n');
}

/** stampa formattata del vettore delle precedenze calcolato da dijkstra()
   \param g grafo di riferimento
   \param n nodo di partenza
   \param prec vettore delle precedenze 
*/
static void stampaprec (graph_t * g, int n, unsigned int* prec) {
  int i;

  if ( g == NULL || prec == NULL || n >= g -> size ) {
    fprintf(stderr,"stampaprec: Parametri non validi.\n");
    return;
  }

  fprintf(stdout,"prec %d ::",n);
  for(i=0;i<g->size;i++){
    if ( prec[i] != UNDEF) fprintf(stdout," %6u ",prec[i]);
    else fprintf(stdout,"  UNDEF ");
  }
  putchar('\n');
}

/** stampa in formato stringa le rotte da n1 a tutti gli altri nodi del grafo 
   \param g grafo di riferimento
   \param n1 nodo di partenza delle rotte
   \param prec vettore delle precedenze calcolato da dijkstra

 */
static void calcola_e_stampa_rotte (graph_t * g, unsigned int n1,unsigned int * prec) {      
  int j;

  if ( g == NULL || prec == NULL || n1 >= g -> size ) {
    fprintf(stderr,"calcola_e_stampa_rotte: Parametri non validi.\n");
    return;
  }

  for ( j = 0; j < g -> size; j ++ ) {
    char * p;
    if ( n1 == j ) 
      continue;
    if ( ( p=shpath_to_string(g,n1,j,prec) ) != NULL ){
      fprintf(stdout,"%s\n",p);
      free(p);
    }
    else {
      fprintf(stdout,"Rotta inesistente\n");
    }
  }
}

/** main function */
int main (void) {

  /* grafo di test */
  graph_t *gmap;
  /* numero citta' nel primo grafo di test */
  int n_citta;
  FILE* fnode, *farc; 
  int i;
  char** p;
  double * dist=NULL;
  unsigned int * prec=NULL;

  mtrace();
  
  /* creazione primo grafo */
  /* contiamo le citta*/
  for(p=citta,i=0; *p!=NULL; p++,i++);
  
  
  /* i e' il numero delle citta */
  n_citta = i;
  gmap = new_graph(n_citta,citta);
  
  /* inseriamo gli archi in strade */
  for(p=strade; *p!=NULL; p++){
    if (  add_edge(gmap,*p) == -1 ) {
      fprintf(stderr,"adding edge --%s--",*p);
      perror(" ");
      exit(EXIT_FAILURE);
    }
  }
  
  /* invoca shortest path (scorretto) */    
  if ( ( dist = dijkstra(NULL,0,NULL) ) != NULL ) {
    perror("dijkstra");
    exit(EXIT_FAILURE);
  }
  
  /* invoca shortest path (corretto) su tutti i nodi */    
  fprintf(stdout,"************out primo test\n");
  for (i = 0; i < gmap -> size ;i++) { 
    if ( ( dist = dijkstra(gmap,i,NULL) ) == NULL ) {
      perror("dijkstra 2");
      exit(EXIT_FAILURE);
    }
    
    /* stampa dist */
    stampadist(gmap,i,dist);
    free(dist);
    
    /* invoca shortest path (con precedenze) */    
    if ( ( dist = dijkstra(gmap,i,&prec) ) == NULL ) {
      perror("dijkstra 3");
      exit(EXIT_FAILURE);
    }
    
    /* stampa prec */
    stampaprec(gmap,i,prec);
    
    free(dist);
    free(prec);
    
  }
  
  /* calcolo stringhe di rotta */
  fprintf(stdout,"************out primo test: rotte\n");
  /* calcola e stampa le rotte fra tutti i nodi */
  for (i = 0; i < gmap -> size ;i++) { 
    if ( ( dist = dijkstra(gmap,i,&prec) ) == NULL ) {
      perror("dijkstra 2");
      exit(EXIT_FAILURE);
    }
    calcola_e_stampa_rotte(gmap,i,prec);
    free(dist);
    free(prec);
  }
  
  /* dealloca il grafo */
  free_graph(&gmap);
  
  /* creazione secondo grafo di test */
  gmap = new_graph(n_citta,citta);
  
  /* inseriamo gli archi in strade -- meno il primo */
  for(p=strade+1; *p!=NULL; p++){
    if (  add_edge(gmap,*p) == -1 ) {
      fprintf(stderr,"adding edge --%s--",*p);
      perror(" ");
      exit(EXIT_FAILURE);
    }
  }
  
  /* invoca shortest path (corretto) su tutti i nodi */    
  fprintf(stdout,"************out secondo test\n");
  for (i = 0; i < gmap -> size ;i++) { 
    
    /* invoca shortest path (corretto) */    
    if ( ( dist = dijkstra(gmap,i,&prec) ) == NULL ) {
      perror("dijkstra 4");
      exit(EXIT_FAILURE);
    }
    
    /* stampa dist e prec */
    stampadist(gmap,i,dist);
    stampaprec(gmap,i,prec);
    free(dist);
    free(prec);
    
  }
  
  /* calcolo stringhe di rotta */
  fprintf(stdout,"************out secondo test: rotte\n");
  for (i = 0; i < gmap->size ;i++) { 
    
    /* invoca shortest path (corretto) */    
    if ( ( dist = dijkstra(gmap,i,&prec) ) == NULL ) {
      perror("dijkstra 4");
      exit(EXIT_FAILURE);
    }
    
    /* calcola e stampa le rotte fra tutti i nodi */
    calcola_e_stampa_rotte(gmap,i,prec);
    
    free(dist);
    free(prec);
    
  }
  /* dealloca il grafo */
  free_graph(&gmap);
  
  
  /* terzo test */
  /* apertura dei file */
  if ( (fnode = fopen(NODESFILE,"r") ) == NULL ) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  if ( (farc = fopen(ARCSFILE,"r") ) == NULL ) {
    perror("fopen");
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
  
  
  /* invoca shortest path (corretto) su tutti i nodi */    
  fprintf(stdout,"************out terzo test\n");
  for (i = 0; i < gmap -> size ;i++) { 
    
    /* invoca shortest path (corretto) */    
    if ( ( dist = dijkstra(gmap,i,&prec) ) == NULL ) {
      perror("dijkstra 5");
      exit(EXIT_FAILURE);
    }
    
    /* stampa dist e prec */
    stampadist(gmap,i,dist);
    stampaprec(gmap,i,prec);
    free(dist);
    free(prec);
    
  }
  
  /* calcolo stringhe di rotta */
  fprintf(stdout,"************out terzo test: rotte\n");
  for (i = 0; i < gmap->size ;i++) { 
    
    /* invoca shortest path (corretto) */    
    if ( ( dist = dijkstra(gmap,i,&prec) ) == NULL ) {
      perror("dijkstra 5");
      exit(EXIT_FAILURE);
    }
    
    /* calcola e stampa le rotte fra tutti i nodi */
    calcola_e_stampa_rotte(gmap,i,prec);
    
    free(dist);
    free(prec);
    
  }
  
  
  /* dealloca il grafo */
  free_graph(&gmap);
  
  muntrace();
  
  exit(EXIT_SUCCESS);
}

