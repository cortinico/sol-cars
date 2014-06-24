/** \file docars.c
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

#include "comsock.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>

/*
 * Macro definite per l'elaborazione interna del client
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
/* ***************** */

/* -----------------------------------
 * VARIABILI GLOBALI
 * -----------------------------------
 */

/**
 * \var sendfd
 * File descriptor della socket in uscita dal client
 * Di default settata a -1
 */
int sendfd = -1;

/**
 * \var sockfd
 * File descriptor della socket su cui si aspettano connessioni entranti dal server
 * Di default settata a -1
 */
int sockfd = -1;

/**
 * \var receivefd
 * File descriptor della socket in entrata al client
 * Di default settata a -1
 */
int receivefd = -1;

/**
 * \var message_sem
 * Flag relativo ad un messaggio inviato
 * Vale 0 se non si e' inviato alcun messaggio
 * Vale 1 se si sta attendendo un esito del messaggio appena inviato
 *
 * \internal Legata alla variabili mux e cond
 * Di default settata a 1
 */
int message_sem = 1;

/**
 * \var login
 * Puntantore alla stringa che conterra' il messaggio per il login
 * Di default settata a NULL
 */
char *login = NULL;

/**
 * \var socket_name
 * Stringa utilizzata per salvare il nome della socket creata dal client
 * Di default settata a -1
 */
char socket_name[LUSERNAME+MAX_PID_CHAR];

/**
 * \var buffer
 * Stringa utilizzata per la lettura da standard input
 */
char buffer[BUFF_SIZE];

/** \var out_message
 * Struttura contentente in messaggio in uscita */
message_t out_message;

/** \var in_message
 * Struttura contentente in messaggio in entrata */
message_t in_message;

/** \var received_signal
 * 	Flag che viene settato a 1 se e' stato ricevuto
 * 	un messaggio di SIGINT, SIGTERM.
 * 	Di default vale 0
 *
 * 	\internal Tipo volatile sig_atomic_t per tralasciare la gestione della mutua esclusione
 */
volatile sig_atomic_t received_signal = 0;

/** \var sharend_sem
 * 	Flag che viene utilizzato per la gesitone degli SHARE_END
 *
 * 	Di default vale 0
 * 	Viene settato ad 1 se e' stato inviato un messaggio di richiesta
 * 	Viene settato a 2 se tale messaggio e' stato inviato
 * 	Viene resettato appena si riceve uno SHARE_END
 *
 * 	\internal Tipo volatile sig_atomic_t per tralasciare la gestione della mutua esclusione
 */
volatile sig_atomic_t sharend_sem = 0;

/** \var end_prompt
 * 	Flag che viene utilizzato per la gesitone della chiusura della sessione
 *
 * 	Di default vale 0
 * 	Viene settato ad 1 se e' stato ricevuto un EOF o e' stato digitato un messaggio di uscita
 * 	Viene utilizzato per capire quando terminare l'esecuzione
 *
 * 	\internal Tipo volatile sig_atomic_t per tralasciare la gestione della mutua esclusione
 */
volatile sig_atomic_t end_prompt = 0;

/** \var mux
 * 	Utilizzata per la mutua esclusione fra i 2 thread del client sulla variabile message_sem
 *
 * 	\see message_sem */
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

/** \var cond
 * 	Utilizzata per la mutua esclusione fra i 2 thread del client sulla variabile message_sem
 *
 * 	\see message_sem */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* ***************** */

/* -----------------------------------
 * Funzioni
 * -----------------------------------
 */

/** Verifica se una stringa e' composta solamente da caratteri, numeri, spazi, ':', oppure '\n'
 *
 *	\param [in] string Puntatore alla stringa da esaminare
 *	\retval -1 se la stringa e' NULL, oppure non e' soddisfa i parametri sopra indicati
 *	\retval 0 se la stringa soddisfa i parametri sopra indicati
 */
int isAlphaNumDots(char *string){
	int i, n;
	if (string == NULL)
		return -1;
	n = strlen(string);
	for(i=0; i<n; i++){
		if(!(isalnum(string[i]) || string[i] == ':' || string[i] == ' ' || string[i] == '\n')) return 0;
	}
	return 1;
}

/** Verifica se una stringa e' composta solamente da  numeri, oppure '\n'
 *
 *	\param [in] string Puntatore alla stringa da esaminare
 *	\retval -1 se la stringa e' NULL, oppure non e' soddisfa i parametri sopra indicati
 *	\retval 0 se la stringa soddisfa i parametri sopra indicati
 */
int isNumeric(char *string){
	int i, n;
	if (string == NULL)
		return -1;
	n = strlen(string);
	for(i=0; i<n; i++){
		if(!(isdigit(string[i]) || string[i] == '\n')) return 0;
	}

	return 1;
}

/**
 * Controlla se la stringa input contiene un messaggio valido tra quelli stabiliti nel protocollo di
 * comunicazione fra client e server.
 *
 * \param [in] input la stringa in ingresso
 * \retval INCORRECT_MESSAGE se il messaggio e' scorretto
 * \retval EXIT_MESSAGE se il messaggio e' un messaggio di uscita
 * \retval HELP_MESSAGE se il messaggio e' un messaggio di aiuto
 * \retval REQ_MESSAGE se il messaggio e' un messaggio di richiesta
 * \retval OFF_MESSAGE se il messaggio e' un messaggio di offerta
 */
int messageParser(char *input){

	char *aux, *aux2;

	if (input == NULL)
		return INCORRECT_MESSAGE;

	if(strcmp(input, "%EXIT\n") == 0) return EXIT_MESSAGE;
	if(strcmp(input, "%HELP\n") == 0) return HELP_MESSAGE;
	if ((aux = strchr(input, ':')) == NULL) return INCORRECT_MESSAGE;

	if( (input[0]=='%') && (input[1]=='R') && (input[2] == ' '))
	{
		if ( (aux2 = strchr(aux+1, ':')) != NULL || aux == (input+3) || !isAlphaNumDots(input+3))
			return INCORRECT_MESSAGE;
		else
			return REQ_MESSAGE;
	}
	if ((aux2 = strchr(aux+1, ':')) == NULL) return INCORRECT_MESSAGE;
	if ( isAlphaNumDots(input) && aux2 != aux+1 && aux!=input && isNumeric(aux2+1))
		return OFF_MESSAGE;

	return INCORRECT_MESSAGE;

}

/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di login valido
 *
 * \param [in] login Una stringa che contiene il messaggio di login
 * \param [in] size La dimensione di tale stringa
 * \param [out] dest Il messaggio di destinazione
 *
 * \retval -1 se size <= 0 oppure login vale NULL
 */
int setLoginMessage(char *login, int size, message_t *dest){

	if (size <= 0 || login == NULL)
		return -1;

	dest->type = MSG_CONNECT;
	dest->length = size;
	
	memcpy(dest->buffer, login, size);
	return size;
}

/** Crea la stringa per il login da inviare in un messaggio al server
 *
 * \param [in] user la strina contenente il nome utente
 * \param [in] pass la strina contenente la password
 * \param [in] sck la strina contenente il nome della socket
 * \param [out] dest la stringa contente il messaggio di login generato (deve essere gia' allocata)
 *
 * \retval -1 se size <= 0 oppure login vale NULL
 */
void createLoginString(char *user, char *pass, char *sck, char *dest){

	char *aux = dest;

	sprintf(aux, "%s", user);
	aux = (strchr(dest,'\0') + 1);
	sprintf(aux, "%s", pass);
	aux = (strchr(aux,'\0') + 1);
	sprintf(aux, "%s", sck);
}

/** Crea il nome della socket del client, contatenando il nome utente al pid del processo
 *
 * \param [in] user la strina contenente il nome utente
 * \param [out] dest la stringa contente il nome della socket generato (deve essere gia' allocata)
 *
 */
void createSocketName(char *user, char *dest){
	memset(dest, '\0', LUSERNAME+MAX_PID_CHAR);
	sprintf(dest, "./tmp/%s%d", user, getpid());
}

/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di exit valido
 *
 * \param [out] mess Il messaggio
 */
void setExitMessage(message_t *mess){

	mess->type = MSG_EXIT;
	mess->length = 0;
}

/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di offerta valido
 *
 * \param [in] input Una stringa che contiene l'offerta da inviare
 * \param [in, out] mess Il messaggio generato, il cui campo buffer deve essere stato gia' allocato
 *
 * \attention Si generera' un errore se il campo buffer del messaggio non e' stato allocato precentemente
 */
void setOfferMessage(char *input, message_t *mess){

	char *aux;
	memset(mess->buffer, '\0', BUFF_SIZE);
	if ((aux = strchr(input, '\n')) != NULL) *aux = '\0';

	mess->type = MSG_OFFER;
	mess->length = strlen(input) + 1;
	strcpy(mess->buffer, input);
}

/** Imposta un messaggio di tipo message_t in modo che sia un messaggio di richiesta valido
 *
 * \param [in] input Una stringa che contiene la richiesta da inviare
 * \param [in, out] mess Il messaggio generato, il cui campo buffer deve essere stato gia' allocato
 *
 * \attention Si generera' un errore se il campo buffer del messaggio non e' stato allocato precentemente
 */
void setRequestMessage(char *input, message_t *mess){

	char *aux;
	memset(mess->buffer, '\0', BUFF_SIZE);
	if ((aux = strchr(input, '\n')) != NULL) *aux = '\0';

	mess->type = MSG_REQUEST;
	mess->length = strlen(input) - 2;
	strcpy(mess->buffer, input+3);
}

/** Stampa un messaggio arrivato al client
 *
 * \param [in] mess il messaggio da stampare
 */
void printIncomingMessage(message_t *mess){
	if (mess->type == MSG_OK)
		print("OK\n");
	if (mess->type == MSG_NO)
		err_print_1 ("MESSAGE REFUSED - %s\n", mess->buffer);
	if (mess->type == MSG_SHARE)
		print_1("\n\t***Broadcast message***: Your request has been served: %s\n", mess->buffer);
	if (mess->type == MSG_SHAREND)
		print("\n\t***Broadcast message***: No more request from server\n");
}

/** Stampa alcuni messaggio di aiuto per l'utente
 */
void print_help(void){
	printf("-- This is 'docars' an interactive client for cars sharing server --\n");
	printf("-- Allowed command:\n");
	printf("\t%%R <start>:<arrival> - To send to server a new request\n");
	printf("\t<start>:<arrival>:<place> - To send to server a new offer\n");
	printf("\t%%EXIT - To disconnect from the server and close the client\n");
	printf("\t%%HELP - To show this help\n");
	fflush(stdout);
}

/** Stampa un messaggio di errore dicendo che il messaggio e' invalido
 */
void print_invalid_command(void){
	err_print("The message you typed is incorrect. If you need help type %%HELP\n");
}

/** Funzione di cleanup del listner
 * 	Invocata tramite una ptread_cleanup_push/pop
 */
static void listen_cleanup(){

	message_sem = 0;
	pthread_mutex_unlock(&mux);
	pthread_cond_signal(&cond);
}

/** Aggiorna il flag message_sem per indicare che il listener
 * 	e' pronto a ricevere messaggi in ingresso, ed il thread main
 * 	puo' essere risvegliato
 *
 * 	\see message_sem
 */
void startReceiving(){
	pthread_mutex_lock(&mux);
		message_sem = 0;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mux);
}

/** Resetta il flag sharend_sem se necessario
 *
 * 	\see sharend_sem
 */
void updateSharEndSem(){
	if (sharend_sem == 2) sharend_sem = 0;
}

/** Cambia i valori dei flag per l'interazione dei 2 thread e risveglia il main
 *
 * 	\see message_sem
 * 	\see sharend_sem
 */
void updateMessageSem(message_t *in_message){
	pthread_mutex_lock(&mux);

	if(sharend_sem == 1 && in_message->type == MSG_OK) sharend_sem = 2;
	if(message_sem == 1 && (in_message->type == MSG_OK || in_message->type == MSG_NO) ) message_sem = 0;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mux);
}

/** Indica se il thread listener deve terminare
 *
 * 	\retval 1 se il thread listener deve terminare
 * 	\retval 0 se il thread listener non deve terminare
 */
int goingToExit(){

	/* Caso sessione interattiva terminata e sharend finiti */
	if(sharend_sem != 2 && end_prompt == 1)
		return 1;
	/* Caso ricevuto segnale di INT/TERM */
	if(received_signal == 1)
		return 1;

	return 0;
}

/** Thread che esegue l'ascolto dei messaggi in ingresso
 *
 *  Il thread sta in ascolto dei messaggi in arrivo dal server e li stampa a video.
 *  Inoltre opera in modo sincrono con il thread main mediante meccanismi di mutua esclusione
 *
 * 	Per una descrizione dettagliata del suo comportamento, si faccia riferimento alla relazione
 *
 */
void * listen_thread(){

	int bytes = 0;
	sigset_t set;

	ec_neg1_c(sigemptyset(&set), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGTERM), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGINT), "Signal Handling", pthread_exit(NULL); )
	ec_neg1_c(sigaddset(&set, SIGQUIT), "Signal Handling", pthread_exit(NULL); )
	ec_zero_c(pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling", pthread_exit(NULL); )

	pthread_cleanup_push(listen_cleanup, NULL);

	sys_print("-- LISTNER -- Start receiving message\n")
	startReceiving();

	while(1)
	{
		errno = 0;
		bytes = receiveMessage(receivefd, &in_message);
		if (bytes == -1 && errno == ENOTCONN)
		{
			print("-- LISTENER -- Server closed connection!\n");
			kill(getpid(), SIGINT);
			break;
		}

		/* NOTA BENE:
		 *
		 * Non devo uscire tutte le volte che errno vale EINTR, bensi' tutte le
		 * volte che received_signal vale 1
		 *
		 * errno potrebbe valere EINTR se ho ricevuto SIGUSR1 dal thread main
		 * In tal caso vuol dire che potrei dover aspettare un ultimo sharend
		 *
		 * In questo caso specifico voglio uscire solo se errno vale EINTR e end_prompt,
		 * ovvero se e' stato ricevuto un segnale, ma la sessione non e' terminata
		 * (cio' vuol dire che non e' stato ricevuto SIGUSR1 da parte di thread main).
		 */
		if (bytes == -1 && errno == EINTR && (end_prompt == 0 || received_signal == 1))
			break;

		if (bytes != -1) printIncomingMessage(&in_message);
		if (bytes != -1 && in_message.length != 0) free(in_message.buffer);


		if(bytes != -1 && in_message.type == MSG_SHAREND)
			updateSharEndSem();

		if(bytes != -1 && (in_message.type == MSG_OK || in_message.type == MSG_NO))
			updateMessageSem(&in_message);

		if(goingToExit() == 1)
			break;
	}
	printf("-- Listener thread successfully closed\n");
	pthread_cleanup_pop(1);
	return (void *)0;
}

/** Setta il flag end_prompt a 1 in modo da indicare che la sessione interattiva e' terminata
 * 	Inoltre invia SIGUSR1 al thread listener
 */
void endPrompt(){
	end_prompt = 1;
	kill(getpid(), SIGUSR1);
}

/** Si mette in attesa su cond che il flag message_sem sia 0
 *  Una volta usciti da questa funzione si ha acquisito la mutua esclusione con mux
 */
void waitForReply(){
	pthread_mutex_lock(&mux);
	while(message_sem == 1) pthread_cond_wait(&cond, &mux);
}

/** Aggiora i valori dei flag per l'interazione fra i 2 thread
 *  e rilascia la mutua esclusione.
 *
 *  \param [in] pars Variabile che indica se il messaggio inviato era un'offerta o una richiesta
 */
void messageSent(int pars){
	if (pars == REQ_MESSAGE || pars == OFF_MESSAGE)
	{
			message_sem = 1;
			if(pars == REQ_MESSAGE && sharend_sem == 0) sharend_sem = 1;
	}
	pthread_mutex_unlock(&mux);
}

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

/**
 * Sblocca la mutua esclusione
 */
void unlock(){
	pthread_mutex_unlock(&mux);
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

/** Gestore dei segnali di terminazione (SIGINT, SIGTERM)
 *  Setta ad uno la variabile received_signal ed invia SIGUSR1 al thread
 *
 *  \param [in] sig numero del segnale
 *  \see received_signal
 */
static void sig_handler(int sig){
	received_signal = 1;
	kill(getpid(), SIGUSR1);
}

/** Funzione per il cleanup del processo
 *  Chiude tutte le socket e rimuove il file relativo alla socket
 *
 */
void cleanup(void){
	if (sendfd != -1) closeSocket(sendfd);
	if (sockfd != -1) closeSocket(sockfd);
	if (receivefd != -1) closeSocket(receivefd);
	(void)unlink(socket_name);
}

/** Gestore vuoto per inibire la gestione di default di SIGUSR1
 */
void dummy(){
	return;
}

/** Thread main
 * 	Gestisce l'interazione con l'utente e l'invio dei messaggi in uscita.
 *
 * 	Per una descrizione piu' dettagliata si faccia riferimento alla relazione
 *
 */
int main(int argc, char* argv[]){

	char *username, password[LUSERNAME], input[BUFF_SIZE], *aux;
	int login_size, parser_response;
	pthread_t listener;
	sigset_t set;
	struct sigaction act;

	/* Gestione dei segnali */
	sys_print("-- MAIN -- Starting un client\n")
	ec_neg1( sigfillset(&set), "Signal Handling" )
	ec_zero( pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling")

	sys_print("-- MAIN -- Ignoring SIGPIPE\n")
	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	ec_neg1( sigaction(SIGPIPE, &act, NULL), "Signal Handling")

	sys_print("-- MAIN -- Installing error handler\n")
	act.sa_handler = err_handler;
	ec_neg1( sigaction(SIGBUS, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGSEGV, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGILL, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGSYS, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGXCPU, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGXFSZ, &act, NULL), "Signal Handling")

	sys_print("-- MAIN -- Installing signal handler\n")
	act.sa_handler = sig_handler;
	ec_neg1( sigemptyset(&act.sa_mask) , "Signal Handling")
	ec_neg1( sigaddset(&act.sa_mask, SIGINT) , "Signal Handling")
	ec_neg1( sigaddset(&act.sa_mask, SIGTERM) , "Signal Handling")
	ec_neg1( sigaddset(&act.sa_mask, SIGQUIT) , "Signal Handling")
	ec_neg1( sigaction(SIGINT, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGTERM, &act, NULL), "Signal Handling")
	ec_neg1( sigaction(SIGQUIT, &act, NULL), "Signal Handling")

	sys_print("-- MAIN -- Installing dummy handler for SIGUSR1\n")
	act.sa_handler = dummy;
	ec_neg1( sigaction(SIGUSR1, &act, NULL), "Signal Handling")

	ec_neg1( sigemptyset(&set), "Signal Handling")
	ec_neg1( sigaddset(&set, SIGUSR1), "Signal Handling")
	ec_zero( pthread_sigmask(SIG_SETMASK, &set, NULL), "Signal Handling" )


	/* Inizializzazione del programma */
	memset(password, '\0', LUSERNAME+1);
	memset(buffer, '\0', BUFF_SIZE);
	out_message.buffer = buffer;
	in_message.buffer = NULL;
	atexit(cleanup);
	
	/* Verifica dei parametri in ingresso */
	if(argc != 2 || strlen(argv[1]) > LUSERNAME)
	{
		err_print_1("Invalid Argument\nUsage: docars <username> - Username max length: %d\n", LUSERNAME)
		exit(EINVAL);
	}
	username = argv[1];

	print("\n\n\t ***Welcome***\n")
	print_2("-- Hi %s, welcome to docars, a car sharing client\n-- Please enter your password... (max %d chars)\n", username, LUSERNAME);

	aux = fgets(password, LUSERNAME, stdin);
	
	if (aux == NULL || strlen(password) == 0 || password[LUSERNAME] != '\0')
	{
		err_print_1("-- ERROR -- Invalid password! Max length: %d\n", LUSERNAME)
		exit(EINVAL);
	}

	/* Creazione Sockets */
	print("-- Trying to establish server connection...\n");
	errno = 0;
	sendfd = openConnection(SOCKET_PATH);
	if(sendfd != -1)
		print("-- Connection with server successfully established\n")
	else
	{
		if (errno != EINTR)
			perror("-- ERROR -- Unable to connect to server\n");
		exit(errno);
	}

	sys_print("-- MAIN -- Creating socket name\n")
	createSocketName(username, socket_name);

	sys_print("-- MAIN -- Creating login string\n")
	login_size = (strlen(username) + strlen(password) + strlen(socket_name) + 3);
	ec_null( login = calloc( login_size, sizeof(char) ), "Allocating memory" )
	createLoginString(username, password, socket_name, login);	
	setLoginMessage(login, login_size, &out_message);

	sys_print("-- MAIN -- Creating input socket\n")
	ec_neg1( sockfd = createServerChannel(socket_name), "Creating input socket" )
	free(login);

	errno = 0;
	if (sendMessage(sendfd, &out_message) == -1)
	{
		if (errno == ENOTCONN || errno == EPIPE)
			err_print("-- ERROR -- Server closed connection!! Try again later...\n")
		if (errno == EINTR)
			print("-- Exiting...\n\n")
		exit(errno);
	}
	print("-- Login message sent to server, waiting for reply...\n");
	errno = 0;
	if ((receivefd = acceptConnection(sockfd)) == -1 || receiveMessage(receivefd, &in_message) == -1)
	{
		if (errno == ENOTCONN)
			err_print("-- ERROR -- Server closed connection!! Try again later...\n")
		if (errno == EINTR)
			print("-- Exiting...\n\n")
		exit(errno);
	}

	if (in_message.type != MSG_OK)
	{
		err_print_1("-- ERROR -- ACCESS DENIED: %s\n", in_message.buffer);
		exit(EXIT_FAILURE);
	}
	else
	{

		/* Parte l'esecuzione del listener */
		free(in_message.buffer);
	    ec_neg1( tryStartThread(&listener, listen_thread, NULL, "Listen thread"), "Starting listen thread")

		while(1)
		{
			/* Inizio sezione critica */

			if (received_signal == 1)
			{
				unlock();
				break;
			}

			/* Mi blocco in attesa di poter inviare messaggi
			 * Alla prima esecuzione, sara' il listener a risvegliarmi
			 */
			waitForReply();

			if (received_signal == 1)
			{
				unlock();
				break;
			}

			printf(PROMPT);
			fflush(stdout);

			memset(input, '\0', BUFF_SIZE+1);
			errno = 0;
			while ((aux = fgets(input, BUFF_SIZE, stdin)) != NULL)
			{
				if (errno != 0 || received_signal == 1 || input[BUFF_SIZE] == '\0')
					break;

				/* Caso in cui il messaggio sia piu' lungo di BUFF_SIZE
				 * Allora svuoto il buffer */
				while (input[BUFF_SIZE] != '\0')
				{
					err_print("-- ERROR -- Message is too long\n")
					memset(input, '\0', BUFF_SIZE+1);
					aux = fgets(input, BUFF_SIZE, stdin);
				}
				printf(PROMPT);
				fflush(stdout);
			}

			if (errno == EINTR || received_signal == 1)
			{
				unlock();
				break;
			}

			if (aux != NULL) parser_response = messageParser(input);
			else parser_response = EXIT_MESSAGE;

			switch (parser_response) {
				case HELP_MESSAGE:
					print_help();
					break;
				case EXIT_MESSAGE:
					setExitMessage(&out_message);
					endPrompt();
					break;
				case REQ_MESSAGE:
					setRequestMessage(input, &out_message);
					break;
				case OFF_MESSAGE:
					setOfferMessage(input, &out_message);
					break;
				case INCORRECT_MESSAGE:
					print_invalid_command();
					break;
				default:
					print_invalid_command();
					break;
			}
			
			if (parser_response == HELP_MESSAGE || parser_response == INCORRECT_MESSAGE)
			{
				unlock();
				continue;
			}
			if (parser_response == EXIT_MESSAGE)
			{
				printf("-- Client soon cloosing");
				if (sharend_sem == 2) printf(", waiting for SHAREND message from server...\n");
				else printf("\n");
				fflush(stdout);

				unlock();
				break;
			}
			errno = 0;
			if ((sendMessage(sendfd, &out_message)) == -1)
			{
				if (errno == ENOTCONN)
				{
					err_print("-- ERROR -- Server closed connection!\n");
					kill(getpid(), SIGUSR1);
					unlock();
					break;
				}
				if (errno == EINTR)
				{
					unlock();
					break;
				}
			}
			messageSent(parser_response);
			/* Fine sezione critica */

			print("-- Message sent, waiting for reply...");
			fflush(stdout);

			if (received_signal == 1) break;
		}

		sys_print("-- MAIN -- Waiting listner\n");
		pthread_join(listener, NULL);
		print("-- Exiting...\n\n");
	}
	exit(EXIT_SUCCESS);
}
