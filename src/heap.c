/**
  \file heap.c
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

#include "heap.h"
#include <stdlib.h>

/** Funzione static per ricavare l'indice del padre nella coda, a partire dall'indice del figlio
 *
 * \param i [in] indice del nodo
 *  */
static unsigned int padre(int i){
  return (i-1)/2;
}


/** Funzione static per ricavare l'indice del figlio nella coda, a partire dall'indice del padre
 *
 * \param i [in] indice del nodo
 * */
static unsigned int sinistro(int i){
  return ((2*i)+1);
}


/** Funzione static per scambiare due elementi nella coda di priorita'
 *
 * \param [in, out] x primo elemento
 * \param [in, out] y secondo elemento
 *  */
static void scambia(heap_elem_t * x, heap_elem_t * y){

  heap_elem_t temp_elem;	
	
	temp_elem = *x;
	*x = * y;
	*y = temp_elem;
}


/** Funzione static per cercare il minimo fra il padre ed i figli all'interno della coda di priorita'
 *
 * \param [in] coda la coda di priorita
 * \param [in] padre indice del padre
 *
 * \retval l'indice del minimo fra il padre ed i 2 figli
 *
 *  */
static unsigned int min_padre_figli(heap_array_t * coda, unsigned int padre){

	unsigned int sx = padre, dx;
	
	if (coda != NULL)
	{
		sx = dx = sinistro(padre);
		if (dx + 1 < coda->heap_size) dx++;
		if ((coda->array[dx].dist < coda->array[sx].dist)) sx = dx;
		if ((coda->array[padre].dist < coda->array[sx].dist)) sx = padre;
	}
	return sx;
}


/** Funzione per riorganizzare la coda di priorita' dopo che sono state effettuate delle modifiche sulla stessa
 *
 * \param [in, out] coda la coda di priorita'
 * \param [in] i indice che indica rispetto a quale nodo deve essere effettuata la riorganizzazione
 * */
void reorganize_heap(heap_array_t * coda, unsigned int i){

	unsigned int size, select;
	size = coda->heap_size;
	
	/* Controlla se ci sono degli elementi da far salire */
	while(i>0 && (coda->array[i].dist < coda->array[padre(i)].dist))
	{
		scambia(&coda->array[i],&coda->array[padre(i)]);
		i = padre(i);
	}
	
	/* Controlla se ci sono degli elementi da far scendere */
	while(sinistro(i) < size && i != min_padre_figli(coda, i))
	{
		select = min_padre_figli(coda, i);
		scambia(&coda->array[i], &coda->array[select]);
		i = select;	
	}
}


int is_empty(heap_array_t * coda){
	
	unsigned int result = -1;
	
	if (coda != NULL)
		result = !coda->heap_size;
				
	return result;
}


void enqueue(heap_array_t * coda, unsigned int elem, double dist){

	/* Inserisce l'elemento in fondo alla coda, e la riorganizza */
	unsigned int size;
  if (coda != NULL)
  {
	  size = coda->heap_size;
	
	  coda->array[size].label = elem;
	  coda->array[size].dist = dist;
	  
	  (coda->heap_size)++;
	  
	  reorganize_heap(coda, size);
	}
}

int decrease_key(heap_array_t * coda, unsigned int elem, double new_dist){

	unsigned int size, i = 0, result = 0; 
	
	if (coda != NULL && new_dist >= 0)
	{
	
	  size = coda->heap_size;
	  while(!result && i < size)
	  {
		  if (coda->array[i].label == elem)
		  {
		  	/* Aggiornamento del peso, e riorganizzazione dell'heap */
			  coda->array[i].dist = new_dist;
			  result = 1;
			  reorganize_heap(coda, i);
		  }
		  i++;
	  }
  }
	return result;
}

int dequeue(heap_array_t * coda){

  int result = -1;  
  
	unsigned int size;
	if (coda != NULL)
	{
	  size = coda->heap_size;
	  if (size)
	  {
	  	/* Restituisco l'elemento in testa, e riorganizza l'heap */
	    result = coda->array[0].label;
	    scambia(&coda->array[0],&coda->array[size-1]);
	    (coda->heap_size)--;
	
	    reorganize_heap(coda, 0);
	  }
	}
	return result;
}
