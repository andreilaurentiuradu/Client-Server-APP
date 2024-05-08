/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32
#define MAX_LEN_ID 10

// void add_topic(client_topics *client, const char *topic) {
//     client->topics =
//         realloc(client->topics, (client->nr_topics + 1) * sizeof(char *));
//     client->topics[client->nr_topics] = malloc(strlen(topic) + 1);
//     strcpy(client->topics[client->nr_topics], topic);
//     client->nr_topics++;
// }

// pornim serverul
void run_server(int listen_tcp, int listen_udp) {
    // struct pollfd poll_fds[MAX_CONNECTIONS];
    struct pollfd *poll_fds = malloc(sizeof(struct pollfd) * 3);
    // id-urile clientilor
    char **ids = malloc(sizeof(char *) * 3);
    // topicurile disponibile pentru fiecare client
    client_topics cl_topics[100];

    int nr_clients = 0;
    int num_sockets = 3;
    int rc;
    udp_msg_received received_packet;

    // setam socket-ul listenfd pentru ascultare
    rc = listen(listen_tcp, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    // adaugam noii file descriptors (socketuri pe care se asculta conexiuni) in
    // multimea poll_fds

    // pentru TCP
    poll_fds[0].fd = listen_tcp;
    poll_fds[0].events = POLLIN;

    // pentru input de la tastatura
    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    // pentru UDP
    poll_fds[2].fd = listen_udp;
    poll_fds[2].events = POLLIN;

    while (1) {
        // asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");
        // parcurgem multimea poll_fds
        for (int i = 0; i < num_sockets; i++) {
            // printf("num_sok %d\n", num_sockets);
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == listen_tcp) {
                    /* am primit o cerere de conexiune pe socketul de
                    listen_tcp, pe care o acceptam */

                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    const int cfd1 = accept(
                        listen_tcp, (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(cfd1 < 0, "accept");

                    int flag = 1;
                    // dezactivam algoritmul lui Nagle
                    DIE(setsockopt(cfd1, IPPROTO_TCP, TCP_NODELAY, &flag,
                                   sizeof(int)) < 0,
                        "Nagle error\n");

                    // primim mesajul(id-ul)
                    char buffer_msg[1501];
                    memset(buffer_msg, 0, 1500);
                    rc = recv(cfd1, buffer_msg, 1500, 0);
                    DIE(rc < 0, "read");

                    // verificam daca id-ul exista deja
                    int closed = 0;
                    for (int j = 3; j < 3 + nr_clients; ++j) {
                        if (strcmp(ids[j], buffer_msg) == 0) {
                            printf("Client %s already connected.\n", ids[j]);

                            // opresc conexiunea cu clientul
                            close(cfd1);
                            closed = 1;
                            break;
                        }
                    }
                    if (closed == 1) {
                        continue;
                    }

                    ids = realloc(ids, sizeof(char *) * (3 + nr_clients + 1));
                    ids[3 + nr_clients] = malloc(strlen(buffer_msg) + 1);

                    // verificam daca e null
                    if (ids[3 + nr_clients] == NULL) {
                        printf("Eroare la alocarea memoriei pentru ID.\n");
                        exit(EXIT_FAILURE);
                    }

                    // copiem id-ul clientului in vectorul de id-uri
                    strcpy(ids[3 + nr_clients], buffer_msg);

                    /* adaugam noul socket intors de accept() la multimea
                    descriptorilor de citire */
                    poll_fds = realloc(
                        poll_fds, sizeof(struct pollfd) * (num_sockets + 1));
                    poll_fds[num_sockets].fd = cfd1;
                    poll_fds[num_sockets].events = POLLIN;
                    num_sockets++;

                    // afisam faptul ca avem conectat un client nou
                    printf("New client %s connected from %s:%d.\n", buffer_msg,
                           inet_ntoa(cli_addr.sin_addr),
                           ntohs(cli_addr.sin_port));
                    ++nr_clients;

                    // cream o noua lista de topicuri pentru noul client
                    // cl_topics = malloc(sizeof(client_topics));
                    cl_topics[nr_clients - 1].nr_topics = 0;
                    // cl_topics[nr_clients - 1].topics = malloc(sizeof(char
                    // *));

                    strcpy(cl_topics[nr_clients - 1].id,
                           ids[3 + nr_clients - 1]);

                    break;
                } else if (poll_fds[i].fd == STDIN_FILENO) {
                    // primim mesaje de la tastatura
                    char buff_msg[1501];
                    memset(buff_msg, 0, 1500);
                    fgets(buff_msg, 1500, stdin);

                    // inchidem serverul
                    if (strncmp("exit", buff_msg, 4) == 0) {
                        for (int j = 3; j < num_sockets; ++j) {
                            free(ids[j]);
                            close(poll_fds[j].fd);
                        }
                        free(ids);
                        free(poll_fds);
                        return;
                    }
                } else if (poll_fds[i].fd == listen_udp) {
                    /* am primit o cerere de conexiune pe socketul de
                   listen_udp, pe care o acceptam */
                    socklen_t udp_len = 0;

                    struct sockaddr_in serv_addr;
                    int rc_udp = recvfrom(
                        listen_udp, &received_packet, sizeof(received_packet),
                        0, (struct sockaddr *)&serv_addr, &udp_len);
                    DIE(rc_udp < 0, "recv");

                    // cream packetul udp de sent
                    udp_msg sent_packet;
                    sent_packet.ip_address = serv_addr.sin_addr;
                    sent_packet.port = htons(serv_addr.sin_port);
                    memset(sent_packet.topic, 0, 51);
                    memset(sent_packet.payload, 0, 1500);
                    strncpy(sent_packet.topic, received_packet.topic, 51);

                    switch (received_packet.data_type) {
                        // INT
                        case 0:
                            // bitul de semn
                            char bit_sgn = received_packet.payload[0];
                            // numarul
                            uint32_t number = ntohl(
                                *((uint32_t *)(received_packet.payload + 1)));

                            if (bit_sgn == 1) {
                                // daca e negativ
                                sprintf(sent_packet.payload, "%d", -number);
                            } else {
                                // daca e pozitiv
                                sprintf(sent_packet.payload, "%d", number);
                            }
                            break;
                        // SHORRT_REAL
                        case 1:
                            uint16_t number2 =
                                ntohs(*((uint16_t *)received_packet.payload));
                            sprintf(sent_packet.payload, "%.2f",
                                    1.0 * number2 / 100);
                            break;
                        // FLOAT
                        case 2:
                            // bitul de semn
                            char bit_sgn2 = received_packet.payload[0];
                            // numarul
                            uint32_t modu = ntohl(
                                *((uint32_t *)(received_packet.payload + 1)));
                            uint8_t decimals =
                                (*((uint8_t *)(received_packet.payload + 1 +
                                               sizeof(uint32_t))));
                            float number3 = modu * 1.0;

                            for (int w = 1; w <= decimals; ++w) {
                                number3 /= 10;
                            }

                            if (bit_sgn2 == 1) {
                                // daca e negativ
                                sprintf(sent_packet.payload, "%.*f", decimals,
                                        -number3);
                            } else {
                                // daca e pozitiv
                                sprintf(sent_packet.payload, "%.*f", decimals,
                                        number3);
                            }
                            break;
                        // STRING
                        case 3:
                            char sir[1501] = {0};
                            strcpy(sir, received_packet.payload);
                            sprintf(sent_packet.payload, "%s", sir);
                            break;
                    }

                    sent_packet.data_type = received_packet.data_type;

                    for (int i1 = 0; i1 < nr_clients; ++i1) {
                        for (int j1 = 0; j1 < cl_topics[i1].nr_topics; ++j1) {
                            // daca am gasit topicul trimitem pachetul
                            // clientului
                            if (strncmp(cl_topics[i1].topics[j1],
                                        sent_packet.topic, 50) == 0) {
                                send_all(poll_fds[3 + i1].fd, &sent_packet,
                                         sizeof(sent_packet));
                                break;
                            }
                        }
                    }
                } else {
                    int rc = recv_all(poll_fds[i].fd, &received_packet,
                                      sizeof(received_packet));
                    DIE(rc < 0, "recv");

                    // se deconecteaza un client(exit)
                    if (rc == 0) {
                        printf("Client %s disconnected.\n", ids[i]);
                        free(ids[i]);
                        close(poll_fds[i].fd);

                        // scoatem din multimea de citire socketul inchis
                        for (int j = i; j < num_sockets - 1; j++) {
                            poll_fds[j] = poll_fds[j + 1];
                            strcpy(ids[j], ids[j + 1]);
                        }
                        // actualizam nr de socketi si de clienti
                        nr_clients--;
                        num_sockets--;
                        poll_fds = realloc(poll_fds,
                                           sizeof(struct pollfd) * num_sockets);
                    } else {
                        // id - ul ids[i]
                        for (int w = 0; w < nr_clients; ++w) {
                            // caut clientul cu id-ul potrivit in
                            // structura
                            // ce retine topicurile pentru fiecare
                            // client
                            if (strcmp(cl_topics[w].id, ids[i]) == 0) {
                                // add_topic(&cl_topics[w],
                                //           received_packet.topic);
                                // verificam daca clientul este deja abonat
                                // la topicul acela
                                int found = 0;
                                for (int w2 = 0; w2 < cl_topics[w].nr_topics;
                                     ++w2) {
                                    if (strcmp(cl_topics[w].topics[w2],
                                               received_packet.topic) == 0) {
                                        found = w2;
                                        break;
                                    }
                                }

                                // daca nu este abonat il abonam
                                // am primit un mesaj(subscribe)
                                if (received_packet.data_type == 1) {
                                    if (!found) {
                                        memset(
                                            cl_topics[w]
                                                .topics[cl_topics[w].nr_topics],
                                            0, 51);
                                        strcpy(
                                            cl_topics[w]
                                                .topics[cl_topics[w].nr_topics],
                                            received_packet.topic);
                                        cl_topics[w].nr_topics++;
                                    }
                                } else if (received_packet.data_type == 0) {
                                    // daca trebuie sa dam unsubscribe
                                    if (found) {
                                        for (int t = found + 1;
                                             t < cl_topics[w].nr_topics; ++t) {
                                            strcpy(cl_topics[w].topics[t],
                                                   cl_topics[w].topics[t + 1]);
                                        }
                                        cl_topics[w].nr_topics--;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    // primesc portul la care se deschide serverul
    if (argc != 2) {
        printf("\n<Port> %s\n", argv[1]);
        return 1;
    }

    // parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // obtinem un socket TCP pentru receptionarea conexiunilor
    const int listen_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listen_tcp < 0, "socket");

    // completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz
    // ca rulam de 2 ori rapid
    const int enable = 1;
    if (setsockopt(listen_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // datele despre server
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listen_tcp, (const struct sockaddr *)&serv_addr,
              sizeof(serv_addr));
    DIE(rc < 0, "bind");

    // obtinem un socket UDP pentru receptionarea conexiunilor
    const int listen_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(listen_udp < 0, "socket");

    // facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz
    // ca rulam de 2 ori rapid
    const int enable_udp = 1;
    if (setsockopt(listen_udp, SOL_SOCKET, SO_REUSEADDR, &enable_udp,
                   sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listen_udp, (const struct sockaddr *)&serv_addr,
              sizeof(serv_addr));
    DIE(rc < 0, "bind");

    // dezactivam algoritmul lui Nagle
    DIE(setsockopt(listen_tcp, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) <
            0,
        "Nagle error\n");

    // pornim serverul
    run_server(listen_tcp, listen_udp);

    // inchidem listenfd
    close(listen_tcp);
    close(listen_udp);

    return 0;
}
