/**
  \file 
  \brief definizione dei prototipi del grafo orientato rappresentato come array 0..N-1

 \author lso11 */

#ifndef __DGRAPH_H
#define __DGRAPH_H
#include <stdio.h>

/** tipo enumerato booleano vero/falso */
typedef enum bool {TRUE=1, FALSE=0} bool_t;



/** elemento lista di adiacenza */
typedef struct edge {
  /** etichetta nodo adiacente (indice nell'array) */
  unsigned int label;     
  /** peso dell'arco (distanza in km) */
  double km;      
  /** prossima adiacenza*/
  struct edge * next;    
} edge_t;

/** lunghezza massima label -- in caratteri */
#define LLABEL 128
/** lunghezza massima distanza in km -- in caratteri */
#define LKM 32

/** nodo del grafo */
typedef struct node {
/** etichetta informativa (nome citta) */
    char* label;      
/** lista di adiacenza */
    edge_t * adj;          
} node_t;

/** Grafo non-orientato rappresentato come array 0 .. (N-1) di vertici
*/
typedef struct graph {
  /** array dei  nodi */
  node_t * node;  
  /** numero nodi */
  unsigned int size;      
} graph_t;



/** crea un grafo 
    \param n numero dei nodi del grafo
    \param lbl array di etichette de nodi formato:
    static const char* citta[] = {
        "PISA",
        "LUCCA",
        "SIENA",
        "LIVORNO",
        NULL,
       };

    Attenzione: le citta possono contenere solo caratteri alfanumerici e lo spazio ' '

    \retval p puntatore al nuovo grafo (se tutto e' andato bene)
    \retval NULL se si e' verificato un errore (setta errno)
    (errno == EINTR se i parametri non sono validi es lbl a NULL)
    (errno == ENOMEM se l'allocazione e' fallita)
*/
graph_t * new_graph (unsigned int n, char **lbl);

/** distrugge un grafo deallocando tutta la sua memoria 
    \param g puntatore al puntatore al grafo da deallocare -- viene messo a NULL dalla funzione dopo la deallocazione
*/
void free_graph (graph_t** g);

/** stampa il grafo su stdout (formato arbitrario a caratteri)
    \param g puntatore al grafo da stampare
*/
void print_graph (graph_t* g);

/** crea una copia completa del grafo (allocando tutta la memoria necessaria)
    \param g puntatore al grafo da copiare

    \retval p puntatore al nuovo grafo (se tutto e' andato bene)
    \retval NULL se si \e verificato un errore -- setta errno
    errno == ENOMEM se l'allocazione e' fallita
    errno == EINVAL se i parametri sono inconsistenti (esempio, g NULL) */
graph_t* copy_graph (graph_t* g);

/** aggiunge un arco al grafo (senza ripetizioni, ogni arco deve connettere una diversa coppia sorgente-destinazione)
    \param g puntatore al grafo
    \param e arco, stringa di formato
       LBLSORGENTE:LBLDESTINAZIONE:DISTANZA_IN_KM                                                                           
       Esempio: FIRENZE:PRATO:20.4 

    Attenzione: le citta possono contenere solo caratteri alfanumerici e lo spazio ' ', la distanza e' un numero reale, 


    \retval 0 se l'inserzione e' avvenuta con successo
    \retval -1 se si e' verificato un errore (setta errno), 
            in questo caso il grafo originario non deve essere modificato
            errno == ENOMEM se l'allocazione e' fallita
	    errno == EINVAL se l'arco e' inconsistente (esempio, il nodo sorgente/destinazione non esiste) */
int add_edge (graph_t * g, char* e);

/** verifica l'esistenza di un nodo
    \param g puntatore al grafo
    \param ss label del nodo


    \retval n l'indice nel grafo (0..(n_size -1)) se il nodo esiste
    \retval -1 altrimenti
*/
int is_node(graph_t* g, char* ss);

/** verifica l'esistenza di un arco
    \param g puntatore al grafo
    \param n1 etichetta nodo primo estremo
    \param n2 etichetta nodo secondo estremo


    \retval TRUE se l'arco esiste
    \retval FALSE altrimenti
*/
bool_t is_edge(graph_t* g, unsigned int n1, unsigned int n2);

/**  grado di un nodo
    \param g puntatore al grafo
    \param lbl la label del nodo

    \retval n il numero di archi uscenti dal nodo 
    \retval -1 se si e' verificato un errore (setta errno )*/
int degree(graph_t * g, char* lbl);  

/** numero di nodi di un grafo
    \param g puntatore al grafo

    \retval n il numero di nodi di g 
    \retval -1 se si e' verificato un errore (setta errno)
*/
int n_size(graph_t* g); 


/** numero di archi un grafo
    \param g puntatore al grafo

    \retval n il numero di archi di g 
    \retval -1 se si e' verificato un errore (setta errno)
*/
int e_size(graph_t* g);

/** carica il grafo da 2 file, il primo contiene le label possibili de nodi, ad esempio:
    PISA
    FIRENZE
    EMPOLI
    separate da '\n', il secondo contenente gli archi secondo il formato
        LBLSORGENTE:LBLDESTINAZIONE:DISTANZA_IN_KM 
    Esempio: FIRENZE:PRATO:20.4
    sempre separati da '\n'
    
    Attenzione: le citta possono contenere solo caratteri alfanumerici e lo spazio ' ', la distanza e' un numero reale, 
    ci possono essere linee vuote (cioe' che contengono solo '\n')

    \param fdnodes file descriptor del file contenente le label dei nodi
    \param fdarcs file descriptor del file contenente gli archi

    \retval g puntatore al nuovo grafo se il caricamento e' avvenuto con successo
    \retval NULL se si e' verificato un errore (setta errno), es
             errno == ENOMEM se l'allocazione e' fallita
	    errno == EINVAL se i parametri sono inconsistenti (esempio, fd non valido) */
graph_t* load_graph (FILE * fdnodes, FILE * fdarcs);

/** salva il grafo su due file, formato \see load_graph
    
    \param fdnodes file descriptor del file che dovra' contenere i nodi del grafo
    \param fdarcs file descriptor del file che dovra' contenere gli archi del grafo

    \param g grafo da scrivere

    \retval 0 se la scrittura e' andata a buon fine
    \retval -1 se si e' verificato un errore (setta errno), es. errno == EIO se la scrittura e' fallita
*/
int save_graph (FILE * fdnodes, FILE * fdarcs, graph_t* g);

#endif
