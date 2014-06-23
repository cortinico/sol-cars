/**
  \file heap.h
  \brief Definizione delle funzioni per la gestione della coda di priorita' implementata con un heap binario di minimo.

  \author Nicola Corti
  
  Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore. */
/*    Copyright (C) 2011  Nicola Corti

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

		 For further information, Nicola Corti - <cortin [at] cli.di.unipi.it>
*/


#ifndef __HEAP_H
#define __HEAP_H


/** Elemento dell'heap array */
typedef struct heap_elem{

	/** Etichetta dell'elemento */
	unsigned int label;
	
	/** Peso in km dell'elemento */
	double dist;
	
} heap_elem_t;

/** Heap binario di minimo */
typedef struct heap_array{

	/** Numero degli elementi attualmente presenti nella coda di priorita' */
	unsigned int heap_size;
	
	/** Puntatore all'heap array */
	heap_elem_t * array;
	
} heap_array_t;



/** verifica se la coda di priorita' e' vuota
    \param [in] coda puntatore alla coda di priorita'

    \retval 0 se la coda e' vuota
    \retval n un intero diverso da 0 se la coda non e' vuota
*/
int is_empty(heap_array_t * coda);


/** Inserisce un elemento nella coda di priorita'
    \param [in, out] coda puntatore alla coda di priorita'
    \param [in] elem indice dell'elemento nel grafo
    \param [in] dist peso dell'elemento
*/
void enqueue(heap_array_t * coda, unsigned int elem, double dist);


/** Se un elemento e' presente nella coda di priorita', diminuisce il suo perso
    \param [in, out] coda puntatore alla coda di priorita'
    \param [in] elem indice dell'elemento nel grafo
    \param [in] new_dist nuovo peso da assegnare all'elemento

    \retval 1 se l'elemento e' stato aggiornato
    \retval 0 se l'elemento non e' stato trovato nella coda
*/
int decrease_key(heap_array_t * coda, unsigned int elem, double new_dist);


/** Rimuove l'elemento di peso minimo dalla coda di priorita'
    \param [in, out] coda puntatore alla coda di priorita'
    
    \retval i l'indice nel grafo dell'elemento estratto
*/
int dequeue(heap_array_t * coda);

#endif
