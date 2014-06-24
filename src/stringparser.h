/**
  \file stringparser.h
  \brief Definizione delle funzioni per il parser delle stringhe utilizzate nel grafo

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


#ifndef __STRINGPARSER_H
#define __STRINGPARSER_H

#include "dgraph.h"

/** \brief Utilizzo mediante le espressioni regolari
 *
 * 	Se si setta la macro REG_EXPR_MODE, la correttezza di un arco viene effettuata
 * 	mediante l'esecuzione delle espressioni regolari.
 *
 * 	Tale funzione e' inserita solo a scopo didattico, e pertanto se ne sconsiglia l'uso,
 * 	dato che rende piu' pesante l'elaborazione delle stringhe.
 *
 */
#ifdef REG_EXPR_MODE
       #include <sys/types.h>
       #include <regex.h>
		/** \def REG_PARSER
		 *
		 * 	Questa macro rappresenta l'espressione regolare che viene utilizzata per verificare
		 * 	la correttezza di un arco
		 */
       #define REG_PARSER "^[A-Za-z0-9 ]\\{1,128\\}:[A-Za-z0-9 ]\\{1,128\\}:[0-9]\\{1,31\\}[.]\\{0,1\\}[0-9]*$\0"
#endif



/** verifica se l'array di nody passato, e' un array corretto di etichette
    \param array_size dimensione dell'array
    \param label_array array di etichette

    \retval TRUE se l'array e' un array di etichette valide
    \retval FALSE se l'array non e' un array di etichette valide
*/
bool_t is_correct_labelarray(unsigned int array_size, char ** label_array);


/** verifica se la stringa relativa all'arco e' corretta
    \param e stringa relativa all'arco, del tipo "PARTENZA:ARRIVO:123.456"
    \param origin buffer dove verra' scritta l'etichetta della citta di partenza
    \param destin buffer dove verra' scritta l'etichetta della citta di destinazione    
    \param km puntatore ad una variabile double, dove verra' scritta la distanza

    \retval TRUE se la stringa e' un arco valido
    \retval FALSE se si e' verificato un errore
*/
bool_t str_pars(char * e, char * origin, char * destin, double * km);

#endif
