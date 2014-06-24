/**  \file 
 *   \brief header libreria di comunicazione socket AF_UNIX
 *
 *    \author lso11
*/

#ifndef _COMSOCK_H
#define _COMSOCK_H

/* -= TIPI =- */

/** <H3>Messaggio</H3>
 * La struttura \c message_t rappresenta un messaggio 
 * - \c type rappresenta il tipo del messaggio
 * - \c length rappresenta la lunghezza in byte del campo buffer
 * - \c buffer e' il puntatore al messaggio (puo' essere NULL se length == 0)
 *
 * <HR>
 */
typedef struct {
    char type;           /** tipo del messaggio */
    unsigned int length; /** lunghezza in byte */
    char* buffer;        /** buffer messaggio */
} message_t; 

/** lunghezza buffer indirizzo AF_UNIX */
#define UNIX_PATH_MAX    108


/** numero di tentativi di connessione da parte del client */
#define  NTRIALCONN 3

/** timeout per il calcolo dgl iaccoppiamenti da parte del server */
#define  SEC_TIMER 30

/** massima lunghezza consentita username */
#define  LUSERNAME 25

/** tipi dei messaggi scambiati fra server e client */
/** richiesta di connessione utente */
#define MSG_CONNECT      'C' 
/** accettazione */
#define MSG_OK           'K' 
/** rifiuto */
#define MSG_NO           'N' 
/** richiesta sharing */
#define MSG_REQUEST      'R' 
/** offerta sharing */
#define MSG_OFFER        'F' 
/** termina la fase interattiva di invio richieste/offerte da parte del client*/
#define MSG_EXIT         'X' 
/** proposta di sharing */
#define MSG_SHARE        'S' 
/** termina l'invio delle proposte di sharing da parte del server */
#define MSG_SHAREND     'H' 



/* -= FUNZIONI =- */
/** Crea una socket AF_UNIX
 *  \param  path pathname della socket
 *
 *  \retval s    il file descriptor della socket  (s>0)
 *  \retval -1   in altri casi di errore (setta errno)
 *               errno = E2BIG se il nome eccede UNIX_PATH_MAX
 *
 *  in caso di errore ripristina la situazione inziale: rimuove eventuali socket create e chiude eventuali file descriptor rimasti aperti
 */
int createServerChannel(char* path);

/** Chiude una socket
 *   \param s file descriptor della socket
 *
 *   \retval 0  se tutto ok, 
 *   \retval -1  se errore (setta errno)
 */
int closeSocket(int s);

/** accetta una connessione da parte di un client
 *  \param  s socket su cui ci mettiamo in attesa di accettare la connessione
 *
 *  \retval  c il descrittore della socket su cui siamo connessi
 *  \retval  -1 in casi di errore (setta errno)
 */
int acceptConnection(int s);

/** legge un messaggio dalla socket --- attenzione si richiede che il messaggio sia adeguatamente spacchettato e trasferito nella struttura msg
 *  \param  sc  file descriptor della socket
 *  \param msg  struttura che conterra' il messagio letto 
 *		(deve essere allocata all'esterno della funzione,
 *		tranne il campo buffer)
 *
 *  \retval lung  lunghezza del buffer letto, se OK 
 *  \retval  -1   in caso di errore (setta errno)
 *                 errno = ENOTCONN se il peer ha chiuso la connessione 
 *                   (non ci sono piu' scrittori sulla socket)
 *      
 */
int receiveMessage(int sc, message_t * msg);

/** scrive un messaggio sulla socket --- attenzione si richiede che il messaggio venga scritto con un'unica write dopo averlo adeguatamente impacchettato
 *   \param  sc file descriptor della socket
 *   \param msg struttura che contiene il messaggio da scrivere 
 *   
 *   \retval  n    il numero di caratteri inviati (se scrittura OK)
 *   \retval -1   in caso di errore (setta errno)
 *                 errno = ENOTCONN se il peer ha chiuso la connessione 
 *                   (non ci sono piu' lettori sulla socket)
 */
int sendMessage(int sc, message_t *msg);

/** crea una connessione all socket del server. In caso di errore funzione tenta NTRIALCONN volte la connessione (a distanza di 1 secondo l'una dall'altra) prima di ritornare errore.
 *   \param  path  nome del socket su cui il server accetta le connessioni
 *   
 *   \return fd il file descriptor della connessione
 *            se la connessione ha successo
 *   \retval -1 in caso di errore (setta errno)
 *               errno = E2BIG se il nome eccede UNIX_PATH_MAX
 *
 *  in caso di errore ripristina la situazione inziale: rimuove eventuali socket create e chiude eventuali file descriptor rimasti aperti
 */
int openConnection(char* path);

#endif
