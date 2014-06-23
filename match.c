/** \file match.c
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

#include "match.h"
#include "dgraph.h"

char * nextCity(char *path){

	char *aux;
	if (path == NULL) return NULL;
	if ((aux = strchr(path, '$')) != NULL)
		aux++;

	return aux;
}

int countCity(char *path){
	if (path == NULL)
		return 0;
	else
		return (1 + countCity(nextCity(path)));
}

char * nextPath(char *path, int n){
	if (n <= 0)
		return path;
	else
		return nextPath(nextCity(path), n-1);
}

int isEqualCity(char *off, char *req){

	char *aux_off = off, *aux_req = req;

	if (off == NULL || req == NULL)
		return FALSE;


	aux_off = strchr(off, '$');
	aux_req = strchr(req, '$');

	if (aux_off == NULL || aux_req == NULL)
	{
		if (aux_off != NULL && aux_req == NULL)
			return (strncmp(off, req, (aux_off - off)) == 0);
		if (aux_off == NULL && aux_req != NULL)
			return (strncmp(off, req, (aux_req - req)) == 0);
		return (strcmp(off, req) == 0);
	}

	if ((aux_req - req) != (aux_off - off))
		return FALSE;

	return (strncmp(off, req, (aux_req - req)) == 0);
}

int cityMatch(char *req, char *off, int *begin){

	int n = 0, beg = 0;
	char *aux_off = off, *aux_req = req;

	while (aux_off != NULL){
		while(isEqualCity(aux_off, aux_req))
		{

			n++;
			aux_off = nextCity(aux_off);
			aux_req = nextCity(aux_req);
		}
		aux_off = nextCity(aux_off);
		if (n != 0)
			break;
		beg++;
	}
	*begin = beg;
	return n;
}

int pathLen(char *elem, int begin, int city_found){
	if(city_found == 0)
		return 0;
	else
	{
		if (begin > 0)
			return pathLen(nextCity(elem), begin - 1, city_found);
		else
			return LLABEL + pathLen(nextCity(elem), begin, city_found -1);
	}
}

char * cityString(char *path, int begin, int city_found, int size){

	char *string, *aux;

	if (size <= 0) return NULL;
	if (begin > 0)
		/* Faccio ricorsivamente i passi per arrivare alla citta' desiderata */
		return cityString(nextCity(path), begin -1, city_found, size);

	string = malloc(size * sizeof(char));

	if (string == NULL)
		return NULL;

	memset(string, '\0', size * sizeof(char));

	aux = path;
	while(city_found > 0)
	{
		aux = strchr(aux, '$');
		if (aux != NULL) aux++;
		city_found--;
	}

	/* Caso in cui sono arrivato in fondo alla stringa */
	if (aux == NULL)
	{
		/* Copio la stringa per intero */
		if (path != NULL)
			strcpy(string, path);
		else
			/* Se anche path e' NULL, torno NULL
			Vuol dire che la stringa era scorretta */
			return NULL;
	}
	else
	{
		/* Copio solo la parte interessata */
		aux--;
		strncpy(string, path, aux - path);
	}
	return string;
}

char *cutPath(char *path, int steps){

	/* -- Attenzione modifica la stringa -- */
	char *aux = path;

	while( aux != NULL && steps > 0){
		aux = strchr(aux, '$');
		if (aux != NULL) aux++;
		steps--;
	}

	if (aux != NULL)
		*(aux-1) = '\0';
	return path;
}
