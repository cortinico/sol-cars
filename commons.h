/** \file commons.h
    \author Nicola Corti
    \brief File contenente le dichiarazioni di macro condivise fra il server ed il client
   
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
#include "settings.h"

#include <stdlib.h>
#include <stdio.h>

#ifndef __COMMONS_H
#define __COMMONS_H

/* Macro per la gestione degli errori... */

/**
 * \def ec_neg1(s,m)
 *
 * Se il valore ritornano dall'esecuzione di s vale -1, esegue perror con messaggio m ed esce
 * ritornando errno
 */
#define ec_neg1(s,m) \
	if ( (s) == -1 ) {perror(m); exit(errno);}

/**
 * \def ec_null(s,m)
 *
 * Se il valore ritornano dall'esecuzione di s vale NULL, esegue perror con messaggio m ed esce
 * ritornando errno
 */
#define ec_null(s,m) \
	if ( (s) == NULL ) {perror(m); exit(errno);}

/**
 * \def ec_neg1_c(s,m)
 *
 * Se il valore ritornano dall'esecuzione di s vale -1, esegue perror con messaggio m ed esegue
 * il comando/i comandi indicati come C
 */
#define ec_neg1_c(s,m,c) \
	if ( (s) == -1 ) {perror(m); c}

/**
 * \def ec_null_c(s,m)
 *
 * Se il valore ritornano dall'esecuzione di s vale NULL, esegue perror con messaggio m ed esegue
 * il comando/i comandi indicati come C
 */
#define ec_null_c(s,m,c) \
	if ( (s) == NULL ) {perror(m); c}

/**
 * \def ec_zero(s,m)
 *
 * Se il valore ritornano dall'esecuzione di s e' diverso da zero, esegue perror con messaggio m ed esce
 * ritornando errno
 */
#define ec_zero(s, m) \
	if ( (s) != 0) {perror(m); exit(errno);}

/**
 * \def ec_zero_c(s,m)
 *
 * Se il valore ritornano dall'esecuzione di s e' diverso da zero, esegue perror con messaggio m ed esegue
 * il comando/i comandi indicati come C
 */
#define ec_zero_c(s, m, c) \
	if ( (s) != 0) {perror(m); c}

/**
 * \def print(s)
 *
 * Stampa la stringa S, secondo la sintassi del comando printf, e svuota il buffer di standard output
 */
#define print(s) \
	{printf(s); fflush(stdout);}

/**
 * \def print_1(s)
 *
 * Stampa la stringa S, secondo la sintassi del comando printf, dando la possibilita'
 * di indicare un parametro, e svuota il buffer di standard output
 */
#define print_1(s, p) \
	{printf(s, p); fflush(stdout);}

/**
 * \def print_2(s)
 *
 * Stampa la stringa S, secondo la sintassi del comando printf, dando la possibilita'
 * di indicare un parametro, e svuota il buffer di standard output
 */
#define print_2(s, p, b) \
	{printf(s, p, b); fflush(stdout);}

/**
 * \def err_print(s)
 *
 * Stampa la stringa S su standard error secondo la classica sintassi di printf.
 */
#define err_print(s) \
	{fprintf(stderr, s);}

/**
 * \def err_print_1(s)
 *
 * Stampa la stringa S su standard error secondo la classica sintassi di printf, dando la possibilita'
 * di indicare un parametro.
 */
#define err_print_1(s, p) \
	{fprintf(stderr, s, p);}

/**
 * \def sys_print(s)
 *
 * Stampa la stringa S, secondo la sintassi del comando printf, e svuota il buffer di standard output.
 * Esegue le stesse cose di print(s), con l'unica differenza che puo' essere ridefinita se non e' stata
 * settata la macro VERBOSE
 * \see print(s)
 */
#define sys_print(s) \
	{printf(s); fflush(stdout);}

/**
 * \def sys_print_1(s)
 *
 * Stampa la stringa S, secondo la sintassi del comando printf, offrendo la possibilita' di indicare un
 * parametro, e svuota il buffer di standard output.
 * Esegue le stesse cose di print_1(s), con l'unica differenza che puo' essere ridefinita se non e' stata
 * settata la macro VERBOSE
 * \see print_1(s)
 */
#define sys_print_1(s, p) \
	{printf(s, p); fflush(stdout);}

/**
 * \note Nel caso in cui sia definita la macro VERBOSE provvede a ridefinire il comportamento
 * delle macro sys_print(s) e sys_print_1(s, p)
 *
 * \see sys_print(s)
 * \see sys_print_1(s, p)
 */
#ifndef VERBOSE
	#undef sys_print
	#define sys_print(s)
	#undef sys_print_1
	#define sys_print_1(s, p)
#endif

#endif
