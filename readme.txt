Server
Programul server.c este o implementare a unui server bazat pe TCP, 
cu functionalitati suplimentare pentru gestionarea pachetelor UDP. 
Acesta serveste ca platforma pentru comunicarea intre mai multi clienti, 
facilitand schimbul de mesaje si abonarea la anumite subiecte.

Functionalitati
1. Comunicare TCP
Serverul asculta conexiuni TCP pe un port specificat si accepta conexiuni de la 
clienti. Odata conectat, primeste ID-ul clientului, verifica unicitatea acestuia 
si il adauga in lista sa de conexiuni active. Serverul raspunde cu aceeasi 
informatie la mesajele primite de la clienti.

2. Comunicare UDP
In plus fata de TCP, serverul suporta si comunicarea prin UDP. Asculta pachete 
UDP pe un port separat. Cand primeste un pachet UDP, proceseaza informatia 
bazandu-se pe tipul de date si trimite mesajul transformat catre clientii 
abonati la subiectul corespunzator.

3. Gestionarea abonamentelor
Clientii pot sa se aboneze la subiecte de interes trimitand cereri de abonare. 
Serverul mentine o lista de subiecte pentru fiecare client si transmite mesaje 
catre clientii abonati in functie de potrivirea subiectelor.

4. Oprire
Serverul poate fi oprit in momentul in care primeste comanda "exit" de la 
administrator. Aceasta asigura ca toate resursele sunt eliberate corect si 
ca conexiunile active sunt inchise inainte de incheierea programului.

Detalii de implementare
Serverul utilizeaza apelul de sistem poll() pentru a gestiona eficient mai 
multe conexiuni si date de intrare. Foloseste intrarile non-blocante pentru 
a evita blocarea operatiunilor pe socket, asigurandu-se ca raspunde rapid 
cererilor clientilor.

Gestionarea erorilor este implementata folosind macro-uri definite in helpers.h, 
care ofera functionalitati convenabile pentru verificarea si raportarea 
erorilor.

Am folosit 3 structuri definite in helpers.h cu urmatoarele scopuri:
- "udp_msg_received", pentru primirea mesajelor UDP contine topicul, 
tipul de data si payloadul 
- "udp_msg", pe langa campurile din "udp_msg_received"primim si adresa ip
- "client_topics", in care retinem nr de topicuri la care este abonat 
un client, id-ul clientului si topicurile

common.c contine 2 functii pentru trimiterea, respectiv primirea a unui
specific de bytes din buffer
common.h contine headerele functiilor din common.c si un macro ce reprezinta
dimensiunea maxima a mesajului din buffer

Client
Programul subscriber.c serveste ca aplicatie client care se conecteaza la 
server pentru a primi mesaje si pentru a se abona la anumite subiecte. Ofera 
o interfata de linie de comanda pentru interactiunea cu serverul, permitand 
utilizatorilor sa se aboneze, sa se dezaboneze si sa primeasca mesaje de la 
server.

In realizarea acestei teme am pornit de la scheletul laboratorului 7.