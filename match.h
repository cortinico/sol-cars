/** \file match.c
    \author Nicola Corti
    \brief File che contiene la gestione delle operazioni su stringhe che contengono il path fra 2 citta'
    indicato come elenco delle citta' separate dal simbolo '$'
    Esempio: PISA$LUCCA$PISTOIA
   
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
#include <stdio.h>
#include <string.h>

/** Restituisce il puntatore alla prossima citta del path
 *
 * \param [in] path stringa contenten il path
 *
 * \retval NULL se il path e' NULL oppure se non vi sono altre citta'
 * \retval il puntatore alla prossima citta'
 */
char * nextCity(char *path);

/** Restituisce il numero delle citta' del path
 *
 * 	\param [in] path la stringa contenente il path
 *
 * 	\retval il numero delle citta'
 * 	\internal Implementata in modo ricorsivo
 */
int countCity(char *path);

/** Restituisce il path ottenuto eseguendo n passi lungo path
 *
 * 	\param [in] path stringa contente il percorso originario
 * 	\param [in] n numero dei passi da eseguire
 *
 * 	\retval puntatore alla stringa ottenuta
 * 	\internal Implementato in modo ricorsivo
 */
char * nextPath(char *path, int n);

/** Indica se le citta' all'inizio dei 2 path sono le stesso
 *
 * \param [in] req il primo path
 * \param [in] off il secondo path
 *
 * \retval 0 se le citta' sono differenti
 * \retval un qualsiasi valore diverso da zero negli altri casi.
 */
int isEqualCity(char *off, char *req);

/** Cerca di trovare un match fra il path req ed off
 * Se riesce a trovarlo, ritorna la lunghezza di tale match, e il numero di citta' nel path offer
 * dal quale inizia il match
 *
 * \param [in] req path che si cerca di associare
 * \param [in] off path su cui si cerca un associazione
 * \param [out] begin numero della citta' da cui partire
 *
 * Vengono trovati match anche parziali per il path req.
 * Nel caso viene indicato il match piu' lungo possibile.
 *
 * \internal l'assunzione precedente e' possibile dato che stiamo lavorando con i cammini minimi
 *
 * \retval la dimensione del match
 */
int cityMatch(char *req, char *off, int *begin);

/** Calcola un upper bound per la dimensione del path generato dal match trovato su
 *  elem, partendo dalla citta' begin, e facendo city_found step. Tale valore e' necessario per
 *  allocare una stringa sufficientemente lunga
 *
 *  Utilizza la funzione isEqualCity
 *  \see isEqualCity
 *
 *  L'upper bound e' calcolato utilizzando LLABEL come valore massimo per la dimensione di un'etichetta
 *
 *  \retval il valore dell'upper bound
 */
int pathLen(char *elem, int begin, int city_found);

/** Restituisce una stringa che contiene il percorso effettuato sul path elem, partendo dalla citta'
 *  begin facendo city_found passi.
 *
 *  \param [in] path il path su cui calcolare il percorso
 *	\param [in] begin la citta' da cui iniziare
 *	\param [in] city_found i passi da fare su tale path
 *	\param [in] size dimensione della stringa da allocare
 *
 *	\attention La stringa viene allocata (malloc), deve essere poi deallocata dall'utente finale.
 *
 *	\retval Il puntatore alla stringa allocata
 */
char *cityString(char *path, int begin, int city_found, int size);

/** Taglia la stringa dopo aver effettuato steps passi
 *
 * 	\param [in, out] path il percorso da taglia
 * 	\param [in] steps il numero di passi da effettuare
 *
 * 	\retval Il puntatore alla seconda parte della stringa tagliata.
 *
 * 	\attention la stringa path risultera' modificata (precisamente tagliata e puntera' alla prima parte
 * 	del path tagliato)
 */
char *cutPath(char *path, int steps);
