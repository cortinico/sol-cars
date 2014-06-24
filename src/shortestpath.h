/**
  \file 
  \brief definizione dei prototipi per il calcolo dei cammini minimi si grafo diretto

 \author lso11 */

#ifndef __SHORTESTPATH_H
#define __SHORTESTPATH_H
#include <stdio.h>
#include "dgraph.h"
/** infinito -- valore di distanza per l'output dell'algoritmo di Dijkstra */
#define INFINITY (-1.0)
#define UNDEF ((unsigned int) -1)

/** implementa l'algoritmo di Dijkstra per la ricerca di tutti i cammini minimi a patire  
    da un nodo sorgente
    \param g grafo pesato, con parametri non negativi
    \param source nodo da cui calcolare i cammini minimi
    \param pprec puntatore di puntatore: (1) 
           se pprec != da NULL viene assegnato a *pprec 
           il vettore dei predecessori definito come segue:
	   per ogni nodo n1 
	               *pprec[n1] = n2
	   se n2 e' il nodo che precede n1 nel cammino minimo da source a n1
	   (2) se pprec == NULL il vettore non viene assegnato (non e' un errore)


    \retval dist puntatore al vettore delle distanze minime calcolate (se tutto e' andato bene )
    \retval NULL se si \'e verificato un errore, setta errno
*/
double* dijkstra (graph_t* g, unsigned int source, unsigned int** pprec);

/** crea la stringa della rotta da n1 a n2 usando l'array delle precedenze calcolato da dijkstra
    \param g
    \param n1
    \param n2
    \param prec

    \retval rotta puntatore alla stringa che descrive la rotta se tutto e' andato bene
    rotta contiene tutte le citta attraversate separate da '$' n1$citta1$....$cittaN$n2
    ad esempio rotta da PISA ad AREZZO PISA$LUCCA$PRATO$FIRENZE$AREZZO
    \retval NULL se la rotta da n1 a n2 non esiste (errno == 0) e
    se si e' verificato un errore (errno != 0)
 */
char * shpath_to_string(graph_t * g, unsigned int n1, unsigned int n2,  unsigned int * prec);

#endif
