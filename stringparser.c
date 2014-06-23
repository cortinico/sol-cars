/**
  \file stringparser.c
  \author Nicola Corti
  \brief Implementazione delle funzioni definite nell'header file stringparser.h
  
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

#define _POSIX_SOURCE
/* Macro definita per implementare la versione POSIX compatibile di strtok_r */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "stringparser.h"
#include "macros.h"
#include "dgraph.h"







/** Funzione static che controlla se la stringa passata e' un double
 *
 * \param [in] label la stringa da controllare
 *
 * \retval TRUE se la stringa e' un double
 * \retval FALSE altrimenti
 * */
static bool_t is_double(char * label){

	unsigned int dotcount = 0, i = 0;
	bool_t result = TRUE;
	
	if (label == NULL)
		return FALSE;
	
	result = result && (strlen(label) <= LKM);
	while(result && label[i] != '\0')
	{
		if (label[i] == '.') 
		  /* Variabile dotcount, per contare quante volte appare il . */
			dotcount++;
		else
			result = result && (isdigit(label[i]));
		i++;
	}
	
	return (result && (dotcount < 2));
}


/** Funzione static che controlla se una label contiene solo carattere alfanumerici o spazi
 *
 * \param [in] label la stringa da controllare
 *
 * \retval TRUE se la stringa e' un double
 * \retval FALSE altrimenti
 * */
static bool_t is_correct_label(char * label){
	
	bool_t result=TRUE;
	unsigned int i = 0;

	if (label == NULL)
		return FALSE;
	
	while(result && label[i] != '\0')
	{
		result = result && (isalnum(label[i]) || label[i] == ' ');
		i++;
	}
	
	return (result && (i <= LLABEL));
}


bool_t is_correct_labelarray(unsigned int array_size, char ** label_array){

	bool_t result = TRUE;	
	unsigned int i = 0;
	
	if (label_array == NULL)
		return FALSE;
	

	while(result && i < array_size)
	{
		result = result && is_correct_label(label_array[i]);
		i++;
	}
	
	return (result && (i == array_size));
}


bool_t str_pars(char * e, char * origin, char * destin, double * km){
	
	char * temp_str1 = NULL, * temp_str2 = NULL, * temp_str3 = NULL;
	bool_t result = FALSE;
	#ifdef REG_EXPR_MODE
		/* Variabili necessarie per l'esecuzione con le espressioni regolari */
		regex_t string_verif;
		int reg_result;
	#endif
	
	unsigned int mem_flag = 0;
	
	/* Utilizzato da strtok: copia del parametro */
	char * temp_e = NULL;
	/* Utilizzato da strtok: buffer per l'esecuzione */
	char * buffer_e = NULL;
	/* Utilizzato da strtok: puntatore alla testa del buffer, per poterlo deallocare */
  char *head_buffer_e = NULL;
	
	if (e == NULL || origin == NULL || destin == NULL || km == NULL)
		return FALSE;
	
	MALLOC_ERRNO(temp_e, (LLABEL*3), char, mem_flag, 0, 1)
	MALLOC_ERRNO(buffer_e, (LLABEL + 1), char, mem_flag, 0, 1)
  head_buffer_e = buffer_e;
  	
	if(!mem_flag)
	{
	  strcpy(temp_e, e);
	  
	  #ifdef REG_EXPR_MODE
	  /* -------------------------
	   * CASO ESPRESSIONI REGOLARI
	   * -------------------------
	   */
	  /* Compilazione dell'espressione regolare */
	  regcomp(&string_verif, REG_PARSER, 0);
	  /* Esecuzione dell'espressione regolare */
	  reg_result = regexec(&string_verif,e,0,NULL,0);
	  /* Deallocazione della struttura usata */
	  regfree(&string_verif);
	  if (result == 0)
	  {
	  #else	

	  /* -------------------------
	   * CASO CLASSICO
	   * -------------------------
	   */
	  /* Ottengo i puntatori alle occorrenze di ":" */
	  temp_str1 = strchr(e, ':');
	  if(temp_str1 != NULL)	temp_str2 = strchr(temp_str1 + 1,':');
	  if(temp_str2 != NULL) temp_str3 = strchr(temp_str2 + 1,':');

	  /* Controllo che le occorrenze di ":" siano soltanto 2, e che non siano adiacenti (vi siano dei caratteri nel mezzo) */
		if((temp_str1) && (temp_str2) && (temp_str1 != e) && (temp_str2 != temp_str1+1) && (strlen(temp_str2+1) > 0) && (temp_str3 == NULL))
		{
		#endif
		  /* Separo la stringa in token, utilizzando la funzione strtok_r,
		   *  una versione rientrante della strtok, ed e' thread-safe */
		  temp_str1 = strtok_r(temp_e, ":", &buffer_e);
			temp_str2 = strtok_r(NULL, ":", &buffer_e);
			temp_str3 = strrchr(e, ':');
			temp_str3++;
			
			/* Controllo che le 2 label siano corrette e che la distanza sia un double */
			if(is_correct_label(temp_str1) && is_correct_label(temp_str2) && is_double(temp_str3))
			{
				origin = strcpy(origin, temp_str1);
				destin = strcpy(destin, temp_str2);
				*km = strtod(temp_str3, NULL);
				result = TRUE;
			}
		}
	}

  free(temp_e);
	free(head_buffer_e);

	return result;	
}
