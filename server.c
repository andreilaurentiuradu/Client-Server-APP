/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
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

void run_server(int listenfd) {
    struct pollfd poll_fds[MAX_CONNECTIONS];
    // id-urile clientilor
    char ids[MAX_CONNECTIONS][MAX_LEN_ID];
    int nr_clients = 0;
    int num_sockets = 2;
    int rc;

    struct chat_packet received_packet;

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(listenfd, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    // Adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea poll_fds
    poll_fds[0].fd = listenfd;
    poll_fds[0].events = POLLIN;

    // pentru input de la tastatura
    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    while (1) {
        // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                // printf("poll_fds[i].fd: %d", poll_fds[i].fd);
                if (poll_fds[i].fd == listenfd) {
                    // Am primit o cerere de conexiune pe socketul de listen, pe
                    // care o acceptam

                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    const int cfd1 = accept(
                        listenfd, (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(cfd1 < 0, "accept");

                    // primesc mesajul(id-ul)
                    char buffer_msg[1501];
                    memset(buffer_msg, 0, 1500);
                    rc = recv(cfd1, buffer_msg, 1500, 0);
                    DIE(rc < 0, "read");
                    // copiem id-ul clientului in vectorul de id-uri
                    strcpy(ids[2 + nr_clients], buffer_msg);

                    // Adaugam noul socket intors de accept() la multimea
                    // descriptorilor de citire
                    // printf("cfd1 %d\n", cfd1);
                    poll_fds[num_sockets].fd = cfd1;
                    poll_fds[num_sockets].events = POLLIN;
                    num_sockets++;

                    printf("New client %s connected from %s:%d.\n", buffer_msg,
                           inet_ntoa(cli_addr.sin_addr),
                           ntohs(cli_addr.sin_port));
                    ++nr_clients;
                    break;
                } else if (poll_fds[i].fd == STDIN_FILENO) {
                    // mesaje de la tastatura
                    char buff_msg[1501];
                    memset(buff_msg, 0, 1500);
                    fgets(buff_msg, 1500, stdin);

                    // inchidem serverul
                    if (strncmp("exit", buff_msg, 4) == 0) {
                        for (int j = 2; j < num_sockets; ++j) {
                            close(poll_fds[j].fd);
                        }
                        return;
                    }
                } else {
                    // se deconecteaza un client
                    int rc = recv_all(poll_fds[i].fd, &received_packet,
                                      sizeof(received_packet));
                    DIE(rc < 0, "recv");

                    if (rc == 0) {
                        printf("Client %s disconnected.\n", ids[i]);
                        close(poll_fds[i].fd);
                        // printf("nr socketuri:%d nr_clienti:%d\n",
                        // num_sockets,
                        //  nr_clients);
                        // Scoatem din multimea de citire socketul inchis
                        for (int j = i; j < num_sockets - 1; j++) {
                            poll_fds[j] = poll_fds[j + 1];
                            strcpy(ids[j], ids[j + 1]);
                        }
                        nr_clients--;
                        num_sockets--;
                        // printf("nr socketuri:%d nr_clienti:%d\n",
                        // num_sockets,
                        //  nr_clients);

                    } else {
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

    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Obtinem un socket TCP pentru receptionarea conexiunilor
    const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz
    // ca rulam de 2 ori rapid
    const int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // datele despre server
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind");
    listen(listenfd, 2);

    // pornim serverul
    run_server(listenfd);

    // Inchidem listenfd
    close(listenfd);

    return 0;
}
