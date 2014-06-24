CARS - Corso di Sistemi Operativi (SOL)
========

CARS rappresenta un semplice sistema di car-sharing. Il sistema è sviluppato in C, ed ha come scopo quello di mostrare il funzionamento dei pthread in ambiente POSIX. Il sistema è formato dai seguenti componenti

* **mgcars** Un server multithread che gestisce le richieste da parte di più client
* **docars** Un client che si connette al server ed effettua richieste
* **comsock** Il protocollo di comunicazione fra client e server

Il sistema è corredato di:
* [Documentazione in formato Doxygen](http://cortinico.github.io/sol-cars/index.html)
* [Relazione del progetto in formato PDF](../../raw/master/doc/tex/relazione.pdf)

Per inizializzare il sistema è sufficiente invocare il comando
```bash
cd src/
make install
```


