Albu Robert Marian 322CC

Proiectul implementeaza un sistem de procesare paralela a pachetelor folosind modelul Producer-Consumer. 
Scopul este de a citi pachete dintr-un fisier, de a le procesa (calcul hash si verdict PASS/DROP) folosind
mai multe fire de executie simultan si de a salva rezultatul sortat cronologic.

Componente principale:

Ring Buffer (Buffer Circular)
Este structura de sincronizare intre thread-ul producator si cele consumatoare. Am folosit un mutex si doua
variabile de conditie (not_full, not_empty) pentru a gestiona accesul la buffer. Producatorul asteapta daca
bufferul este plin, iar consumatorii asteapta daca acesta este gol.

Consumer Threads (Workers)
Se pot lansa intre 1 si 32 de thread-uri de tip consumator. Fiecare thread extrage pachete din ring buffer,
apeleaza functia de procesare si introduce rezultatul intr-o structura de tip Min-Heap partajata.

Min-Heap
Pentru ca thread-urile lucreaza in paralel, ordinea de finalizare a pachetelor nu este neaparat cea cronologica.
Am implementat un Min-Heap pentru a sorta pachetele in timp real dupa timestamp. Operatiile de insert si extract
au complexitate O(log n). Am folosit un mutex separat pentru a proteja heap-ul de accesul concurent al consumatorilor.

Finalizare
Dupa ce toate pachetele au fost procesate si thread-urile consumatoare si-au incheiat executia (pthread_join),
thread-ul principal extrage elementele ramase in heap si le scrie in fisierul de iesire, garantand astfel ordinea
corecta a datelor.
