/**
  \file macros.h
  \brief Definizione delle macro per le allocazioni e per il controllo dei parametri

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

#ifndef __MACROS_H
#define __MACROS_H

#include <stdlib.h>
#include <errno.h>


/** Effettua l'allocazione di un'area di memoria e restituisce il puntatore.

    \param dest variabile di tipo puntatore, che conterra' il puntatore all'area di memoria
    						allocata; in caso di fallimento, conterra' NULL
    \param elem la dimensione da allocare
    \param number il tipo di elementi da allocare
    \param flag variabile di controllo, che indica l'esito dell'allocazione
    \param correctvalue valore che deve assumere flag, affinche' l'allocazione possa essere effettuata
    \param falsevalue valore che assumera' flag in caso di fallimento dell'allocazione
    
    Si veda la nota a pie' di pagina
    
    In caso di fallimento dell'allocazione, viene settato il valore della variabile errno a ENOMEM

*/
#define MALLOC_ERRNO(dest, number, elem, flag, correctvalue, falsevalue)\
 if ((flag) == (correctvalue))\
	{ \
		if(((dest) = malloc((number) * sizeof(elem))) == NULL)\
	  	 {errno = ENOMEM; (flag) = (falsevalue);}\
  }
																															 	 
							

  
/* ====== NOTA ====== 
	
	Per convenzione, la variabile flag per la MALLOC_ERRNO si chiamera' "mem_flag"
	
	Come valore correct consideriamo 0, e come falsevalue consideriamo 1

	Questo al fine di agevolare la leggibilita' del codice
*/
  
  
#endif
