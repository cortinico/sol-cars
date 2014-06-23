#include <stdlib.h>

#ifndef __SETTINGS_H
#define __SETTINGS_H

/*
 *	Questo file contiene le impostazioni consigliate per un computer
 *  di potenza ridotta.
 *
 *  Si noti che i valori sono puramente di consiglio, si consiglia di provarli
 *  e di modificarli al fine di trovare la combinazione piu' adatta
 *  
 *  Per utilizzare il file, copiarlo nella cartella src/ e rinominarlo
 *  in settings.h
 */



/* ===================== PARAMETRI DEL SISTEMA =====================
 *
 * Questo file contiene le impostazioni per il sistema Cars
 *
 * Le modifiche a questo file verranno rese effettive soltanto dopo una
 * ricompilazione di ENTRAMBI il client e il server.
 *
 * ATTENZIONE!
 * 		Avere dei client che sono stati compilati con una determinata
 * 		impostazione ed avere altri che sono stati compilati con impostazioni
 * 		differenti puo' portare ad un malfunzionamento del sistema
 *
 * ATTENZIONE!
 * 		Modificare i valori delle macro soltanto entro quanto indicato
 * 		dai commenti. Inserire valori differenti puo' portare ad un serio
 * 		malfunzionamento del sistema, o all'impossibilita' di ricompilare il codice.
 *
 * NOTA:
 * 		Nel caso in cui si volesse riportare lo stato alla condizione iniziale
 * 		e possibile recuperare il file settings.h contenuto nella cartella ../doc/settings
 */

/*
 * SOCKET_PATH
 *
 * Questa macro definisce il nome del file sul quale il server apre la sua socket
 * Inserire un path di file valido, altrimenti il server non potra' avviarsi, e i client
 * non potranno connettersi
 *
 * ATTENZIONE: rimuovere questa macro porta ad un'impossibilita' di installare il sistema
 *
 * DEFAULT "./tmp/cars.sck"
 *
 */
#define SOCKET_PATH "./tmp/cars.sck"

/*
 * NSEC_TIMER
 *
 * Questa macro definisce il numero di microsecondi da attendere prima di far partire il match.
 * Deve essere utilizzata in combinazione con la macro SEC_TIMER definita nel file comsock.h
 *
 * INFO: per computer poco potenti aumentare il tempo di attesa, in modo da non sovraccaricare
 * troppo il sistema. Per computer piu' potenti, e' possibile diminuire il valore. Si tenga di
 * conto che a valori minori corrispondono tempi di risposta minori per i client
 *
 * ATTENZIONE: evitare di settare entrambi le macro a 0, il thread match stara' constantemente
 * in esecuzione.
 *
 * ATTENZIONE: valori negativi di una di queste 2 macro possono portare ad un grave malfunzionamento
 *
 * DEFAULT 0
 *
 */
#define NSEC_TIMER 0

/*
 * LOG_FILE_NAME
 *
 * Questa macro definisce il nome del file sul quale il server salvera' il proprio log.
 *
 * ATTENZIONE: settare questa macro allo stesso valore della macro SOCKET_NAME porta ad grave
 * malfunzionamento
 *
 * ATTENZIONE: non settare questa macro a nomi di file gia' presenti (in particolar modo non settarla
 * a valori quali "./mgcars" o altri nomi di file importanti), dato che all'avvio del server tali file
 * vengono cancellati irrimediabilmente.
 *
 * ATTENZIONE: rimuovere questa macro porta ad un'impossibilita' di installare il sistema
 *
 * DEFAULT "./mgcars.log"
 *
 */
#define LOG_FILE_NAME "./mgcars.log"

/*
 * PERM_FILE
 *
 * Questa macro definisce i permessi di default con cui viene aperto il file di log.
 *
 * Il tipo con cui deve essere specificata deve essere lo stesso richiesto dalla svc
 * open (per info vedi open(2)).
 *
 * Si tenga di conto che il valore effettivo dei bit di protezione sara' il risultato
 * di (PERM_FILE & ~umask), per maggiori info si veda umask(2) e umask(1).
 *
 * ATTENZIONE: rimuovere questa macro porta ad un'impossibilita' di installare il sistema
 *
 * ATTENZIONE: se non si sta cosa si sta facendo e' consigliabile lasciare questa macro invariata.
 *
 * DEFAULT (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
 *
 */
#define PERM_FILE (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/*
 * POOL_SIZE
 *
 * Questa macro definisce la dimensione del pool di thread worker che devono essere
 * avviati all'avvio del sistema
 *
 * Rappresenta un valore molto importante ai fini delle prestazioni del sistema
 *
 * INFO: Un valore troppo piccolo porta ad un carico minore del sistema, ma puo' portare
 * a rifiutare alcune richieste nei confronti di qualche utente.
 *
 * INFO: Un valore troppo grande porta ad un carico maggiore del sistema, ma si riesce a
 * servire contemporaneamente piu' utenti.
 *
 * INFO: Si consiglia di provare alcuni valori e di cercare il valore adatto alla propria
 * infrastruttura.
 *
 * ATTENZIONE: rimuovere questa macro porta ad un'impossibilita' di installare il sistema
 *
 * ATTENZIONE: il valore di questa macro deve sempre essere maggiore di 0 (si consiglia
 * almeno 4) e non sufficientemente grande (si consiglia di non superare).
 * Nel caso si decida di aumentare il numero dei thread a numeri elevati, si consideri
 * il limite sul numero di thread imposto dal proprio sistema operativo
 *
 * DEFAULT 32
 *
 */
#define POOL_SIZE 4

/*
 * MAX_TRY
 * DELAY
 *
 * Queste macro definiscono il comportamento da seguire nel caso in cui non si riesca ad
 * avviare un thread.
 *
 * In caso di fallimento ad avviare un thread, si attenderanno DELAY secondi e
 * si riprovera' per MAX_TRY volte.
 *
 * INFO: Si consideri che il fatto di non riuscire ad avviare un thread per assenza di risorse
 * dovrebbe essere un caso piu' unico che raro, dato che, molto probabilmente, dopo DELAY secondi
 * non sara' comunque possibile avviare un nuovo thread.
 *
 * INFO: Se accade spesso questo evento, forse e' meglio considerare l'eventualita' di diminuire il
 * numero dei thread avviati.
 *
 * ATTENZIONE: rimuovere questa macro porta ad un'impossibilita' di installare il sistema
 *
 * ATTENZIONE: i valori di questa macro devo essere ragionevolmenti bassi, altrimenti posso portare
 * a lunghe attese del sistema.
 *
 * DEFAULT
 * 		MAX TRY 5
 * 		DELAY 1
 *
 */
#define MAX_TRY 3
#define DELAY 5

/*
 * PROMPT
 *
 * Queste macro definisce il prompt che deve essere visualizzato ogni volta che il client
 * attende un valore in input.
 *
 * Puo' essere personalizzato a piacimento.
 *
 * ATTENZIONE: se si decide di non visualizzare il prompt settare la macro a questo valore:
 * 		#define PROMPT ""
 * Si tenga pero' in considerazione che l'assenza del prompt potrebbe indurre in confusione l'utente
 * che non potrebbe comprendere quando deve inserire il messaggio.
 *
 * ATTENZIONE: Un valore troppo lungo potrebbe far visualizzare il prompt su piu' righe, rendendo
 * poco elegante la visualizzazione sul client.
 *
 * ATTENZIONE: rimuovere questa macro porta ad un'impossibilita' di installare il sistema
 *
 * DEFAULT "client> "
 *
 */
#define PROMPT "> "

/*
 * UPDATE_OFFER
 *
 * Queste macro, se definita, fa variare il comportamento del server, nel merito all'aggiornamento
 * delle richieste.
 *
 * Se questa macro e' settata (non commentata) il server elaborera' ogni cammino trovato e aggiornera'
 * le richieste dividendole in modo da poter utilizzare a pieno tutti i posti disponibili offerti
 * da un offerente
 *
 * Se questa macro non e' settata (commentata) il server non provvedera' alla divisione delle offerte,
 * ma si limitera' a diminuire il numero dei posti di un'offerta 'presa'
 *
 * Per una spiegazione piu' dettagliata, e per un esempio, si veda la relazione
 *
 * ATTENZIONE: utilizzare questa macro puo' portare ad un sovraccarico del sistema nel lungo termine,
 * se ne sconsiglia l'uso su macchine poco performanti.
 *
 * DEFAULT commentata
 *
 */
/* #define UPDATE_OFFER */

/*
 * FREE_OFFER_LIST
 *
 * Queste macro, se definita, fa variare il comportamento del server, nel merito al mantenimento o meno
 * delle offerte elaborate
 *
 * Se questa macro e' settata (non commentata) il server provvedera' ad eliminare tutte le offerte che
 * non vengono accoppiate non nessuna richiesta ad ogni esecuzione del match
 *
 * Se questa macro non e' settata (commentata) il server non eliminera' tali offerte, e le terra'
 * disponibili per tutte le esecuzioni del match
 *
 * INFO: il settaggio o meno di questa macro fa variare l'esito dei risultati calcolati, si pensi
 * bene di che genere di sistema si ha bisogno nel settare o meno questa macro
 *
 * INFO: utilizzare questa macro porta ad un miglioramento delle prestazioni, dato che il numero delle
 * offerte viene azzerato ogni volta.
 *
 * ATTENZIONE: decidere di NON UTILIZZARE questa macro e di UTILIZZARE la macro UPDATE_OFFER puo' portare
 * nel lungo termine ad un allungamento del tempo di elaborazione e ad un rallentamento del sistema
 *
 * DEFAULT non-commentata
 *
 */
#define FREE_OFFER_LIST

/*
 * VERBOSE
 *
 * Se questa macro e' settata verranno stampate tutti i messaggi, compresi quelli interni di ogni
 * singolo thread.
 *
 * Se ne consiglia l'uso soltanto a chi e' interessato a conoscere i dettagli implementativi del sistema,
 * oppure nei casi in cui si sia presentato un errore, e si voglia comprendere la sua natura.
 *
 * INFO: l'elevato numero di stampe potrebbe provocare una leggera diminuzione delle prestazioni.
 *
 * ATTENZIONE: se ne sconsiglia l'uso in una esecuzione normale, dato che l'output prodotto e' molto
 * prolisso
 *
 * DEFAULT commentata
 *
 */
/* #define VERBOSE */



/*
 * ==========================================================================
 *
 * 									MACRO AVANZATE
 *
 * 		Modificare questi valori solo se si e' consapevoli delle conseguenze
 * ==========================================================================
 */

/*
 * CHAR_LENGTH
 * BUFF_SIZE
 *
 * Queste macro sono relative al protocollo di comunicazione implementato nella libreria
 * comsock.
 *
 * I dati relativi ad un messaggio, prima di essere inviati, vengono serializzati in un
 * array di caratteri.
 *
 * Il valore CHAR_LENGTH rappresenta il numero di caratteri necessari per rappresentare
 * la dimensione del buffer.
 *
 * Un numero inferiore di caratteri permettere di esprimere numeri piu' piccoli, e di conseguenza
 * la dimensione del buffer risultera' piu' piccola
 *
 * Il valore di default risulta in genere sufficiente a soddisfare tutti i casi.
 *
 * INFO: incrementare i valori solo in presenza di grafi con nomi di citta' parecchio lunghi,
 * oppure con grafi molto grandi, dove i path generati sono molto lunghi.
 *
 * DEFAULT
 * 		CHAR_LENGTH 4
 * 		BUFF_SIZE 512
 *
 */
#define CHAR_LENGTH 4
#define BUFF_SIZE 512

/*
 * MAX_PID_CHAR
 *
 * Queste macro rappresenta il numero massimo di cifre di un PID (process identifier) del
 * sistema in uso
 *
 * E' utilizzata dal client per generare la stringa che contiene il nome del socket
 * sul quale attende i messaggi in ingresso.
 *
 * Il valore di default risultato corretto nella maggior parte dei casi.
 *
 * ATTENZIONE: se si compila su una macchina che usa PID con un numero di cifre maggiore
 * di MAX_PID_CHAR si potrebbe incorrere in errori critici con conseguente arresto del client
 *
 */
#define MAX_PID_CHAR 6

/* ***************** */
#endif
