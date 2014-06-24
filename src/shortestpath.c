   /** \file shortestpath.c
       \author Nicola Corti
       \brief Implementazione delle funzioni definite in shortestpath.h

     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

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
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "shortestpath.h"
#include "macros.h"
#include "heap.h"


/** Funzione static che effettua un confronto fra 2 valori, considerando il valore INFINITY come infinito
 *
 * 	\param [in] a primo valore
 * 	\param [in] b secondo valore
 *
 * 	\retval 0 se a > di b
 * 	\retval un valore di verso da 0 negli altri casi
 * */
static int is_less(double a, double b){

	int result = 0;

	if (a != INFINITY)
	{
		if (b != INFINITY)
			result = (a < b);
		else
			result = 1;
	}
	return result;
}


double * dijkstra (graph_t* g, unsigned int source, unsigned int** pprec){

	heap_array_t * coda = NULL;
	edge_t * forw_star = NULL;
	double * dist = NULL;
	unsigned int * prec = NULL;

	double new_dist;	
	unsigned int size, i, current, next_node;
	/* Flag per l'interruzione del flusso di esecuzione in caso di memoria insufficiente
	 * Vedi macros.h per una descrizione piu' dettagliata */
	unsigned int mem_flag = 0;


	if (g == NULL || (source >= n_size(g)))
	{
		errno = EINVAL;
		return NULL;
	}

	size = n_size(g);
	
	MALLOC_ERRNO(dist, size, double, mem_flag, 0, 1)
	MALLOC_ERRNO(prec, size, int, mem_flag, 0, 1)
	MALLOC_ERRNO(coda, 1, heap_array_t, mem_flag, 0, 1)
	MALLOC_ERRNO(coda->array, size, heap_elem_t, mem_flag, 0, 1)
	
	if (!mem_flag)
	{
		/* Inizializzazione dell'algoritmo di Dijkstra */
		coda -> heap_size = 0;
		for(i = 0; i < size; i++)
		{
			prec[i] = UNDEF;
			dist[i] = INFINITY;
		}
		dist[source] = 0;
		enqueue(coda, source, 0);
		
		while(!is_empty(coda))
		{
			current = dequeue(coda);
			forw_star = g->node[current].adj;
			
			/* Analisi della stella uscente del nodo */
			while(forw_star != NULL)
			{
				new_dist = dist[current] + forw_star->km;
				next_node = forw_star -> label;
				
				if(is_less(new_dist, dist[next_node]))
				{
					/* Aggiornamento della distanza, del predecessore e della coda di priorita' */
					dist[next_node] = new_dist;
					prec[next_node] = current;
					
					if(decrease_key(coda, next_node, new_dist) == 0)
						enqueue(coda, next_node, new_dist);
				}
				forw_star = forw_star -> next;
			}
		}
		free(coda->array);		
		
		if(pprec != NULL)
			*pprec = prec;
		else
			free(prec);
	}
	else
	{
		free(dist);
		free(prec);
		dist = NULL;		
	}
	free(coda);			
	
	return dist;
}


char * shpath_to_string(graph_t * g, unsigned int n1, unsigned int n2,  unsigned int * prec){

	unsigned int * succ = NULL;
	char * output = NULL;

	unsigned int i = 0, size;
	/* Flag per l'interruzione del flusso di esecuzione in caso di memoria insufficiente
	 * Vedi macros.h per una descrizione piu' dettagliata */
	unsigned int mem_flag = 0;

	if ((g == NULL) || (prec == NULL) || (n1 >= n_size(g)) || (n2 >= n_size(g)))
	{
		errno = EINVAL;
		return NULL;
	}

	size = n_size(g);
	MALLOC_ERRNO(succ, size, unsigned int, mem_flag, 0, 1) 
	MALLOC_ERRNO(output, (size * (LLABEL+1) + 1), char, mem_flag, 0, 1)
	if(!mem_flag)
	{
		/* Creazione dell'array dei successori */
		for(i = 0; i < size; i++) succ[i] = UNDEF;
					
		if (prec[n2] != UNDEF) succ[prec[n2]] = n2;
		i = prec[n2];
		
		while(i != n1 && i != n2 && i != UNDEF)
		{
			succ[prec[i]] = i;
			i = prec[i];				
		}
		
		if (i == n1)
		{
			/* Creazione della stringa dell'output */
			output = strcpy(output,g->node[n1].label);
		  while(i != n2)
			{
				i = succ[i];
				output = strcat(output, "$");
				output = strcat(output, g->node[i].label);
			}
 		}
		else
		{
			errno = 0;
			free(output);
			output = NULL;
		}
	}
	free(succ);
	

	return output;
}

