/** \file dgraph.c
   \author Nicola Corti
   
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

#include "dgraph.h"
#include "macros.h"
#include "stringparser.h"


graph_t * new_graph(unsigned int n, char **lbl){
	
	graph_t * n_graph = NULL;
	node_t * n_node = NULL;
	
	unsigned int label_len=0, i = 0;

	/* Flag per l'interruzione del flusso di esecuzione in caso di memoria insufficiente
	 * Vedi macros.h per una descrizione piu' dettagliata */
	unsigned int mem_flag = 0;
	
	
	if(lbl == NULL || n == 0 || (is_correct_labelarray(n, lbl)) == FALSE)
	{
		errno = EINVAL;
		return n_graph;
	}
	
	MALLOC_ERRNO(n_graph, 1, graph_t, mem_flag, 0, 1)
	MALLOC_ERRNO(n_graph -> node, n, node_t, mem_flag, 0, 1)
  if(!mem_flag)
  {
		n_graph -> size = n;	
		n_node = n_graph -> node;
		
		/* Copio le etichette, o metto a NULL le etichette se ho avuto un errore nell'allocazione
		   (cosi' in caso di errore posso chiamare tranquillamente free_graph) */
		for(i = 0; i < n; i++)
		{
			n_node[i].adj = NULL;
			
			label_len = strlen(lbl[i]);
			MALLOC_ERRNO(n_node[i].label, (label_len + 1), char, mem_flag, 0, 1)
			
			if(!mem_flag)
			{
				strncpy(n_node[i].label, lbl[i], label_len);
				n_node[i].label[label_len] = '\0';
			}					
			else
				n_node[i].label = NULL;
		}
	}
	
	if(mem_flag && n_graph != NULL)
	  free_graph(&n_graph);

	return n_graph;
}


void free_graph (graph_t** g){

	node_t * free_nodes = NULL;
	edge_t * prec = NULL, * succ = NULL;
	int graph_size = 0, i = 0;

	if(g != NULL && *g != NULL)
	{
		graph_size = n_size(*g);
		free_nodes = (*g) -> node;
		
		if(free_nodes != NULL)
		{
			for(i = 0; i < graph_size; i++)
			{
				succ = free_nodes[i].adj;
				
				/* Free della lista di adiacenza */
				while (succ != NULL)
				{
				  prec = succ;
				  succ = succ -> next;
				  free(prec);
				}
				free(free_nodes[i].label);	
			}
			free(free_nodes);
		}
		free(*g);
		*g = NULL;
	}
}


void print_graph (graph_t* g){

	node_t * print_nodes = NULL;
	edge_t * print_edge = NULL;
	int graph_size = 0, i = 0;
	
	if(g == NULL)
	{
		errno = EINVAL;
		return;
	}
	
	graph_size = n_size(g);
	print_nodes = g -> node;
  
	for(i = 0; i < graph_size; i++)
	{
		printf("%s: \n",print_nodes[i].label);
		
		print_edge = print_nodes[i].adj;
		while(print_edge != NULL)
		{
			printf("\t%s, %.1f\n",print_nodes[print_edge->label].label, print_edge->km);
			print_edge = print_edge -> next;
		}
	}	
}


graph_t* copy_graph (graph_t* g){

	graph_t * cpy_graph = NULL;
	node_t * cpy_node = NULL, * origin_node = NULL;
	edge_t * cpy_edge = NULL, * origin_edge = NULL;
	
	int graph_size = 0, i = 0, label_len = 0;

	/* Flag per l'interruzione del flusso di esecuzione in caso di memoria insufficiente
	 * Vedi macros.h per una descrizione piu' dettagliata */
	unsigned int mem_flag = 0;
	
	if(g == NULL)
	{
		errno = EINVAL;
		return NULL;
	}
			
	MALLOC_ERRNO(cpy_graph, 1, graph_t, mem_flag, 0, 1)
	MALLOC_ERRNO((cpy_graph -> node), n_size(g), node_t, mem_flag, 0, 1)
	
	if(!mem_flag)
	{
		graph_size = n_size(g);
		cpy_graph -> size = graph_size;
	
		cpy_node = cpy_graph -> node;
		origin_node = g -> node;
		
		/* Copia dell'array delle etichette */
		for(i = 0; i < graph_size; i++)
		{
			label_len = strlen(origin_node[i].label);
			cpy_node[i].adj = NULL;
			
			MALLOC_ERRNO(cpy_node[i].label,(label_len+1),char,mem_flag, 0, 1)
			if(!mem_flag)
			{
				strncpy(cpy_node[i].label, origin_node[i].label, label_len);
				cpy_node[i].label[label_len] = '\0';
			}
			else
			  cpy_node[i].label = NULL;
		}
		
		/* Copia delle liste di adiacenza */
		i = 0;
		while(i < graph_size && !mem_flag)
		{
			origin_edge = g->node[i].adj;
			if(origin_edge != NULL)
			{
				MALLOC_ERRNO(cpy_node[i].adj, 1, edge_t, mem_flag, 0, 1);
				if(!mem_flag)
				{
					/* Copia del primo elemento */
					cpy_node[i].adj -> label = origin_edge -> label;
					cpy_node[i].adj -> km = origin_edge -> km;
					cpy_node[i].adj -> next = NULL;
					
					cpy_edge = cpy_node[i].adj;
					origin_edge = origin_edge -> next;
					
					/* Copia della stella uscente */
					while(origin_edge != NULL && !mem_flag)
					{
						MALLOC_ERRNO(cpy_edge->next, 1, edge_t, mem_flag, 0, 1)
						if(!mem_flag)
						{
							cpy_edge = cpy_edge->next;
							cpy_edge->label = origin_edge -> label;
							cpy_edge->km = origin_edge -> km;
							cpy_edge->next = NULL;
							
							origin_edge = origin_edge->next;
						}
					}
				}
			}
			i++;
		}
		
	}
	
	
	/* Deallocazione del nuovo grafo, nel caso ci sia stato un errore di allocazione */
	if (mem_flag)
		free_graph(&cpy_graph);
	
	return cpy_graph;
}


int add_edge (graph_t * g, char* e){
	
	edge_t * new_edge = NULL;
	char * origin = NULL, * destin = NULL;
	
	int result = -1;
	double km;
	unsigned int n1, n2;
	unsigned int mem_flag = 0;
	
	if (g == NULL || e == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	MALLOC_ERRNO(origin, LLABEL, char, mem_flag, 0, 1)
	MALLOC_ERRNO(destin, LLABEL, char, mem_flag, 0, 1)
	
	if(!mem_flag)
	{
		if (str_pars(e, origin, destin, &km) == FALSE)
			errno = EINVAL;
		else
		{
			/* Ricavo gli indici delle 2 etichette, e mi assicuro che l'arco non esista gia' */
			n1 = is_node(g, origin);
			n2 = is_node(g, destin);
			if(is_edge(g,n1,n2) == TRUE || n1 == -1 || n2 == -1 || n1 == n2)
				errno = EINVAL;
			else
			{
				/* Inserimento in testa del nuovo elemento */
				MALLOC_ERRNO(new_edge, 1, edge_t, mem_flag, 0, 1)
				if (!mem_flag)						
				{
					new_edge -> label = n2;
					new_edge -> km = km;
					new_edge -> next = g->node[n1].adj;
					g->node[n1].adj = new_edge;
					result = 0;
				}
			}
		}
	}
	
	free(origin);
	free(destin);
		
	return result;
}


int is_node(graph_t * g, char * ss){

	int result = -1;
	int graph_size = 0, i = 0;
	
	if(g == NULL || ss == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	graph_size = n_size(g);
	while(result == -1 && i < graph_size)
	{
		if (!strcmp(ss,g->node[i].label)) result = i;
		i++;
	}

	return result;
}


bool_t is_edge(graph_t* g, unsigned int n1, unsigned int n2){
		
	bool_t result = FALSE;
	edge_t * scan_edge = NULL;
	int graph_size = 0;
	
	if (g == NULL || (n1 >= n_size(g)) || (n2 >= n_size(g)))
	{
		errno = EINVAL;
		return FALSE;
	}
	
	graph_size = n_size(g);
	
	scan_edge = g->node[n1].adj;
	while(!result && scan_edge != NULL)
	{
		if(scan_edge->label == n2) result = TRUE;
		scan_edge = scan_edge->next;
	}
	return result;
}


int degree(graph_t * g, char* lbl){

	edge_t * degree_edge = NULL;
	int count = -1;
	unsigned int node_ind = 0;
	
	if (g == NULL || lbl == NULL || (is_node(g,lbl)) == -1)
	{
		errno = EINVAL;
		return -1;
	}

	count = 0;
	node_ind = is_node(g,lbl);
	
	degree_edge = g->node[node_ind].adj;
	while(degree_edge != NULL)
	{
		count++;
		degree_edge = degree_edge -> next;
	}	
	
	return count;
}  


int n_size(graph_t* g){

	if (g == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	else
		return (int) g->size;
}


int e_size(graph_t* g){
	
	int count = -1;
	int graph_size = 0, i = 0;
	
	if (g == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	count = 0;
	graph_size = n_size(g);
	
	for(i = 0; i < graph_size; i++)
		count += degree(g, g -> node[i].label);

	
	return count;
}


graph_t* load_graph (FILE * fdnodes, FILE * fdarcs){

	char * temp_label = NULL;
	/* Etichetta temporanea per il caricamento delle stringhe */
	
	char ** elenco = NULL;
	
	char ** temp_elenco = NULL;
	/* Puntatore temporaneo all'elenco delle label, usato per gestire la realloc */

	graph_t * loaded_graph = NULL;

	/* Flag per l'interruzione del flusso di esecuzione in caso di memoria insufficiente
	 * Vedi macros.h per una descrizione piu' dettagliata */
	unsigned int mem_flag = 0;
	unsigned int k;
	int i = 0;
	
	if (fdnodes == NULL || fdarcs == NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	

	MALLOC_ERRNO(elenco, 1, char *, mem_flag, 0, 1)
	MALLOC_ERRNO(temp_label, ((LLABEL * 2) + LKM + 3), char, mem_flag, 0, 1)
	
	
	/* Preparazione dell'elenco delle etichette */
	while(!mem_flag && (fgets(temp_label, LLABEL, fdnodes) != NULL)) 
	{
		if(strcmp(temp_label,"\n"))
		{
			MALLOC_ERRNO(elenco[i], (LLABEL +1), char, mem_flag, 0, 1)
			if(!mem_flag)
			{
				k = strlen(temp_label);
				temp_label[k-1] = '\0';
				strcpy(elenco[i],temp_label);
				i++;
				
				/* Riallocazione dell'elenco delle etichette */
				if((temp_elenco = realloc(elenco,(i+1)*sizeof(char *))) == NULL)
					mem_flag = 1;
				else
				{
					elenco = temp_elenco;
					elenco[i] = NULL;
				}
				
			}
		}
	}		
  
	if (!mem_flag)
	{
		/* Creazione del nuovo grafo */
		if((loaded_graph = new_graph(i,elenco)) != NULL)
		{
			while(!mem_flag && (fgets(temp_label,LLABEL,fdarcs) != NULL))
			{
				if (strcmp(temp_label,"\n"))
				{
					k = strlen(temp_label);
					temp_label[k-1] = '\0';
					
					/* Inserimento degli archi */
					if (add_edge(loaded_graph,temp_label) != 0)
					{
						if (errno == ENOMEM) mem_flag = 1;
						else
						{
							free_graph(&loaded_graph);
							return NULL;
						}
					}
				}
			}
		}		
	}
		
	/* Deallocazione delle strutture utilizzate */
	while(elenco != NULL && i>=0)
	{
	  free(elenco[i]);
	  i--;
	}
	free(elenco);
	free(temp_label);
	
	/* Deallocazione del grafo caricato, nel caso ci sia stato un errore di allocazione */
	if(mem_flag)
	{
	  free_graph(&loaded_graph);
	  errno = ENOMEM;
	}
				
		
	return loaded_graph;
}


int save_graph (FILE * fdnodes, FILE * fdarcs, graph_t* g){

	int result = 0;
	edge_t * save_edge= NULL;
	unsigned int graph_size = 0, i = 0;
	
	if (fdnodes == NULL || fdarcs == NULL || g == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	graph_size = n_size(g);
	while(i < graph_size && !result)
  {
  	/* Scrittura del file delle etichette */
    if((fprintf(fdnodes,"%s\n",g->node[i].label)) < 0)
		{
			result = -1;
			errno = EIO;
		}
		
		/* Scrittura del file degli archi */
		save_edge = g->node[i].adj;
		while(save_edge != NULL && !result)
		{
			if((fprintf(fdarcs,"%s:%s:%.1f\n",g->node[i].label,g->node[save_edge->label].label, save_edge->km)) < 0)
			{
				result = -1;
				errno = EIO;
			}				
			save_edge = save_edge -> next;
		}
		i++;
	}	  
	return result;
}

