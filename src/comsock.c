/** \file comsock.c
    \author Nicola Corti
    \brief Implementazione delle funzioni definite nell'header file comsock.h

    \bug riga 231: si deve rendere parametrica la scrittura dei caratteri della dimensione.
   
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


#include "comsock.h"

#include "settings.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/**
 * \def CHAR_LENGTH
 *
 * Dimensione in caratteri del campo dimensioni.
 * Necessaria per effettuare la serializzazione del messaggio e la sua ricezione
 *
 * \internal Necessaria per effetuare la serializzazione e la deserializzazione
 *
 * \see sendMessage
 * \see receiveMessage
 */
#define CHAR_LENGTH 4

/**
 * \def BUFF_SIZE
 *
 * Dimensione del campo buffer del messaggio.
 * Necessaria per effettuare la serializzazione del messaggio e la sua ricezione
 *
 * \internal Necessaria per effetuare la serializzazione e la deserializzazione
 *
 * \see sendMessage
 * \see receiveMessage
 */
#define BUFF_SIZE 512


int createServerChannel(char* path){

	int sfd;
	struct sockaddr_un sa;

	if ( path == NULL || strlen(path) == 0 )
		return -1;
	
	if ( strlen(path) > UNIX_PATH_MAX )
	{
		errno = E2BIG;
		return -1;
	}
	
	if ( (sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;
	
	strcpy(sa.sun_path, path);
	sa.sun_family = AF_UNIX;	
	
	if ( bind(sfd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
	{
		close(sfd);
		return -1;
	}

	if ( listen(sfd, SOMAXCONN) == -1)
	{
		close(sfd);
		/* Rimuovo il file che ho creato prima se non riesco a fare la listen */
		(void)unlink(path);
		return -1;
	}
	
	return sfd;
}

int closeSocket(int s){
	/* Uso l'errno settato da close */
	return close(s);
}

int acceptConnection(int s){

	int acc_fd;
	
	if ( (acc_fd = accept(s, NULL, 0)) == -1)
		return -1;
	else
		return acc_fd;
}

int receiveMessage(int sc, message_t * msg){

	int bytes, int_size;
	char size[CHAR_LENGTH + 1];

	if ( msg == NULL || sc < 0)
	{
		errno = EINVAL;
		return -1;
	}

	bytes = read(sc, &(msg->type), sizeof(char));
	if ( bytes <= 0)
	{
		if (bytes == 0) errno = ENOTCONN;
		return -1;
	}
	
	bytes = read(sc, &size, (CHAR_LENGTH) * sizeof(char));
	if ( bytes <= 0)
	{
		if (bytes == 0) errno = ENOTCONN;
		return -1;
	}
	/* Termino la stringa in modo da poterla passare a strtod tranquillamente */
	size[CHAR_LENGTH] = '\0';

	errno = 0;
	int_size = (int)strtod(size, NULL);
	if (errno != 0)	
		return -1;
	
	msg->length = int_size;
	
	if (int_size == 0)
	{
		msg->buffer = NULL;
		return 0;
	}

	/* Se fallisce l'allocazione ritorno un errore */
	if ( (msg->buffer = malloc(int_size * sizeof(char))) == NULL)
		return -1;
	
	bytes = read(sc, msg->buffer, int_size * sizeof(char));
	if ( bytes <= 0)
	{
		if (bytes == 0) errno = ENOTCONN;
		free(msg->buffer);
		return -1;
	}
				
	return bytes;   

}

/** Calcola l'elevamento a potenza, in base 10 di un numero
 *
 * \internal La funzione e' implementata in modo ricorsivo
 *
 * \param [in] exp l'esponente
 * \retval -1 se l'esponente e' negativo
 * \retval 10^exp altrimenti
 */
static int power(int exp){
	if (exp < 0) return -1;
	else
	{
		if (exp == 0) return 10;
		else return (10 * power(exp - 1));
	}
}

/**
 * \bug riga 231: si deve rendere parametrica la scrittura dei caratteri della dimensione.
 */
int sendMessage(int sc, message_t *msg){

	unsigned int size, i, bytes, bufsize, len;
	/* Vettore di caratteri per la serializzazione */
	char *serial;
	char length[CHAR_LENGTH];

	if (msg == NULL || (msg->length != 0 && msg->buffer == NULL))
		return -1;

	if (msg->length > power(CHAR_LENGTH) || msg->length > BUFF_SIZE)
	{
		errno = E2BIG;
		return -1;
	} 

	size = 1 + CHAR_LENGTH + msg->length;
	
	/* Controllo dimensione del buffer del socket in uscita */
	len = sizeof(bufsize);
	if (getsockopt(sc, SOL_SOCKET, SO_SNDBUF, &bufsize, &len) != -1)
	{
		if (size > bufsize)
		{
			errno = E2BIG;
			return -1;
		}
	}
	
	
	if ( (serial = malloc(size * sizeof(char))) == NULL)
		return -1;
	
	serial[0] = msg->type;
	
	/* --- BUG: METTERE PARAMETRICA LA SCRITTURA --- */
	sprintf(length, "%04d", msg->length);
	strncpy(serial + 1, length, CHAR_LENGTH); 

	for(i = 1 + CHAR_LENGTH; i < size; i++)
		serial[i] = msg->buffer[i-(CHAR_LENGTH + 1)];

	bytes = write(sc, serial, size);
	if (bytes <= 0)
	{
		if (bytes == 0) errno = ENOTCONN;
		free(serial);
		return -1;
	}

	/* Libero l'array seriale che ho allocato, dato che non mi serve piu' */
	free(serial);
	return bytes;
}

int openConnection(char* path){

	int i = 0, conn_fd;
	struct sockaddr_un sa;

	if (path == NULL || strlen(path) == 0)
		return -1;
	
	if (strlen(path) > UNIX_PATH_MAX)
	{
		errno = E2BIG;
		return -1;
	}

	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, path);

	if ((conn_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	while(i < NTRIALCONN)
	{
		if (connect(conn_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		{
			sleep(1);
			i++;
		}
		else
			break;
	}
	
	if (i == NTRIALCONN)
		return -1;
	else
		return conn_fd;	
}
