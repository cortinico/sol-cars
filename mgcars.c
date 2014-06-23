/** \file mgcars.c
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
#include "settings.h"
#include "commons.h"
#include "match.h"

#include "comsock.h"

#include "dgraph.h"
#include "shortestpath.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

/* ***************** */

/** Valore true */
#define TRUE 1
/** Valore false */
#define FALSE 0

/*
 * Macro definite per l'elaborazione interna del server
 */
/** Il messaggio parsato e' incorretto */
#define INCORRECT_MESSAGE -1
/** Il messaggio e' un messaggio di aiuto */
#define HELP_MESSAGE 0
/** Il messaggio e' un messaggio di uscita */
#define EXIT_MESSAGE 1
/** Il messaggio e' un messaggio di richiesta */
#define REQ_MESSAGE 2
/** Il messaggio e' un messaggio di offerta */
#define OFF_MESSAGE 3

/** L'utente non e' valido */
#define USER_INVALID 10
/** L'utente non e' stato trovato fra quelli noti */
#define USER_NOT_FOUND 11
/** La password non e' corretta */
#define USER_INVALID_PASS 12
/** L'utente e' gia' loggato da un'altra parte */
#define USER_ALREADY_ON 13
/** L'utente e' corretto */
#define USER_CORRECT 14

/** Il messaggio e' corretto */
#define MESSAGE_CORRECT 30
/** Il numero dei posti contiene un errore */
#define NUMERIC_ERROR 31
/** La partenza non e' stata trovata */
#define DEPARTURE_NOT_FOUND 32
/** L'arrivo non e' stato trovato */
#define ARRIVAL_NOT_FOUND 33
/** La partenza e' troppo lunga */
#define DEPARTURE_TOO_LENGTH 34
/** L'arrivo e' troppo lungo */
#define ARRIVAL_TOO_LENGTH 35

/* ************************ */

/*
 * Strutture utilizzate dal server
 */


/** \struct credential
 * 	Struttura utilizzata per gestire i dati degli utenti
 */
struct credential{
	/** Nome utente */
	char username[LUSERNAME];
	/** Password dell'utente */
	char password[LUSERNAME];
	/** Bit che indica se l'utente e' attivo o meno */
	int active;
	/** Puntatore alla prossima struttura */
	struct credential *next;
};
/** \typedef credential_t
 *	Tipo utilizzato per le credentiali dell'utente
 *	\see credential
 */
typedef struct credential credential_t;


/** \struct worker
 * 	Struttura utilizzata per gestire il pool di thread
 */
struct worker{
	/** TID del relativo thread */
	pthread_t tid;
	/** Socket in entrata presso il worker */
	int in_sck;
	/** Socket in uscita presso il worker */
	int out_sck;
	/** Bit che indica se il worker e' attivo */
	int active;
	/** Variabile per l'attesa e il risveglio del worker */
	pthread_cond_t cond;
	/** Numero delle richieste inviate */
	int sentreqs;
};
/** \typedef worker_t
 *	Tipo utilizzato per il pool di thread
 *	\see worker
 */
typedef struct worker worker_t;



/** \struct reqoff
 * 	Struttura utilizzata per gestire le code di offerte/richieste
 */
struct reqoff{
	/** bit che indica se se la richiesta e' un offerta o una richiesta */
	int type;
	/** utente che ha inviato la richiesta/offerta */
	char user[LUSERNAME];
	/** citta' di partenza */
	char depart[LLABEL];
	/** citta' di arrivo */
	char arriv[LLABEL];
	/** numero dei posti */
	int place;
	/** file descripor della socket sulla quale inviare gli SHARE_END */
	int share_fd;
	/** puntatore alla prossima struttura */
	struct reqoff *next;
	/** stringa che contiene il cammino minimo fra depart e arriv */
	char *path;
};

/** \typedef reqoff_t
 *	Tipo utilizzato per le richieste/offerte
 *	\see reqoff
 */
typedef struct reqoff reqoff_t;



/** \struct assoc
 * 	Struttura utilizzata per gestire i match che son stati trovati.
 */
struct assoc{
	/** Numero di citta' che soddisfano questo step del match */
	int city_found;
	/** Numero di citta' da cui partire relativo all'offerta relativa a questo step */
	int begin_from;
	/** Puntatore all'offerta che soddisfa questo step */
	reqoff_t *found_off;
	/** puntatore alla prossima struttura */
	struct assoc *next;
};
/** \typedef assoc_t
 *	Tipo utilizzato per gestire gli step di un match
 *	\see credential
 */
typedef struct assoc assoc_t;




/* ***************************** */

/* -----------------------------
 * Variabili Globali
 * -----------------------------
 */

/** \var mux_user
 * 	Utilizzata per la mutua esclusione sulla lista degli utenti
 * 	\see list */
pthread_mutex_t mux_user = PTHREAD_MUTEX_INITIALIZER;

/** \var mux_pool
 * 	Utilizzata per la mutua esclusione sul pool di thread
 * 	\see pool */
pthread_mutex_t mux_pool = PTHREAD_MUTEX_INITIALIZER;

/** \var mux_request
 * 	Utilizzata per la mutua esclusione sulla coda delle richieste */
pthread_mutex_t mux_request = PTHREAD_MUTEX_INITIALIZER;

/** \var mux_offer
 * 	Utilizzata per la mutua esclusione sulla coda delle offerte */
pthread_mutex_t mux_offer = PTHREAD_MUTEX_INITIALIZER;

/** \var pool
 *  Struttura che contiene tutti i dati degli worker
 *  \see POOL_SIZE
 *  \see worker */
worker_t pool[POOL_SIZE];

/** \var list
 *  Lista degli utenti attivi
 *  \see credential
 */
credential_t *list = NULL;

/** \var map
 * 	Grafo orientato del server
 */
graph_t *map;

/** \var request
 *  Coda delle richieste pendenti
 *  \see reqoff */
reqoff_t *request = NULL;

/** \var offer
 *  Coda delle offer pendenti
 *  \see reqoff */
reqoff_t *offer = NULL;

/** \var matcher
 *
 *  TID del matcher
 *  utilizzato per l'invio dei messaggi
 */
pthread_t matcher;

/** \var handler
 *  
 *  TID del signal handler
 *  utilizzato per l'invio dei messaggi
 */
pthread_t handler;

/** \var listener
 *
 *  TID del listener (main)
 *  utilizzato per l'invio dei messaggi
 */
pthread_t listener;





/** Vettore utilizzato per passare i parametri agli worker */
int num[POOL_SIZE];

/** \var log_fd
 * 	File descriptor del file di log
 */
int log_fd = -1;

/**
 * 	Flag che viene utilizzato per coordinare la chiusura del sistema
 * 	Di default vale 0
 * 	Viene settato a 1, appena il signal handler riceve un segnale di terminazione.
 * 	Questo indica agli worker che devono essere terminati
 * 	Viene settato a 2, appena tutti gli worker sono terminati
 * 	Questo indica al thread match che deve terminare
 *
 * 	\internal Tipo volatile sig_atomic_t per tralasciare la gestione della mutua esclusione
 */
volatile sig_atomic_t received_signal = 0;


/* ***************** */

/* -----------------------------------
 * Funzioni
 * -----------------------------------
 */

/** Incrementa il valore della variabile received_signal
 * 	\see received_signal
 */
void increaseFlagSig(){
	received_signal++;
}

/** Ritorna il valore della variabile received_signal
 *
 * 	\retval il valore della variabile suddetta
 * 	\see received_signal
 */
int getFlagSig(){
	return received_signal;
}

/** Controlla la validita' del messaggio mess
 *
 * \param [in] mess il messaggio da verificare
 * \param [out] depart se il messaggio e' corretto, scrive su depart la citta' indicata come partenza
 * \param [out] arrival se il messaggio e' corretto, scrive su depart la citta' indicata come arrivo
 * \param [out] place se il messaggio e' corretto, scrive place il numero dei posti dell'offerta (se e' una offerta, altrimenti il valore non e' significativo)
 *
 * \retval MESSAGE_CORRECT se il messaggio e' corretto
 * \retval NUMERIC_ERROR se e' presente un errore nel numero dei posti
 * \retval DEPARTURE_NOT_FOUND se la partenza non e' stata trovata nel grafo
 * \retval ARRIVAL_NOT_FOUND se l'arrivo non e' stato trovato nel grafo
 * \retval DEPARTURE_TOO_LENGTH se la partenza e' troppo lunga
 * \retval ARRIVAL_TOO_LENGTH se l'arrivo e' troppo lungo
 *
 * \attention depart e arrival devono essere gia' allocate
 */
int checkValidMessage(message_t *mess, char *depart, char *arrival, int *place){

	char *dep, *arr, *num = NULL;
	int num_int = 0;

	memset(depart, '\0', LLABEL*sizeof(char));
	memset(arrival, '\0', LLABEL*sizeof(char));

	dep = mess->buffer;

	arr = strchr(dep, ':');
	*arr = '\0';
	arr++;

	strcpy(depart, dep);
	if (mess->type == MSG_OFFER)
	{
		num = strchr(arr, ':');
		*num = '\0';
		num++;
		errno = 0;
		num_int = (int)strtod(num, NULL);
		if (errno != 0 || num_int <= 0) return NUMERIC_ERROR;
		*place = num_int;
	}
	strcpy(arrival, arr);

	if(strlen(dep) > LLABEL)
		return DEPARTURE_TOO_LENGTH;
	if(strlen(arr) > LLABEL)
		return ARRIVAL_TOO_LENGTH;
	if(is_node(map, dep) == -1)
		return DEPARTURE_NOT_FOUND;
	if(is_node(map, arr) == -1)
		return ARRIVAL_NOT_FOUND;

	return MESSAGE_CORRECT;
}

/* =============================================
 * GESTIONE DEGLI UTENTI
 * =============================================
 */


/** Controlla la correttezza di un utente
 *
 *  \internal Acquisisce la mutua esclusione sull'elenco degli utenti e la rilascia successivamente
 *
 *  \param user [in] stringa contenente il nome utente
 *  \param pass [in] stringa contenente la password
 *
 *  \retval USER_NOT_FOUND se l'utente non e' stato trovato
 *  \retval USER_INVALID_PASSWORD se l'utente ha inserito la password errata
 *  \retval USER_ALREADY_ON se l'utente e' settato come gia' attivo
 *  \retval USER_CORRECT se l'utente puo' autenticarsi
 *  \retval USER_INVALID in tutti gli altri casi
 */
int isCorrectUser(char *user, char *pass){

	credential_t *aux = list;

	if (user == NULL || pass == NULL) return USER_INVALID;

	pthread_mutex_lock(&mux_user);
	
	if (list == NULL)
	{
		pthread_mutex_unlock(&mux_user);
		return USER_NOT_FOUND;
	}
	
	while(aux != NULL && strcmp(aux->username, user))
		aux = aux->next;

	if (aux == NULL)
	{
		pthread_mutex_unlock(&mux_user);
		return USER_NOT_FOUND;
	}
	if (strcmp(aux->password, pass) != 0)
	{
		pthread_mutex_unlock(&mux_user);
		return USER_INVALID_PASS;
	}
	if (aux->active == 1)
	{
		pthread_mutex_unlock(&mux_user);
		return USER_ALREADY_ON;
	}
	pthread_mutex_unlock(&mux_user);
	return USER_CORRECT;
}

/** Estrae i dati di login da un messaggio
 *
 * 	\param [in] mess il messaggio di login
 * 	\param [out] user la stringa che conterra' l'utente (gia' allocata)
 * 	\param [out] pass la stringa che conterra' la password (gia' allocata)
 * 	\param [out] sck la stringa che conterra' il nome del socket (gia' allocata)
 *
 * 	\retval 0 se i dati sono estratti correttamente
 * 	\retval -1 altrimenti
 */
int extractLoginData(message_t * mess, char *user, char *pass, char *sck){

	char *aux = NULL;

	if (mess->type != MSG_CONNECT || mess->length <= 0) return -1;

	/* Dovremo controllare meglio...
	 * Qui un messaggio composto male potrebbe bloccare il server
	 * */
	aux = mess->buffer;
	strcpy(user, aux);
	aux = aux + strlen(aux) + 1;
	strcpy(pass, aux);
	aux = aux + strlen(aux) + 1;
	strcpy(sck, aux);

	return 0;
}

/** Libera la lista degli utenti
 *
 *  \attention da invocare soltanto in fase di chiusura del server
 *  \attention non vengono trattati i problemi di mutua esclusione
 *
 *  \retval NULL
 */
credential_t * freeUserList(){

	credential_t *aux = list;
	while(aux != NULL){
		aux = list->next;
		free(list);
		list = aux;
	}
	return NULL;
}

/** Aggiunge un nuovo utente e lo imposta come attivo
 *
 * 	\param [in] user stringa contenente il nome utente del nuovo utente
 * 	\param [in] pass stringa contenente la password del nuovo utente
 *
 * 	\internal Acquisisce la mutua esclusione sull'elenco degli utenti e la rilascia successivamente
 *
 *	\retval 0 se l'aggiunta e' avvenuta correttamente
 *	\retval -1 altrimenti
 */
int addNewUser(char *user, char *pass){

	credential_t *new_usr;

	if (user == NULL || pass == NULL)
		return -1;

	if ( (new_usr = malloc(sizeof(credential_t))) == NULL)
		return -1;

	memset(new_usr->username, '\0', LUSERNAME);
	memset(new_usr->password, '\0', LUSERNAME);
	strcpy(new_usr->username, user);
	strcpy(new_usr->password, pass);
	new_usr->active = 1;

	pthread_mutex_lock(&mux_user);
	if (list == NULL)
		new_usr->next = NULL;
	else
		new_usr->next = list;
	list = new_usr;
	pthread_mutex_unlock(&mux_user);

	return 0;
}

/** Logga un utente nel sistema
 *
 * 	\param [in] user nome utente dell'utente da loggare
 *  \internal Acquisisce la mutua esclusione sull'elenco degli utenti e la rilascia successivamente
 */
void loginUser(char *user){

	credential_t *aux;

	pthread_mutex_lock(&mux_user);
		aux = list;
		while(aux != NULL && strcmp(aux->username, user) != 0) aux = aux->next;
		if (aux != NULL) aux->active = 1;
	pthread_mutex_unlock(&mux_user);
	return;
}


/** Slogga un utente nel sistema
 *
 * 	\param [in] user nome utente dell'utente da sloggare
 *  \internal Acquisisce la mutua esclusione sull'elenco degli utenti e la rilascia successivamente
 */
void logoutUser(char *user){

	credential_t *aux;

	pthread_mutex_lock(&mux_user);
		aux = list;
		while(aux != NULL && strcmp(aux->username, user) != 0) aux = aux->next;
		if (aux != NULL) aux->active = 0;
	pthread_mutex_unlock(&mux_user);
	return;
}


/* =============================================
 * GESTIONE DEL POOL DI THREAD
 * =============================================
 */

/** Cerca ed attiva un thread dormiente fra quelli del pool
 *
 *	\param [in] receive_fd il file descriptor della socket da assegnare al nuovo worker
 *	\param [in] init vale TRUE se si sta inizializzando lo scheduler, FALSE altrimenti
 *
 *	\internal Acquisisce la mutua esclusione sul pool di thread e la rilascia successivamente
 *	\internal Utilizza una variabile statica per gestire lo scheduling round robin
 *
 *	\retval -1 se non si e' trovato un thread disponibile
 *	\retval l'indice nel pool di thread del thread trovato
 */
int findActivateThread(int receive_fd, int init){

	int step = 0, value = -1;
	static int lookup;

	/* Inizializzazione della variabile statica */
	if (init == TRUE)
	{
		lookup = 0;
		return -1;
	}
	pthread_mutex_lock(&mux_pool);
	while(step < POOL_SIZE)
	{
		if(pool[lookup].active == 0)
		{
			pool[lookup].active = 1;
			pool[lookup].in_sck = receive_fd;
			pool[lookup].sentreqs = 0;
			value = lookup;
			pthread_cond_signal(&(pool[lookup].cond));
			break;
		}
		step++;
		lookup = (lookup + 1) % POOL_SIZE;
	}
	pthread_mutex_unlock(&mux_pool);
	return value;
}

/** Inizializza il pool di thread
 *
 * 	\attention da utilizzare solo all'avvia del sistema
 * 	\attention non tiene di conto dei problemi di mutua esclusione
 *
 */
void initializePool(){

	int i;
	for (i = 0; i<POOL_SIZE; i++){
		pool[i].out_sck = -1;
		pool[i].in_sck = -1;
		pool[i].active = 0;
		pool[i].sentreqs = 0;
		pthread_cond_init(&(pool[i].cond), NULL);
	}
	return;
}





/* =============================================
 * GESTIONE DEI MESSAGGI
 * =============================================
 */


/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di share valido
 *
 * \param [out] out Il messaggio
 * \param [in] buff il buffer da copiare in quello del messaggio
 *
 * Il buffer del messaggio deve essere stato gia' allocato
 */
void setShareMessage(message_t *out, char* buff){

	if (out == NULL || buff == NULL)return;

	out->type = MSG_SHARE;
	strcpy(out->buffer, buff);
	out->length = strlen(buff) + 1;
}

/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di share_end valido
 *
 * \param [out] out Il messaggio
 */
void setSharEndMessage(message_t *out){

	if (out == NULL)return;

	out->type = MSG_SHAREND;
	out->length = 0;
}

/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di ok valido
 *
 * \param [out] out Il messaggio
 */
void setOkMessage(message_t *out){

	if(out == NULL)return;

	out->type = MSG_OK;
	out->length = 0;

	return;
}

/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di error valido
 *
 * \param [out] out Il messaggio
 * \param [in] type il tipo utilizzato per settare il messaggio di errore
 */
void setErrorMessage(int type, message_t *out){

	if (out == NULL) return;

	out->type = MSG_NO;
	switch (type) {
		case USER_ALREADY_ON:
			sprintf(out->buffer, "You are already connected somewhere\n");
			break;
		case USER_INVALID_PASS:
			sprintf(out->buffer, "Invalid password!\n");
			break;
		case USER_INVALID:
			sprintf(out->buffer, "Combination of username and password is invalid\n");
			break;
		case NUMERIC_ERROR:
			sprintf(out->buffer, "The number of place you sent in offer in incorrect\n");
			break;
		case DEPARTURE_NOT_FOUND:
			sprintf(out->buffer, "DEPARTURE does not exist in server internal map\n");
			break;
		case ARRIVAL_NOT_FOUND:
			sprintf(out->buffer, "ARRIVAL does not exist in server internal map\n");
			break;
		case DEPARTURE_TOO_LENGTH:
			sprintf(out->buffer, "Max DEPARTURE length is set to %d\n", LLABEL);
			break;
		case ARRIVAL_TOO_LENGTH:
			sprintf(out->buffer, "Max ARRIVAL length is set to %d\n", LLABEL);
			break;
		default:
			sprintf(out->buffer, "Unknow error, contact system administrator\n");
			break;
	}
	out->length = strlen(out->buffer)+1;
	return;
}


/* =============================================
 * GESTIONE DELLE CODE DI RICHIESTE/OFFERTE
 * =============================================
 */

/** Aggiugne un elemento in fondo ad una coda di offerte/richieste
 *
 * \param [in] list coda in cui inserire
 * \param [in] elem elemento da inserire
 *
 * \attention accertarsi di aver acquisito la mutua esclusione prima di invocare questa funzione
 */
reqoff_t * appendList(reqoff_t *list, reqoff_t *elem){

	reqoff_t *aux = list;

	if (list == NULL) return elem;

	while(aux->next != NULL) aux = aux->next;

	aux->next = elem;
	return list;
}

/** Crea un nuovo elemento di tipo reqoff_t e lo inserisce nella coda corretta
 *
 * \internal Acquisisce la mutua esclusione sulla coda specifica e la rilascia successivamente
 *
 * \param [in] mess il messaggio relativo
 * \param [in] depart stringa contenente la localita' di partenza
 * \param [in] arrival stringa contenente la localita' di arrivo
 * \param [in] place numero dei posti
 * \param [in] send_fd file descriptor della socket nella quale inviare evenutali share
 * \param [in] user stringa contenente l'utente che invia la richiesta/offerta
 *
 * \retval -1 in caso di errore
 * \retval 0 in tutti gli altri casi
 *
 * \see appendList
 */
int appendReqOff(message_t *mess, char *depart, char *arrival, int place, int send_fd, char *user){

	reqoff_t *new_reqoff = NULL;
	int i_dep, i_arr;
	unsigned int *prec;
	double *dijk;

	if ((new_reqoff= malloc(sizeof(reqoff_t))) == NULL) return -1;

	new_reqoff->type = mess->type;
	new_reqoff->share_fd = send_fd;
	new_reqoff->next = NULL;
	strcpy(new_reqoff->user, user);
	strcpy(new_reqoff->depart, depart);
	strcpy(new_reqoff->arriv, arrival);

	if (mess->type == MSG_OFFER)
		new_reqoff->place = place;
	else
		new_reqoff->place = 0;

	i_dep = is_node(map, new_reqoff->depart);
	i_arr = is_node(map, new_reqoff->arriv);

	if ((dijk = dijkstra(map, is_node(map, new_reqoff->depart), &prec)) == NULL)
		return -1;
	free(dijk);

	new_reqoff->path = shpath_to_string(map, is_node(map, new_reqoff->depart), is_node(map, new_reqoff->arriv), prec);

	free(prec);

	if(mess->type == MSG_REQUEST)
	{
		pthread_mutex_lock(&mux_request);
		request = appendList(request, new_reqoff);
		pthread_mutex_unlock(&mux_request);
	}
	else
	{
		pthread_mutex_lock(&mux_offer);
		offer = appendList(offer, new_reqoff);
		pthread_mutex_unlock(&mux_offer);
	}

	return 0;
}

/** Libera una coda delle richieste/offerte
 *
 *  \attention da invocare soltanto in fase di chiusura del server
 *  \attention non vengono trattati i problemi di mutua esclusione
 *
 *  \retval NULL
 */
reqoff_t * freeReqOffList(reqoff_t *list){

	reqoff_t *aux = list;
	while(aux != NULL)
	{
		aux = list->next;
		free(list->path);
		free(list);
		list = aux;
	}
	return NULL;
}

/** Stampa una coda di richieste/offerte
 *
 * 	\param [in] list la coda da stampare
 */
void printLists(reqoff_t *list){

	reqoff_t *aux = list;
	while(aux != NULL)
	{
		if (aux->path != NULL)
			printf("dep: %s arr: %s user: %s place: %d type: %c path: %s\n", aux->depart, aux->arriv, aux->user, aux->place, aux->type, aux->path);
		aux = aux->next;
	}
}



/* =============================================
 * THREAD MATCH
 * =============================================
 */

/** Alloca e restituisce una stringa contenente l'associazione da scrivere sul log e da inviare con un messaggio share.
 *
 *  \param [in] user il nome utente di chi ha effettuato la richiesta
 *  \param [in] elem puntatore allo step del match di cui si desidera la stringa
 *
 *  \retval NULL se e' avvenuto un errore
 *  \retval il puntatore alla stringa altrimenti
 *
 */
char * createAssoc(char *user, assoc_t *elem){

	int size, path_size;
	char *association, *path;

	path_size = pathLen(elem->found_off->path, elem->begin_from, elem->city_found);

	size = strlen(user) + 1 + strlen(elem->found_off->user) + 1 + path_size + 1;
	/* Va gestito */
	association = malloc(size * sizeof(char));
	if (association != NULL)
	{
		path = cityString(elem->found_off->path, elem->begin_from, elem->city_found, path_size);
		if (path != NULL)
		{
			sprintf(association, "%s$%s$%s", elem->found_off->user, user, path);
			/* Libero il path */
			free(path);
		}
	}


	return association;
}

/** Cerca di creare un percorso che soddisfi il path, e restituisce il percorso effettuato tramite l
 *
 *	\internal implementato in modo ricorsivo
 *
 *	\param [in] path il path che si vuole cercare di soddisfare
 *  \param [out] la lista del percorso effettuato.
 *
 *  \retval TRUE se si e' riusciti a trovare un percorso (dunque l e' diverso da null)
 *  \retval FALSE se non si e' risciti a trovare un percorso
 *
 *  \b Se il valore e' FALSE il percorsono non viene free, l'utente deve provvedere a questo.
 *
 */
int findRec(char *path, assoc_t **l){

	reqoff_t *aux = offer, *max_off = aux;
	assoc_t *new_ass = NULL;
	int max = 0, val, val_begin, max_begin;

	while(aux != NULL){
		if (aux->place <= 0)
		{
			aux = aux->next;
			continue;
		}
		val = cityMatch(path, aux->path, &val_begin);
		if (val > max)
		{
			max = val;
			max_off = aux;
			max_begin = val_begin;
		}
		aux = aux->next;
	}

	if (max == 1 || max == 0)
		/* Se ho trovato solo 1 citta' vuol dire che non riesco a spostarmi */
		/* Se ho trovato 0 citta' vuol dire che il match non e' proprio possibile */
		return FALSE;
	else
	{
		/* Se fallisce la malloc si deve ritornare false...e si cancellera'
		 *  poi la lista */
		new_ass = malloc(sizeof(assoc_t));
		if (new_ass == NULL)
			return FALSE;

		new_ass->city_found = max;
		new_ass->begin_from = max_begin;
		new_ass->found_off = max_off;

		if (*l == NULL)
			new_ass->next = NULL;
		else
			new_ass->next = *l;
		*l = new_ass;

		if(max == countCity(path))
			return TRUE;
		else
			return findRec(nextPath(path, max - 1), l);
	}
}

/** Scrive un'associazione nel file di log
 *
 * 	\param [in] assoc puntatore alla associazione da scrivere
 *
 */
void writeToLog(char *assoc){

	if (assoc != NULL)
		{write(log_fd, assoc, strlen(assoc) * sizeof(char));
		write(log_fd, "\n", 1);}
}

/** Libera la lista del percorso trovato
 *
 *  \retval NULL
 */
assoc_t * freeAssocList(assoc_t *list){

	assoc_t *aux;
	while(list != NULL)
	{
		aux = list->next;
		free(list);
		list = aux;
	}
	return NULL;
}

/** Alloca e restituisce una nuova offerta da un posto, in modo da poter aggiorare la lista delle offerte.
 *
 * 	\param [in] depart la stringa contentente la partenza
 * 	\param [in] arriv la stringa contentente l'arrivo
 * 	\param [in] user la stringa contentente l'utente
 * 	\param [in] path la stringa contentente il path
 * 	\param [in] type il tipo (nel caso in cui si debba utilizzare per spaccare una richiesta, nel nostro caso vale sempre MSG_OFFER)
 *	\param [in] share_fd il file descriptor relativo all'offerta
 *
 *	\retval NULL in caso di errore
 *	\retval il puntatore ad una nuova struttura altrimenti
 *
 */
reqoff_t * newRemainOffer(char *depart, char *arriv, char *user, char *path, int type, int share_fd){

	reqoff_t *aux = NULL;

	if ( (aux = malloc(sizeof(reqoff_t))) == NULL)
		return NULL;

	strcpy(aux->depart, depart);
	strcpy(aux->arriv, arriv);
	strcpy(aux->user, user);
	aux->path = path;
	aux->type = type;
	aux->share_fd = share_fd;
	aux->place = 1;

	return aux;

}

/** Partendo da un passo di un match aggiorna la lista delle offerte
 *
 * \param [in] ass il passo del match di cui si desidera aggiornare l'offerta.
 *
 * Se la macro UPDATE_OFFER non e' settata si provvede solamente a diminuire il numero dei posti
 * altrimenti si spacca l'offerta in 2/3
 */
void updateOfferList(assoc_t *ass){

	reqoff_t *aux = ass->found_off;
#ifdef UPDATE_OFFER
	reqoff_t *new_off = NULL;
	char *new_path, *aux_path, *new_city, new_depart[LLABEL];
	memset(new_depart, '\0', LLABEL);
#endif

	sys_print_1("[ MATCH ] Updating offer %s\n", ass->path)
	(aux->place)--;

#ifdef UPDATE_OFFER

	if(ass->begin_from != 0){

		new_path = strdup(aux->path);
		new_path = cutPath(new_path, ass->begin_from + 1);
		new_city = strrchr(new_path, '$') + 1;

		new_off = newRemainOffer(aux->depart, new_city, aux->user, new_path, aux->type, aux->share_fd);
		new_off->next = aux->next;
		aux->next = new_off;
	}

	if(ass->city_found != countCity(nextPath(aux->path, ass->begin_from))){

		aux_path = nextPath(aux->path, ass->begin_from + ass->city_found - 1);
		new_path = strdup(aux_path);
		new_city = strchr(new_path, '$');
		strncpy(new_depart, new_path, new_city - new_path);

		new_off = newRemainOffer(aux->depart, aux->arriv, aux->user, new_path, aux->type, aux->share_fd);
		new_off->next = aux->next;
		aux->next = new_off;
	}
#endif
}


/** Data una richiesta r cerca di crearne un possibile match, e stampa ogni passo del match sul log
 *  Inoltre invia un messaggio di share al file descriptor indicato
 *
 *	\param [in] r la richiesta da elaborare
 *
 *	Invoca la funzione ricorsiva findRec
 *	\see findRec
 */
void findAssociation (reqoff_t *r){

	assoc_t *list = NULL, *aux;
	char buffer[BUFF_SIZE], *assoc;
	message_t out_message;

	memset(buffer, '\0', BUFF_SIZE);
	out_message.buffer = buffer;

	sys_print_1("[ MATCH ] Looking match for %s\n", r->path);
	if (findRec(r->path, &list) == TRUE)
	{
		aux = list;
		while(aux != NULL)
		{
			/* createAssoc alloca memoria, deve essere liberata */
			assoc = createAssoc(r->user, aux);
			writeToLog(assoc);
			sys_print_1("[ MATCH ] Association %s wrote into log\n", assoc);
			setShareMessage(&out_message, assoc);

			sys_print_1("[ MATCH ] Association %s sending SHARE message\n", assoc);

			/* Se non riesco ad inviare perche' il client e' caduto, vado avanti lo stesso... */
			errno = 0;
			sendMessage(r->share_fd, &out_message);
			if (errno == EINTR)
			{
				free(assoc);
				break;
			}

			/* Libero la stringa dell'associazione */
			free(assoc);

			/* Aggiorno la lista delle offerte */

			updateOfferList(aux);

			aux = aux->next;
		}

	}
	list = freeAssocList(list);
}

/** Rilascia la mutua esclusione sul pool di thread, sulla coda richieste e sulla coda offerte
 */
void unlockAll(){
	pthread_mutex_unlock(&mux_pool);
	pthread_mutex_unlock(&mux_request);
	pthread_mutex_unlock(&mux_offer);
}

/** Acquisisce la mutua esclusione sul pool di thread, sulla coda richieste e sulla coda offerte
 */
void lockAll(){
	pthread_mutex_lock(&mux_pool);
	pthread_mutex_lock(&mux_request);
	pthread_mutex_lock(&mux_offer);
}


/** Funzione relativa al thread match
 *
 *  Il thread match resta in attesa di un messaggio SIGUSR1 oppure dello scadere
 *  del quanto di tempo
 *
 *  Successivamente acquisisce la mutua esclusione, elabora tutte le richieste in coda,
 *  ed invia i messaggi di share_end
 *
 *	Ad ogni esecuzione svuota la coda delle richieste
 *
 *	Se la macro FREE_OFFER_LIST e' settata, viene liberata la coda delle offerte, altrimenti viene
 *	lasciata per le prossime esecuzioni
 *
 *	Successivamente si rimette in attesa.
 */
void * match(){

	sigset_t set;
	struct timespec time;
	int sig, i, byte;
	reqoff_t *aux;
	message_t out_message;
	char buffer[BUFF_SIZE];

	out_message.buffer = buffer;

	time.tv_sec = SEC_TIMER;
	time.tv_nsec = NSEC_TIMER;

	ec_neg1_c(sigemptyset(&set), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGINT), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGTERM), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGPIPE), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGUSR1), "Signal Handling", pthread_exit(NULL); )
	ec_zero_c(pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling", pthread_exit(NULL); )

	ec_neg1_c(sigemptyset(&set), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGUSR1), "Signal Handling", pthread_exit(NULL); )

	print("[ MATCH ] Match thread succesfully started\n");
	while(1){
		errno = 0;
		sig = sigtimedwait(&set, NULL, &time);

		if (sig == -1 && errno == EAGAIN)
			print("[ MATCH ] Started from time quantum elapsed\n")
		else
			print_1("[ MATCH ] Started from signal: %d\n", sig)


		/* Inizio sezione critica */
		lockAll();

#ifdef VERBOSE
		/* Se verbose e' attivo, stampo le code */
		print("[ MATCH ] Printing request List\n")
		printLists(request);
		print("[ MATCH ] Printing offer List\n");
		printLists(offer);
#endif

		/* Fase del match */
		aux = request;
		while (aux != NULL){
			findAssociation(aux);
			aux = aux->next;
		}

		/* Fase di ripulitura delle code */
		print("[ MATCH ] Freeing up request list\n");
		request = freeReqOffList(request);
#ifdef FREE_OFFER_LIST
		print("[ MATCH ] Freeing up offer List\n");
		offer =  freeReqOffList(offer);
#endif

		/* Invio gli share end */
		sys_print("[ MATCH ] Start sending SHARE_END messages\n");
		for(i = 0; i < POOL_SIZE; i++)
		{
			if(pool[i].sentreqs > 0)
			{
				pool[i].sentreqs = 0;
				setSharEndMessage(&out_message);
				errno = 0;
				if ((byte = sendMessage(pool[i].out_sck, &out_message)) == -1)
				{
					if (errno == ENOTCONN)
						continue;
					if (errno == EINTR);
						break;
				}
			}
		}

		unlockAll();
		/* Fine sezione critica */
		print("[ MATCH ] Work over, back to sleep\n");

		if (getFlagSig() == 2)
			break;
	}

	/* Svuoto le 2 liste - Qui ho la certezza che gli worker sono gia' morti
	 * Posso tralasciare la mutua esclusione */
	print("[ MATCH ] About to exit, freeing up request list\n");
	request = freeReqOffList(request);
	print("[ MATCH ] About to exit, freeing up offer list\n");
	offer = freeReqOffList(offer);

	print("[ MATCH ] Exiting...\n");
	return (void *)0;
}




/* =============================================
 * THREAD GESTORE DEI SEGNALI
 * =============================================
 */

/** Funzione relativa al thread gestore dei segnali.
 *
 *  Il thread una volta avviato si mette in attesa di segnali SIGINT o SIGTERM.
 *  Una volta arrivati tali messaggi coordina la terminazione del server.
 *
 *  Per maggiori informazioni sulla terminazione del server controllare la relazione
 *
 */
void * signal_handler(){

	sigset_t set;
	int sig, i;

	ec_neg1_c(sigemptyset(&set), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGUSR1), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGTERM), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGINT), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGPIPE), "Signal Handling", pthread_exit(NULL); )
	ec_zero_c(pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigdelset(&set, SIGUSR1), "Signal Handling", pthread_exit(NULL); )

	print("[ SIGNAL ] Ready for receiving signal\n");
	while(1){
		sigwait(&set, &sig);
		sys_print_1("[ SIGNAL] Received signal %d\n", sig);
		if (sig == SIGINT || sig == SIGTERM)
			break;
		/* Nel caso SIGPIPE viene gia' ignorato a livello di processo */
		if (sig == SIGPIPE)
			continue;
	}

	print("[ SIGNAL ] Now shutting down server\n");

	/* Terminazione del server */
	increaseFlagSig();

	/* Fase terminazione worker */
	pthread_mutex_lock(&mux_pool);
	for(i = 0; i<POOL_SIZE; i++)
	{
		if (pool[i].active == 0)
		{
			pool[i].active = 1;
			pthread_cond_signal(&(pool[i].cond));
		}
		pthread_kill(pool[i].tid, SIGUSR2);
	}
	pthread_mutex_unlock(&mux_pool);

	for(i = 0; i<POOL_SIZE; i++)
	{
		sys_print_1("[ SIGNAL ] Waiting for worker %d\n", i)
		pthread_join(pool[i].tid, NULL);
	}

	/* Fase terminazione match */
	increaseFlagSig();
	pthread_kill(matcher, SIGUSR1);
	sys_print_1("[ SIGNAL ] Waiting for match %d\n", i)
	pthread_join(matcher, NULL);

	/* Chiudo le eventuali socket che sono rimaste aperte da match */
	for (i = 0; i<POOL_SIZE; i++)
	{
		if (pool[i].out_sck != -1)
			closeSocket(pool[i].out_sck);
	}

	/* Mando un segnal al thread main, in modo che aspetti il mio valore di ritorno */
	sys_print("[ SIGNAL ] Sending SIGUSR2 to main thread\n")
	pthread_kill(listener, SIGUSR2);
	print("[ SIGNAL ] Signal handler thread is exiting...\n")
	pthread_exit(NULL);
}




/* =============================================
 * THREAD WORKER
 * =============================================
 */

/** Aumenta il numero delle richieste relative al singolo worker
 *
 * 	\param [in] my_id l'indice nel pool di thread del worker
 *
 * 	\internal Acquisisce la mutua esclusione sul pool di thread e la rilascia successivamente
 */
void increaseRequests(int my_id){
	pthread_mutex_lock(&mux_pool);
	(pool[my_id].sentreqs)++;
	pthread_mutex_unlock(&mux_pool);
}

/** Imposta il valore del socket in uscita del worker
 *
 * 	\param [in] my_id l'indice nel pool di thread del worker
 *	\param [in] out_fd il file descriptor del socket in uscita
 *
 * 	\internal Acquisisce la mutua esclusione sul pool di thread e la rilascia successivamente
 */
void setMyOutSck(int my_id, int out_fd){
	pthread_mutex_lock(&mux_pool);
	pool[my_id].out_sck = out_fd;
	pthread_mutex_unlock(&mux_pool);
}

/** Riporta un worker in fase di attesa
 *
 * 	\param [in] my_id l'indice nel pool di thread del worker
 * 	\param [out] user la stringa contente l'utente, da resettare
 * 	\param [out] pass la stringa contente la password, da resettare
 * 	\param [out] sck la stringa contente il socket, da resettare
 * 	\param [out] buff la stringa contente il buffer del messaggio in ingresso, da resettare
 * 	\param [out] sig vale TRUE se si ci sta disattivando in seguito ad un segnale (in tal caso non chiudiamo il socket in uscita), FALSE negli altri casi
 *
 * 	\internal Acquisisce la mutua esclusione sul pool di thread e la rilascia successivamente
 *
 * 	Chiude le socket aperte, e riporta i valori dei file descriptor al loro valore di default (-1)
 */
void goingBackToSleep(int my_id, char *user, char *pass, char *sck, char *buff, int sig){

	/* Reimposto il thread in modalita' dormiente */
	memset(user, '\0', LUSERNAME);
	memset(pass, '\0', LUSERNAME);
	memset(sck, '\0', LUSERNAME+MAX_PID_CHAR);
	memset(buff, '\0', BUFF_SIZE);
	pthread_mutex_lock(&mux_pool);

	pool[my_id].active = 0;
	if (sig == FALSE)
		pool[my_id].sentreqs = 0;
	if (pool[my_id].in_sck != -1)
	{
		closeSocket(pool[my_id].in_sck);
		pool[my_id].in_sck = -1;
	}
	/* Se sig vale true, desidero lasciare il socket aperto,
	 * per poter mandare l'ultimo messaggio
	 */
	if (pool[my_id].out_sck != -1 && sig == FALSE)
	{
		closeSocket(pool[my_id].out_sck);
		pool[my_id].out_sck = -1;
	}
	pthread_mutex_unlock(&mux_pool);
	print_1("[ WORKER ] %d: Back to sleep", my_id)
}

/** Si sospende in attesa di essere attivato
 *
 * 	\param [in] my_id l'indice nel pool di thread del worker
 * 	\param [out] receive_fd conterra' il valore del socket in entrata assegnato al worker
 *
 * 	\internal Acquisisce la mutua esclusione sul pool di thread e la rilascia successivamente
 *	\internal Si ferma su una pthread_cond_wain in attesa di essere risvegliato
 */
void waitForActivation(int my_id, int *receive_fd){

	pthread_mutex_lock(&mux_pool);
	while(pool[my_id].active == 0) pthread_cond_wait(&(pool[my_id].cond), &mux_pool);
	*receive_fd = pool[my_id].in_sck;
	pthread_mutex_unlock(&mux_pool);
}

/** Funzione relativa al worker
 *
 * 	Il worker si mette in attesa di essere risvegliato
 * 	Appena risvegliato cerca di autenticare l'utente che si sta collegando
 * 	Una volta verifica la correttezza dell'utente avvia una sessione di messaggi con il client
 * 	Appena e' terminata tale sessione il thread torna in fase di attesa
 *
 * 	Per maggiori informazioni leggere la relazione
 *
 * 	\param [in] me puntatore all'intero che contiene il numero progressivo nel pool di thread
 *
 */
void * worker_thread(void * me){

	sigset_t set;
	int my_id, receive_fd = -1, send_fd = -1, verif, byte, place;
	char user[LUSERNAME], pass[LUSERNAME], sck[LUSERNAME+MAX_PID_CHAR];
	char departure[LLABEL], arrival[LLABEL];
	message_t in_message, out_message;
	char buff[BUFF_SIZE];
	
	/* Inizializzazione */
	memset(user, '\0', LUSERNAME);
	memset(pass, '\0', LUSERNAME);
	memset(sck, '\0', LUSERNAME+MAX_PID_CHAR);
	memset(buff, '\0', BUFF_SIZE);
	out_message.buffer = buff;
	in_message.buffer = NULL;
	/* Estraggo il mio ID (numero progressivo nel pool) */
	my_id = *((int *)me);

	ec_neg1_c(sigemptyset(&set), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGUSR1), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGTERM), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGINT), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGPIPE), "Signal Handling", pthread_exit(NULL); )
	ec_zero_c(pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling", pthread_exit(NULL); )


	sys_print_1("[ WORKER ] %d: Succesfully started\n", my_id)
	while(1)
	{
		waitForActivation(my_id, &receive_fd);

		if (getFlagSig() == 1)
			break;
		print_1("[ WORKER ] %d: Start serving someone\n", my_id);

		errno = 0;
		if( (byte = receiveMessage(receive_fd, &in_message)) == -1){
			if (errno == ENOTCONN)
				goingBackToSleep(my_id, user, pass, sck, buff, FALSE);
			if (errno == EINTR)
				break;
		}

		if (extractLoginData(&in_message, user, pass, sck) == -1){
			err_print_1("[ WORKER ] %d: Received not a login message, back to sleep\n", my_id)

			/* Il messaggio e' malformato, si ignora l'utente */
			if(in_message.length != 0) free(in_message.buffer);
			goingBackToSleep(my_id, user, pass, sck, buff, FALSE);
			continue;
		}
		if(in_message.length != 0) free(in_message.buffer);

		verif = isCorrectUser(user, pass);
		switch (verif) {
			case USER_NOT_FOUND:
				addNewUser(user, pass);
				/* Qui si dovrebbe controllare se la malloc e' andata a cattivo fine
				 * if (addNewUSer(user, pass) == -1)
				 */
				print_2("[ WORKER ] %d: User %s successfully added to known user\n", my_id, user);
				setOkMessage(&out_message);
				break;
			case USER_CORRECT:
				print_2("[ WORKER ] %d: User %s successfully logged in\n", my_id, user);
				loginUser(user);
				setOkMessage(&out_message);
				break;
			default:
				print_2("[ WORKER ] %d: User %s incorrect data\n", my_id, user);
				setErrorMessage(verif, &out_message);
				break;
		}

		/* Caso in cui non riesco a rispondere al messaggio */
		errno = 0;
		if ((send_fd = openConnection(sck)) == -1 || sendMessage(send_fd, &out_message) == -1)
		{
			if (verif == USER_NOT_FOUND || verif == USER_CORRECT)
				/* Se non riesco a collegarmi alla socket del client
				 * Devo sloggare l'utente altrimenti non si puo' piu' ricollegare */
				logoutUser(user);
			/* Qui non so a quale funzione e' relativo errno
			 * Ma se vale EINTR vuol dire che una delle 2 funzioni
			 * e' stata interrotta da un segnale, allora devo terminare.
			 */
			if (errno == EINTR)
				break;
			goingBackToSleep(my_id, user, pass, sck, buff, FALSE);
			continue;
		}

		if (verif != USER_NOT_FOUND && verif != USER_CORRECT){
			goingBackToSleep(my_id, user, pass, sck, buff, FALSE);
			continue;
		}

		/* Imposto il valore del file descripto della socket in uscita */
		setMyOutSck(my_id, send_fd);

		/* Sessione interattiva di messaggi */
		while(1)
		{
			errno = 0;
			if (receiveMessage(receive_fd, &in_message) == -1)
			{
				if (errno == ENOTCONN)
					print_2("[ WORKER ] %d: User %s closed connection\n", my_id, user);
				break;
			}

			print_1("[ WORKER ] %d: Received a new message\n", my_id);
			if (in_message.type == MSG_EXIT)
			{
				print_2("[ WORKER ] %d: User %s sent exit message\n", my_id, user);
				break;
			}

			verif = checkValidMessage(&in_message, departure, arrival, &place);

			if (verif == MESSAGE_CORRECT)
			{
				setOkMessage(&out_message);
				/* Bisogna gestire l'errore sulla malloc*/

				print_1("[ WORKER ] %d: Message have been enqueued\n", my_id);

				/* Se la malloc non e' andata a buon fine si inviare error message */
				if (appendReqOff(&in_message, departure, arrival, place, send_fd, user) == -1)
				{
					/* Con -2 setto l'errore sconosciuto */
					setErrorMessage(-2, &out_message);
				}
				else
				{
					if(in_message.type == MSG_REQUEST)
					{
						sys_print_1("[ WORKER ] %d: Increased number of sent requests\n", my_id);
						increaseRequests(my_id);
					}
				}
			}
			else
				setErrorMessage(verif, &out_message);

			/* Libero il buffer dove ho ricevuto il messaggio */
			if (in_message.length != 0) free(in_message.buffer);

			print_2("[ WORKER ] %d: Sending message %c\n", my_id, out_message.type);

			errno = 0;
			if (sendMessage(send_fd, &out_message) == -1)
			{
				if (errno == ENOTCONN)
					print_2("[ WORKER ] %d: User %s closed connection\n", my_id, user);
				break;
			}
		}
		logoutUser(user);
		print_2("[ WORKER ] %d: User %s succesfully logged out\n", my_id, user);

		if (getFlagSig() == 1)
		{
			/* Devo terminare e potrei dover lasciare il socket in uscita aperto
			 * Dipende dal fatto di aver inviato richieste o meno */
			goingBackToSleep(my_id, user, pass, sck, buff, TRUE);
			break;
		}
		else
			goingBackToSleep(my_id, user, pass, sck, buff, FALSE);

	}
	print_1("[ WORKER ] %d: Exiting...\n", my_id);
	pthread_exit(NULL);
}




/* =============================================
 * THREAD MAIN
 * =============================================
 */

/** Prova ad avviare un thread.
 * 	Nel caso in cui non vi siano sufficienti risorse attende per DELAY secondi e prova di nuovo
 * 	ad avviarlo per MAX_TRY volte
 *
 * 	\param [in, out] t il TID del thread avviato
 * 	\param [in] routine la funzione che indica il thread da avviare
 * 	\param [in] arg il parametro della funzione
 * 	\param [in] message Nome del thread, in modo da poter scrivere a video l'errore
 *
 * 	\retval 0 se il thread e' stato avviato correttamente
 * 	\retval -1 se non si e' riusciti ad avviare il thread
 */
int tryStartThread(pthread_t *t, void *(*routine) (void *), void *arg, char *message){

	int count = 1;
	errno = 0;
    while(pthread_create(t, NULL, routine, arg) != 0 && errno == EAGAIN && count <= MAX_TRY)
	{
    	errno = 0;
		err_print_1("No enough resource for starting %s, retry soon...",message)
		count++;
		sleep(DELAY);
	}
    if (count > MAX_TRY)
    	return -1;

    return 0;
}

/** Avvia tutti gli worker del pool di thread
 *
 *	\retval -1 se non si e' riusciti ad avviare un thread
 *	\retval 0 negli altri casi
 *
 *	\internal Acquisisce la mutua esclusione sul pool di thread e la rilascia successivamente
 *
 *	Gli worker vengono avviati con tryStartThread
 *	\see tryStartThread
 *
 */
int startWorker(){

	int i, k;

	/* Si deve utilizzare una variabile globale per passare il numero progressivo,
	 * altrimenti il parametro non viene passato correttamente
	 *
	 * Si usa dunque l'array di interi num
	 */
	pthread_mutex_lock(&mux_pool);
	for(i=0; i<POOL_SIZE; i++){
		num[i] = i;
		k = tryStartThread( &(pool[i].tid), worker_thread, (void *)(num+i), "Worker");
		if (k == -1)
		{
			pthread_mutex_unlock(&mux_pool);
			return -1;
		}
	}
	pthread_mutex_unlock(&mux_pool);
	return 0;
}

/** Gestore vuoto per inibire la gestione di default di SIGUSR1
 */
void dummy(){
	return;
}

/** Gestore dei segnali di sistema (SIGSEGV, etc...)
 *  Visualizza un messaggio di errore e termina immediatamente il processo
 *
 *  \param [in] sig numero del segnale
 */
static void err_handler(int sig){

	write(STDERR_FILENO, "Critical error! Now exiting...\n", 31);
	_exit(sig);
}

/** Stampa l'ambiente d'esecuzione del server
 */
void printEnv(){

	print("------------------------------------------\n")
	print("\t Server environment\n")
	print("------------------------------------------\n")
	print_1("SOCKET_PATH = %s\n", SOCKET_PATH)
	print_1("NTRIALCONN = %d\n", NTRIALCONN)
	print_1("SEC_TIMER = %d\n", SEC_TIMER)
	print_1("NSEC_TIMER = %d\n", NSEC_TIMER)
	print_1("LOG_FILE_NAME = %s\n", LOG_FILE_NAME)
	print_1("POOL_SIZE = %d\n", POOL_SIZE)
	print_1("MAX_TRY = %d\n", MAX_TRY)
	print_1("DELAY = %d\n", DELAY)
#ifdef UPDATE_OFFER
	print("UPDATE_OFFER = ON\n")
#else
	print("UPDATE_OFFER = OFF\n")
#endif
#ifdef FREE_OFFER_LIST
	print("FREE_OFFER_LIST = ON\n")
#else
	print("FREE_OFFER_LIST = OFF\n")
#endif
#ifdef VERBOSE
	print("VERBOSE = ON\n")
#else
	print("VERBOSE = OFF\n")
#endif
	print("------------------------------------------\n")
}


/** Funzione relativa al thread main (listener)
 *
 * 	Il main, dopo aver correttamente inizializzato il server,
 * 	avvia i vari thread del server, e si mette in attesa di connessioni in entrata
 *
 *  Per maggiori dettagli sul thread main leggere la relazione
 *
 */
int main(int argc, char* argv[]){

	char *nodes_file, *arcs_file;
	int server_sock = -1, receive_fd = -1, next;
	FILE *fnode, *farc;

	sigset_t set;
	struct sigaction act;

	sys_print("[ MAIN ] --- SERVER SUCCESSFULLY STARTED ---\n")
	sys_print("[ MAIN ] Blocking al signal...\n")
	/* Blocco tutti i segnali per l'inizializzazione */
	ec_neg1( sigfillset(&set) , "Signal Handling")
	ec_zero( pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling")

	sys_print("[ MAIN ] Initializing thread pool...\n")
	/* Inizializzazione */
	initializePool();

	/* Gestione dei Segnali */
	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	sys_print("[ MAIN ] Ignoring SIGPIPE...\n")
	ec_neg1( sigaction(SIGPIPE, &act, NULL), "Signal Handling")

	sys_print("[ MAIN ] Setting up signal handler for system signals...\n")
	act.sa_handler = err_handler;
	ec_neg1( sigaction(SIGBUS, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGSEGV, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGILL, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGSYS, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGXCPU, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGXFSZ, &act, NULL), "Signal Handling")

	sys_print("[ MAIN ] Setting up empty signal handler for INT TERM USR1 USR2...\n")
	act.sa_handler = dummy;
	ec_neg1( sigemptyset(&act.sa_mask) , "Signal Handling")
	ec_neg1( sigaddset(&act.sa_mask, SIGINT) , "Signal Handling")
	ec_neg1( sigaddset(&act.sa_mask, SIGTERM) , "Signal Handling")
	ec_neg1( sigaddset(&act.sa_mask, SIGUSR1) , "Signal Handling")
	ec_neg1( sigaddset(&act.sa_mask, SIGUSR2) , "Signal Handling")
	ec_neg1( sigaction(SIGINT, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGTERM, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGUSR1, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGUSR2, &act, NULL), "Signal Handling")

	/* Blocco solo i segnali che non voglio ricevere */
	sys_print("[ MAIN ] Setting up main thread signal mask...\n")
	ec_neg1( sigemptyset(&set), "Signal Handling" )
	ec_neg1( sigaddset(&set, SIGINT) , "Signal Handling")
	ec_neg1( sigaddset(&set, SIGTERM) , "Signal Handling")
	ec_neg1( sigaddset(&set, SIGUSR1) , "Signal Handling")
	ec_zero( pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling")

	if (argc != 3)
	{
		err_print("\n\n\tERROR: Invalid Argument\nUsage: mgcars <name city file> <arc file>\n")
		exit(EINVAL);
	}
	nodes_file = argv[1];
	arcs_file = argv[2];

	/* Messaggio di Benvenuto e stampa ambiente*/
	print("\n\t *** WELCOME ***\n")
	print("\nThis is 'mgcars' a multithread server for cars sharing sistem\n")
	printEnv();

	/* Apertura dei File e caricamento della mappa */
	sys_print("[ MAIN ] Opening files...\n")
    ec_null( (fnode = fopen(nodes_file,"r")) , "Opening Nodes File" )
    ec_null( (farc = fopen(arcs_file,"r")) , "Opening Arc File")

    print("[ MAIN ] Loading graph from files...\n");
    ec_null_c( (map = load_graph(fnode, farc)), "Loading Map", fclose(fnode); fclose(farc); exit(EXIT_FAILURE); )

	print("[ MAIN ] Graph succesfully loaded...\n");
#ifdef VERBOSE
    print("[ MAIN ] Printing Graph...\n");
    print_graph(map);
#endif
    fclose(fnode);
    fclose(farc);


    /* Creazione del file di log */
    print("[ MAIN ] Creating log file...\n");
    ec_neg1( (log_fd = open(LOG_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, PERM_FILE)), "Creating log file")

    /* Mi salvo il TID del thread main */
    listener = pthread_self();

    /* Avvio dei thread */
    print("[ MAIN ] Now starting Workers...\n");
    ec_neg1( startWorker(), "Starting Workers" )
    print_1("[ MAIN ] %d Workers successfully started...\n", POOL_SIZE);

    sys_print("[ MAIN ] Now starting Signal Handler...\n");
    ec_neg1( tryStartThread(&handler, signal_handler, NULL, "Signal Handler"), "Starting signal handler")
    print("[ MAIN ] Signal Handler successfully started...\n");

    /* Sblocco il segnale SIGUSR2 */
    sys_print("[ MAIN ] Unblocking SIGUSR2 for main thread...\n");
    ec_neg1( sigdelset(&set, SIGUSR2), "Signal Handling" )
    ec_zero( pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling")

    sys_print("[ MAIN ] Now starting Match...\n");
    ec_neg1( tryStartThread(&matcher, match, NULL, "Match"), "Starting match handler")
    print("[ MAIN ] Match successfully started...\n");

    ec_neg1( (server_sock = createServerChannel(SOCKET_PATH)), "Creating Socket")
    sys_print_1("[ MAIN ] Socket created on %s...\n", SOCKET_PATH)
    printf("\n\n\t *** SERVER READY ***\n\n");

    /* Inizializzo la funzione che fa lo scheduling */
    findActivateThread(-1, TRUE);
    while(1)
    {
    	errno = 0;
    	if((receive_fd = acceptConnection(server_sock)) == -1 && errno == EINTR)
    	{
    		sys_print("[ MAIN ] Accept interrupt by signal...\n");
    		break;
    	}
    	print("[ MAIN ] New request for connection...\n");
		next = findActivateThread(receive_fd, FALSE);

    	if (next == -1)
    	{
    		/* Chiudo e non voglio far sapere che sono gia' sovraccarico */
    		print("[ MAIN ] No enough worker, closing socket\n");
    		closeSocket(receive_fd);
    	}
    	else
    		print_1("[ MAIN ] Scheduled to %d\n", next);

    	if (getFlagSig() == 1)
    		break;
    }

    sys_print("[ MAIN ] Waiting signal handler thread\n")
    pthread_join(handler, NULL);
    sys_print("[ MAIN ] Freeing user list\n")
    list = freeUserList();
    sys_print("[ MAIN ] Closing socket\n")
    closeSocket(server_sock);
    sys_print("[ MAIN ] Removing socket file\n")
    (void)unlink(SOCKET_PATH);
    sys_print("[ MAIN ] Freeing Graph\n")
    free_graph(&map);
    sys_print("[ MAIN ] Closing log file\n")
    close(log_fd);
    sys_print("[ MAIN ] Exiting...\n")
    print("[ MAIN ] --- SERVER SUCCESSFULLY DOWN ---\n\n");
	exit(EXIT_SUCCESS);
}
